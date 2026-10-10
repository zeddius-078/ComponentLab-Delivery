#pragma once
#include <QJsonObject>
#include <QMap>
#include <stdexcept>

namespace token_input {
using Values = QMap<QString, QJsonValue>;
// Identical consumption shape to PythonGUI TokenService and CGY TokenStore:
// profiles[theme-density] -> groups -> full semantic/component paths.
inline Values profile(const QJsonObject& runtime, const QString& name) {
    const auto profiles = runtime["profiles"].toObject();
    if (!profiles.contains(name))
        throw std::invalid_argument(("Missing canonical Profile: " + name).toStdString());
    Values result;
    const auto groups = profiles[name].toObject();
    for (auto group = groups.begin(); group != groups.end(); ++group) {
        if (group.key().startsWith('$') || !group.value().isObject())
            continue;
        const auto values = group.value().toObject();
        for (auto it = values.begin(); it != values.end(); ++it) {
            if (result.contains(it.key()))
                throw std::invalid_argument("Duplicate generated Token path");
            result[it.key()] = it.value();
        }
    }
    if (result.isEmpty())
        throw std::invalid_argument("Empty canonical Token profile");
    return result;
}
} // namespace token_input
