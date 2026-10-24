#pragma once
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <functional>
#include <stdexcept>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace v2 {
// 与生成器共用同一发布事务；writer 在生产中是 QSaveFile，测试中可注入写入失败。
// 暂存目录生命周期仍由调用方的 QTemporaryDir 管理，成功后才关闭自动清理。
inline void publishFormalPackage(const QString& staging, const QString& destination,
                                 const QString& recordPath, QJsonObject& record,
                                 const std::function<void(const QString&, const QByteArray&)>& writer) {
    record["published"] = false;
    writer(recordPath, QJsonDocument(record).toJson());
    bool renamed = false;
#ifdef Q_OS_WIN
    // Windows 上目录改名可能被扫描器/索引器的短时句柄占用拒绝（ACCESS_DENIED/
    // SHARING_VIOLATION）。仅对这两类瞬时错误做有限重试，其余错误立即失败。
    DWORD lastError = 0;
    for (int attempt = 0; attempt < 20 && !renamed; ++attempt) {
        renamed = QDir().rename(staging, destination);
        if (renamed) break;
        lastError = GetLastError();
        if (lastError != ERROR_ACCESS_DENIED && lastError != ERROR_SHARING_VIOLATION)
            break;
        ::Sleep(25);
    }
    const auto detail = renamed ? QString{}
                                : QStringLiteral(" (%1 -> %2, win32=%3)")
                                      .arg(staging, destination).arg(lastError);
#else
    renamed = QDir().rename(staging, destination);
    const auto detail = renamed ? QString{}
                                : QStringLiteral(" (%1 -> %2)").arg(staging, destination);
#endif
    if (!renamed)
        throw std::runtime_error(("OUTPUT_PUBLISH_FAILED" + detail).toStdString());
    record["published"] = true;
    try {
        writer(recordPath, QJsonDocument(record).toJson());
    } catch (...) {
        record["published"] = false;
        if (!QDir().rename(destination, staging))
            throw std::runtime_error("PUBLISH_RECORD_FAILED_RETRACTION_FAILED");
        throw;
    }
}
}
