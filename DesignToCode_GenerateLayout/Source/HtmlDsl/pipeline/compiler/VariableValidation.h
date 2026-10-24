#pragma once
// 校验页面已导出的变量引用，不替代 SDK Token 求值，也不扫描未使用的组件库变体。
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace v2 {
inline QJsonObject validateVariables(const QJsonObject& raw, const QString& profile,
                                     const QString& runtimeProfile = {}) {
    const auto variables = raw["variableMap"].toObject();
    const auto sets = raw["variableSetMap"].toObject();
    int references = 0;
    QSet<QString> reached;
    QJsonArray warnings;
    const auto fail = [](const QString& code, const QString& detail) {
        throw std::runtime_error((code + ": " + detail).toStdString());
    };
    // Pixso 普通绑定使用字符串 ID，variableConsumptionMap 则保留内部 GUID 对象。
    const auto aliasId = [](const QJsonValue& value) -> QString {
        if (value.isString()) return value.toString();
        const auto guid = value.toObject()["guid"].toObject();
        const auto session = guid["sessionID"];
        const auto local = guid["localID"];
        if (!session.isDouble() || !local.isDouble() || session.toDouble() < 0 ||
            local.toDouble() < 0 || session.toDouble() != std::floor(session.toDouble()) ||
            local.toDouble() != std::floor(local.toDouble())) return {};
        return QString::number(session.toDouble(), 'f', 0) + ":" + QString::number(local.toDouble(), 'f', 0);
    };
    std::function<QString(const QString&, const QJsonObject&, QSet<QString>&)> resolve;
    resolve = [&](const QString& id, const QJsonObject& modes, QSet<QString>& active) -> QString {
        if (id.isEmpty() || !variables.value(id).isObject())
            fail("VARIABLE_REFERENCE_MISSING", id);
        if (active.contains(id)) fail("VARIABLE_ALIAS_CYCLE", id);
        if (active.size() >= 256) fail("VARIABLE_ALIAS_DEPTH_EXCEEDED", id);
        active.insert(id);
        reached.insert(id);
        const auto variable = variables[id].toObject();
        const auto type = variable["type"].toString();
        const auto setId = variable["variableSetID"].toString();
        const auto available = sets[setId].toArray();
        QString mode = modes[setId].toString();
        if (mode.isEmpty()) {
            for (const auto& candidate : available)
                if (candidate.toObject()["name"] == profile) {
                    if (!mode.isEmpty()) fail("VARIABLE_MODE_AMBIGUOUS", setId);
                    mode = candidate.toObject()["id"].toString();
                }
            // 语义匹配：导出集合可能使用 Light/Dark、Standard/Wide 或
            // light-comfortable 等组合命名；按主题/密度/宽度词元唯一匹配。
            if (mode.isEmpty()) {
                const auto tokensOf = [](const QString& name) {
                    QSet<QString> tokens;
                    for (const auto& part : name.toLower().split(
                             QRegularExpression(QStringLiteral("[^a-z]+")), Qt::SkipEmptyParts))
                        tokens.insert(part);
                    return tokens;
                };
                const auto designTokens = tokensOf(profile);
                const auto runtimeTokens = tokensOf(runtimeProfile);
                QSet<QString> theme, density, width;
                for (const auto* value : {"light", "dark"})
                    if (designTokens.contains(QString::fromLatin1(value))) theme.insert(QString::fromLatin1(value));
                for (const auto* value : {"comfortable", "compact"})
                    if (designTokens.contains(QString::fromLatin1(value))) density.insert(QString::fromLatin1(value));
                for (const auto* value : {"standard", "wide"})
                    if (runtimeTokens.contains(QString::fromLatin1(value))) width.insert(QString::fromLatin1(value));
                const auto intersects = [](const QSet<QString>& left, const QSet<QString>& right) {
                    for (const auto& token : left)
                        if (right.contains(token)) return true;
                    return false;
                };
                QStringList semantic;
                for (const auto& candidate : available) {
                    const auto tokens = tokensOf(candidate.toObject()["name"].toString());
                    bool hasTheme = false, hasDensity = false, hasWidth = false;
                    for (const auto* value : {"light", "dark"})
                        hasTheme = hasTheme || tokens.contains(QString::fromLatin1(value));
                    for (const auto* value : {"comfortable", "compact"})
                        hasDensity = hasDensity || tokens.contains(QString::fromLatin1(value));
                    for (const auto* value : {"standard", "wide"})
                        hasWidth = hasWidth || tokens.contains(QString::fromLatin1(value));
                    // 无词元的模式名（如 Value）交给单模式规则处理，不参与语义猜测。
                    if (!hasTheme && !hasDensity && !hasWidth) continue;
                    bool matches = true;
                    if (hasTheme) matches = matches && !theme.isEmpty() && intersects(tokens, theme);
                    if (matches && hasDensity) matches = matches && !density.isEmpty() && intersects(tokens, density);
                    if (matches && hasWidth) matches = matches && !width.isEmpty() && intersects(tokens, width);
                    if (matches) semantic.append(candidate.toObject()["id"].toString());
                }
                if (semantic.size() == 1) mode = semantic.first();
                else if (semantic.size() > 1) fail("VARIABLE_MODE_AMBIGUOUS", setId);
            }
            // 单模式集合没有选择歧义；多模式集合绝不能默认取第一项。
            if (mode.isEmpty() && available.size() == 1)
                mode = available.first().toObject()["id"].toString();
        }
        bool known = false;
        for (const auto& candidate : available)
            if (candidate.toObject()["id"] == mode) known = true;
        const auto values = variable["modeValues"].toObject();
        if (!known || mode.isEmpty() || !values[mode].isObject())
            fail("VARIABLE_MODE_VALUE_MISSING", id + "/" + mode);
        const auto value = values[mode].toObject();
        const auto data = value["value"].toObject();
        if (value["dataType"] == "ALIAS") {
            const auto target = aliasId(data["alias"]);
            if (target.isEmpty()) fail("INVALID_VARIABLE_ALIAS", id);
            const auto targetType = resolve(target, modes, active);
            if (targetType != type) fail("VARIABLE_TYPE_MISMATCH", id);
        } else {
            if (value["dataType"] != type) fail("VARIABLE_TYPE_MISMATCH", id);
            bool valid = false;
            if (type == "FLOAT")
                valid = data["floatValue"].isDouble() && std::isfinite(data["floatValue"].toDouble());
            else if (type == "COLOR") {
                const auto color = data["colorValue"].toObject();
                valid = !color.isEmpty();
                for (const auto* channel : {"r", "g", "b", "a"}) {
                    const double number = color[channel].toDouble();
                    const double maximum = QString(channel) == "a" ? 1 : 255;
                    valid = valid && color[channel].isDouble() && std::isfinite(number) &&
                        number >= 0 && number <= maximum;
                }
            } else {
                // 尚无真实导出证据的类型不猜字段格式；新增类型须先补采集样例和测试。
                fail("UNSUPPORTED_VARIABLE_LITERAL_TYPE", type);
            }
            if (!valid) fail("INVALID_VARIABLE_LITERAL", id);
        }
        active.remove(id);
        return type;
    };
    std::function<void(const QJsonValue&, QJsonObject, QString, QString)> scan;
    scan = [&](const QJsonValue& value, QJsonObject modes, QString expected, QString path) {
        if (value.isArray()) {
            const auto array = value.toArray();
            for (int i = 0; i < array.size(); ++i)
                scan(array[i], modes, expected, path + "[" + QString::number(i) + "]");
            return;
        }
        if (!value.isObject()) return;
        const auto object = value.toObject();
        const auto localModes = object["variableModeBySetMap"].toObject();
        for (auto it = localModes.begin(); it != localModes.end(); ++it) modes[it.key()] = it.value();
        const auto field = object["field"].toString();
        if (field == "FILLPAINTS" || field == "STROKEPAINTS") expected = "COLOR";
        static const QSet<QString> numericFields{"FONT_SIZE", "HEIGHT", "WIDTH", "STROKE_WEIGHT",
            "STACK_SPACING", "TOP_LEFT_RADIUS", "TOP_RIGHT_RADIUS", "BOTTOM_LEFT_RADIUS", "BOTTOM_RIGHT_RADIUS"};
        if (numericFields.contains(field)) expected = "FLOAT";
        if (object["dataType"] == "ALIAS") {
            ++references;
            QSet<QString> active;
            const auto id = aliasId(object["value"].toObject()["alias"]);
            if (id.isEmpty()) fail("INVALID_VARIABLE_ALIAS", path);
            const auto actual = resolve(id, modes, active);
            if (!expected.isEmpty() && expected != actual) fail("VARIABLE_TYPE_MISMATCH", path);
            // 当前 Pixso colorVar 的 resolvedDataType 有时错误标为 BOOLEAN。
            // 以真实变量定义和消费字段为准，保留元数据差异，不虚构转换。
            const auto hint = object["resolvedDataType"].toString();
            if (!hint.isEmpty() && hint != actual)
                warnings.append(QJsonObject{{"code", "VARIABLE_TYPE_HINT_MISMATCH"},
                    {"path", path}, {"variableId", id}, {"hint", hint}, {"actual", actual}});
            return;
        }
        for (auto it = object.begin(); it != object.end(); ++it)
            scan(it.value(), modes, it.key() == "colorVar" ? "COLOR" : expected,
                 path + "." + it.key());
    };
    scan(raw["pixTreeNodes"], {}, {}, "pixTreeNodes");
    return {{"status", "PASS"}, {"scope", "exported page bindings; SDK rendering remains authoritative"},
            {"referenceCount", references}, {"reachedVariableCount", reached.size()}, {"warnings", warnings}};
}
}
