#pragma once
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <functional>
#include <stdexcept>

namespace v2 {
// 与生成器共用同一发布事务；writer 在生产中是 QSaveFile，测试中可注入写入失败。
// 暂存目录生命周期仍由调用方的 QTemporaryDir 管理，成功后才关闭自动清理。
inline void publishFormalPackage(const QString& staging, const QString& destination,
                                 const QString& recordPath, QJsonObject& record,
                                 const std::function<void(const QString&, const QByteArray&)>& writer) {
    record["published"] = false;
    writer(recordPath, QJsonDocument(record).toJson());
    if (!QDir().rename(staging, destination)) throw std::runtime_error("OUTPUT_PUBLISH_FAILED");
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
