#pragma once
// C++ 共用的对象型 JSON 读取入口，缺文件或格式错误通过异常交给上层处理。
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <stdexcept>

namespace v2 {
inline QJsonObject readJson(const QString& path) {
    // 顶层必须是对象，避免数组或标量被默认转换为空对象后继续运行。
    QFile f(path);
    // 输入文件必须可读；缺失时携带路径抛错，便于定位部署缺件，而不把空内容交给 JSON 解析。
    if (!f.open(QIODevice::ReadOnly))
        throw std::runtime_error(("INPUT_MISSING: " + path).toStdString());
    QJsonParseError e;
    const auto d = QJsonDocument::fromJson(f.readAll(), &e);
    // 同时核对语法和顶层类型，确保调用方后续按对象读取字段时有合法输入。
    if (e.error != QJsonParseError::NoError || !d.isObject())
        throw std::runtime_error("INVALID_JSON");
    return d.object();
}
}
