#include <QCoreApplication>
#include <QColor>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QTextStream>
#include <cmath>
#include <stdexcept>
#include "token_inputs.h"

namespace {
using Tokens = QMap<QString, QJsonValue>;
void require(bool ok, const QString& message) {
    if (!ok)
        throw std::runtime_error(message.toStdString());
}
QByteArray read(const QString& path) {
    QFile f(path);
    require(f.open(QIODevice::ReadOnly), "Cannot read " + path);
    return f.readAll();
}
QJsonObject json(const QString& path) {
    QJsonParseError e;
    auto d = QJsonDocument::fromJson(read(path), &e);
    require(e.error == QJsonParseError::NoError && d.isObject(), "Invalid JSON " + path);
    return d.object();
}
QJsonValue resolve(QJsonValue v, const Tokens& t, QSet<QString> seen = {}) {
    if (v.isString() && v.toString().startsWith('{') && v.toString().endsWith('}')) {
        const auto key = v.toString().mid(1, v.toString().size() - 2);
        require(t.contains(key) && !seen.contains(key), "Missing/cyclic alias " + key);
        seen.insert(key);
        return resolve(t[key], t, seen);
    }
    if (v.isArray()) {
        QJsonArray result;
        for (auto item : v.toArray())
            result.append(resolve(item, t, seen));
        return result;
    }
    if (v.isObject()) {
        auto object = v.toObject();
        for (auto i = object.begin(); i != object.end(); ++i)
            i.value() = resolve(i.value(), t, seen);
        return object;
    }
    return v;
}
double number(QJsonValue v, const Tokens& t, const QString& name, double lo, double hi) {
    v = resolve(v, t);
    require(v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble() >= lo && v.toDouble() <= hi,
            "Invalid numeric field " + name);
    return v.toDouble();
}
QString n(double v) {
    return QString::number(v, 'g', 12);
}
QString color(QJsonValue value, const Tokens& tokens) {
    const auto v = resolve(value, tokens);
    QColor c;
    if (v.isObject()) {
        auto o = v.toObject();
        auto a = o["components"].toArray();
        require(o["colorSpace"].toString() == "srgb" && (a.size() == 3 || a.size() == 4),
                "Only sRGB colors are supported");
        c = QColor::fromRgbF(number(a[0], {}, "red", 0, 1), number(a[1], {}, "green", 0, 1),
                             number(a[2], {}, "blue", 0, 1),
                             number(a.size() == 4                    ? a[3]
                                    : o.value("alpha").isUndefined() ? QJsonValue(1)
                                                                     : o["alpha"],
                                    {}, "alpha", 0, 1));
    } else {
        auto s = v.toString();
        auto m = QRegularExpression("^rgba\\(\\s*([0-9.]+)\\s*,\\s*([0-9.]+)\\s*,\\s*([0-9.]+)\\s*,"
                                    "\\s*([0-9.]+)\\s*\\)$")
                     .match(s);
        if (m.hasMatch()) {
            c = QColor(int(number(m.captured(1).toDouble(), {}, "red", 0, 255)),
                       int(number(m.captured(2).toDouble(), {}, "green", 0, 255)),
                       int(number(m.captured(3).toDouble(), {}, "blue", 0, 255)));
            c.setAlphaF(number(m.captured(4).toDouble(), {}, "alpha", 0, 1));
        } else
            c = QColor(s);
    }
    require(c.isValid(), "Invalid color " + v.toString());
    return c.alpha() == 255 ? c.name()
                            : QString("rgba(%1, %2, %3, %4)")
                                  .arg(c.red())
                                  .arg(c.green())
                                  .arg(c.blue())
                                  .arg(c.alpha());
}
Tokens profileTokens(Tokens t, const QJsonObject& profiles, const QString& theme,
                     const QString& density) {
    Q_UNUSED(t);
    return token_input::profile(profiles, theme + '-' + density);
}
QMap<QString, QString> palette(const QJsonObject& input, const Tokens& t) {
    QMap<QString, QString> p;
    for (auto i = input.begin(); i != input.end(); ++i)
        p[i.key()] = color(i.value(), t);
    return p;
}
void validatePreset(const QJsonObject& p, const Tokens& t) {
    require(QRegularExpression("^[a-z][a-z0-9-]*$").match(p["id"].toString()).hasMatch(),
            "Unsafe/empty preset ID");
    require(!p["label"].toString().isEmpty(), "Empty preset label");
    auto fill = p["fill"].toString();
    require(QStringList{"solid", "linearGradient", "outline"}.contains(fill),
            "Unknown fill backend");
    if (fill == "linearGradient") {
        for (auto k : {"from", "to"}) {
            auto a = p[k].toArray();
            require(a.size() == 2, "Gradient point must have two coordinates");
            for (auto v : a)
                number(v, t, k, 0, 1);
        }
        require(p["from"] != p["to"], "Gradient direction is zero");
        auto stops = p["stops"].toArray();
        require(stops.size() >= 2 && stops.size() <= 16, "Gradient requires 2..16 stops");
        double last = -1;
        for (auto s : stops) {
            double at = number(s.toObject()["at"], t, "gradient stop", 0, 1);
            require(at > last, "Stops must be strictly ascending");
            last = at;
        }
        require(stops.first().toObject()["at"].toDouble() == 0 &&
                    stops.last().toObject()["at"].toDouble() == 1,
                "Stops must cover 0..1");
    }
    auto shadows = p["shadows"].toArray();
    require(shadows.size() <= 8, "Maximum eight shadows");
    require(fill != "outline" || shadows.isEmpty(), "Outline-only requires no shadows");
    for (auto v : shadows) {
        auto s = v.toObject();
        require(s["kind"] == "inset" || s["kind"] == "outer", "Unknown shadow kind");
        color(s["color"], t);
        number(s["alpha"], t, "shadow alpha", 0, 1);
        number(s["x"], t, "shadow x", -32, 32);
        number(s["y"], t, "shadow y", -32, 32);
        number(s["blur"], t, "shadow blur", 0, 32);
        number(s["spread"], t, "shadow spread", -8, 8);
    }
}
QJsonObject recipe(const QJsonObject& preset, const Tokens& t, QString bg, QString fg,
                   QString border, QString high, QString low, QString accent, double radius,
                   double width, bool disabled, bool decorate, QString borderStyle = "solid") {
    QString fill = bg;
    QString kind = preset["fill"].toString();
    QJsonArray shadows;
    if (decorate && kind == "outline") {
        fill = "transparent";
        fg = accent;
        border = accent;
    }
    if (decorate && (!disabled || preset["stateGradient"].toBool()) && kind == "linearGradient") {
        auto a = preset["from"].toArray(), b = preset["to"].toArray();
        fill = QString("qlineargradient(x1:%1, y1:%2, x2:%3, y2:%4")
                   .arg(n(a[0].toDouble()), n(a[1].toDouble()), n(b[0].toDouble()),
                        n(b[1].toDouble()));
        for (auto value : preset["stops"].toArray()) {
            auto stop = value.toObject();
            auto binding = stop["color"].toString();
            auto c = binding == "high"         ? high
                     : binding == "low"        ? low
                     : binding == "background" ? bg
                                               : color(stop["color"], t);
            fill += QString(", stop:%1 %2").arg(n(stop["at"].toDouble()), c);
        }
        fill += ')';
    }
    if (decorate && !disabled)
        for (auto v : preset["shadows"].toArray()) {
            auto s = v.toObject();
            s["color"] = color(s["color"], t);
            for (auto k : {"alpha", "x", "y", "blur", "spread"})
                s[k] = resolve(s[k], t);
            shadows.append(s);
        }
    auto qss =
        QString(
            "background: %1; color: %2; border-style: %3; border-color: %4; border-radius: %5px;")
            .arg(fill, fg, borderStyle, border, n(radius));
    return {{"qss", qss},           {"foreground", fg},   {"glyph", fg}, {"radius", radius},
            {"borderWidth", width}, {"shadows", shadows}, {"fill", fill}};
}
void output(const QString& path, const QByteArray& bytes, bool check) {
    if (QFile::exists(path) && read(path) == bytes)
        return;
    require(!check, "Generated drift: " + path);
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile f(path);
    require(f.open(QIODevice::WriteOnly), "Cannot write " + path);
    require(f.write(bytes) == bytes.size() && f.commit(), "Cannot commit " + path);
}
QJsonObject solidInnerShadow(const QJsonObject& button, const Tokens& t) {
    const auto source = button["solidInnerShadow"].toObject();
    require(source["id"] == "solid-inner-shadow", "Missing solid inner-shadow contract");
    auto geometry = resolve(source["geometry"], t).toObject();
    for (const char* key :
         {"width", "cornerRadius", "fadeStartRatio", "buttonRadius", "borderWidth"})
        number(geometry[key], {}, key, -8192, 8192);
    require(geometry["matchButtonRadiusToInnerShadow"].isBool(),
            "Inner-shadow match switch is boolean");
    const auto states = source["states"].toObject(), defaults = states["default"].toObject();
    QJsonObject resolved;
    QJsonArray fallbacks;
    for (QString state : {"default", "hover", "pressed", "disabled"}) {
        QJsonObject visual;
        for (const char* field : {"shadowColor", "borderColor", "textColor", "bgTop", "bgMid",
                                  "bgBottom", "bgMidStop"}) {
            QJsonValue input = states[state].toObject()[field];
            const auto alias = input.toString();
            const bool missingAlias = alias.startsWith('{') && alias.endsWith('}') &&
                                      !t.contains(alias.mid(1, alias.size() - 2));
            if (state != "default" && (input.isUndefined() || input.isNull() || missingAlias)) {
                input = defaults[field];
                fallbacks.append(state + '.' + field);
            }
            visual[field] = QString(field) == "bgMidStop"
                                ? QJsonValue(number(input, t, field, -8192, 8192))
                                : QJsonValue(color(input, t));
        }
        resolved[state] = visual;
    }
    const auto policy = resolve(source["colorPolicy"], t).toObject();
    require(color(policy["black"], {}) == "#000000", "Inner shadow requires pure black");
    require(policy["baseSelection"] == "darkest-qgray-of-state-stops" &&
                policy["transparentTypes"] == "material-supplies-face" &&
                policy["surfaceSamples"].toArray() == QJsonArray{0, 0.25, 0.5, 0.75, 1} &&
                policy["ghost"] == false,
            "Unsupported geometric shadow color policy");
    QJsonObject roles;
    for (const auto& role : {QString("primary"), QString("secondary"), QString("danger")}) {
        const double alpha = number(policy["blackMix"].toObject()[role], {}, "black mix", 0, 1);
        QJsonObject roleStates;
        for (const auto& state :
             {QString("default"), QString("hover"), QString("pressed"), QString("disabled")}) {
            const auto supplied =
                resolve(button["materialStates"].toObject()[role].toObject()[state], t).toObject();
            const auto stops = supplied["stops"].toArray();
            require(stops.size() == 5, "Expected five supplied Button state gradient stops");
            QJsonArray candidates, backgroundStops;
            for (int i = 0; i < stops.size(); ++i) {
                const auto stop = stops[i].toObject();
                require(number(stop["at"], {}, "stop", 0, 1) == i * .25,
                        "Button state stops must be 0/.25/.5/.75/1");
                const auto normalized = color(stop["color"], t);
                candidates.append(normalized);
                backgroundStops.append(QJsonObject{{"at", i * .25}, {"color", normalized}});
            }
            QColor darkest;
            for (const auto& candidate : candidates) {
                const QColor current(candidate.toString());
                require(current.isValid() && current.alpha() == 255,
                        "Geometric Button face colors must be opaque");
                if (!darkest.isValid() || qGray(current.rgb()) < qGray(darkest.rgb()))
                    darkest = current;
            }
            const QColor shadow(qRound(darkest.red() * (1 - alpha)),
                                qRound(darkest.green() * (1 - alpha)),
                                qRound(darkest.blue() * (1 - alpha)));
            roleStates[state] = QJsonObject{{"bgTop", candidates[0]},
                                            {"bgMid", candidates[2]},
                                            {"bgBottom", candidates[4]},
                                            {"bgMidStop", .5},
                                            {"backgroundStops", backgroundStops},
                                            {"textColor", color(supplied["foreground"], t)},
                                            {"borderColor", color(supplied["border"], t)},
                                            {"shadowColor", shadow.name()},
                                            {"shadowBaseColor", darkest.name()},
                                            {"blackMix", alpha}};
        }
        roles[role] = roleStates;
    }
    return {{"id", source["id"]},    {"label", source["label"]}, {"geometry", geometry},
            {"states", resolved},    {"roleStates", roles},      {"colorPolicy", policy},
            {"fallbacks", fallbacks}};
}
} // namespace
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        auto a = app.arguments();
        require(a.size() == 3 || (a.size() == 4 && a[3] == "--check"),
                "Usage: checkbox_token_compiler SOURCE OUTPUT [--check]");
        QString root = QDir(a[1]).absolutePath(), out = QDir(a[2]).absolutePath();
        bool check = a.size() == 4;
        auto contract = json(root + "/tokens/checkbox-contract.json"),
             button = json(root + "/tokens/button-contract.json"),
             profiles = json(root + "/DesignSystem/generated/runtime/tokens_qt.json");
        Tokens base = token_input::profile(
            profiles, profiles["$meta"].toObject()["defaultProfile"].toString());
        QCryptographicHash hash(QCryptographicHash::Sha256);
        QJsonObject inputHashes;
        for (auto path :
             {"DesignSystem/generated/runtime/tokens_qt.json", "tokens/checkbox-contract.json",
              "tokens/button-contract.json", "styles/Z_CheckBox.qss.in",
              "tools/checkbox_token_compiler.cpp", "tools/token_inputs.h"}) {
            auto bytes = read(root + '/' + path);
            hash.addData(path);
            hash.addData(bytes);
            inputHashes[path] = QString::fromLatin1(
                QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
        }
        QString sourceHash = QString::fromLatin1(hash.result().toHex());
        auto presets = contract["surfacePresets"].toArray();
        require(!presets.isEmpty(), "No surface presets");
        QSet<QString> ids;
        for (auto v : presets) {
            auto p = v.toObject();
            require(!ids.contains(p["id"].toString()), "Duplicate preset");
            ids.insert(p["id"].toString());
        }
        require(ids.contains("solid"), "Default solid preset required");
        QJsonObject allOutputs;
        for (QString theme : {"light", "dark"})
            for (QString density : {"comfortable", "compact"}) {
                QString profile = theme + '-' + density;
                auto t = profileTokens(base, profiles, theme, density);
                auto cb = contract["checkbox"].toObject();
                auto cp = palette(cb["palette"].toObject(), t),
                     bp = palette(button["palette"].toObject(), t);
                double radius = number(cb["radius"], t, "radius", 0, 8),
                       width = number(cb["borderWidth"], t, "border", 0.5, 4);
                auto sizes = cb["sizes"].toObject();
                for (auto i = sizes.begin(); i != sizes.end(); ++i) {
                    auto s = i.value().toObject();
                    for (auto k : {"indicator", "hitTarget", "fontSize", "spacing"})
                        s[k] = number(s[k], t, k, 0, 128);
                    require(s["hitTarget"].toDouble() >= 32 && s["fontSize"].toDouble() >= 14,
                            "Checkbox interaction/body floor violated");
                    require(s["indicator"].toDouble() +
                                    2 * (number(cb["focusWidth"], t, "focusWidth", 1, 4) +
                                         number(cb["focusGap"], t, "focusGap", 0, 8)) <=
                                s["hitTarget"].toDouble(),
                            "Indicator/focus frame exceeds hit target");
                    require(radius <= s["indicator"].toDouble() / 2, "Radius exceeds indicator");
                    sizes[i.key()] = s;
                }
                QJsonObject recipes, capabilities;
                QString qss = QString::fromUtf8(read(root + "/styles/Z_CheckBox.qss.in"));
                for (auto pv : presets) {
                    auto preset = resolve(pv, t).toObject();
                    validatePreset(preset, t);
                    QString id = preset["id"].toString();
                    const bool decorated =
                        preset["fill"] != "solid" || !preset["shadows"].toArray().isEmpty();
                    capabilities[id] =
                        QJsonObject{{"types", decorated ? QJsonArray{"PRIMARY", "DEFAULT", "DASHED"}
                                                        : QJsonArray{"PRIMARY", "DEFAULT", "DASHED",
                                                                     "TEXT", "LINK"}},
                                    {"ghost", !decorated}};
                    for (QString state : {"default", "hover", "pressed", "disabled"}) {
                        bool disabled = state == "disabled";
                        QString suffix =
                            state == "default" ? "" : state.left(1).toUpper() + state.mid(1);
                        auto bindings =
                            contract["gradientStateBindings"].toObject()[state].toObject();
                        auto boundSuffix = [&](const char* which) {
                            auto value = bindings[which].toString();
                            require(QStringList{"default", "hover", "pressed", "disabled"}.contains(
                                        value),
                                    "Unknown gradient state binding");
                            return value == "default" ? QString()
                                                      : value.left(1).toUpper() + value.mid(1);
                        };
                        for (bool selected : {false, true}) {
                            auto r = recipe(
                                preset, t, cp[(selected ? "accent" : "unchecked") + suffix],
                                cp["onAccent"], cp[(selected ? "accent" : "border") + suffix],
                                cp[(selected ? "accent" : "unchecked") + boundSuffix("high")],
                                cp[(selected ? "accent" : "unchecked") + boundSuffix("low")],
                                cp["accent" + suffix], radius, width, disabled, true);
                            r["foreground"] = cp[disabled ? "disabledText" : "text"];
                            recipes["checkbox/" + id + '/' + (selected ? "selected" : "unchecked") +
                                    '/' + state] = r;
                            qss += QString("\nQFrame[checkboxPart=\"FACE\"][surfacePreset=\"%1\"]["
                                           "selectionState=\"%2\"][interactionState=\"%3\"] { %4 "
                                           "border-width:%5px; }\n")
                                       .arg(id, selected ? "selected" : "unchecked", state,
                                            r["qss"].toString(), n(width));
                        }
                        for (QString type : {"PRIMARY", "DEFAULT", "DASHED", "TEXT", "LINK"})
                            for (bool danger : {false, true})
                                for (bool ghost : {false, true}) {
                                    QString role = danger ? "danger" : "primary",
                                            accent = bp[role + (disabled ? QString() : suffix)];
                                    QString bg = "transparent",
                                            fg = (type == "DEFAULT" || type == "DASHED" ||
                                                  type == "TEXT") &&
                                                         state == "default" && !danger
                                                     ? bp["foreground"]
                                                     : accent;
                                    QString border = "transparent";
                                    bool face =
                                        type == "PRIMARY" || type == "DEFAULT" || type == "DASHED";
                                    if (type == "PRIMARY") {
                                        bg = accent;
                                        fg = bp[danger ? "onBrand" : "primaryText"];
                                        border = accent;
                                    } else if (face) {
                                        bg = bp["surface"];
                                        border =
                                            state == "default" && !danger ? bp["border"] : accent;
                                    } else if (state == "hover" || state == "pressed")
                                        bg = bp[role + "Soft"];
                                    if (ghost) {
                                        bg = "transparent";
                                        fg = danger ? accent : bp["onBrand"];
                                        border = face ? fg : "transparent";
                                    }
                                    if (disabled) {
                                        bg = face && !ghost ? bp["disabled"] : "transparent";
                                        fg = bp["disabledText"];
                                        border = face ? bp["disabledBorder"] : "transparent";
                                    }
                                    auto gradientColor = [&](const char* which) {
                                        auto bound = boundSuffix(which);
                                        if (disabled)
                                            return bg;
                                        return type == "PRIMARY" ? bp[role + bound]
                                               : bound.isEmpty() ? bp["surface"]
                                                                 : bp["neutral" + bound];
                                    };
                                    auto material = preset;
                                    auto states = button["materialStates"].toObject();
                                    if (!states.isEmpty() && face && !ghost) {
                                        const auto roleName = danger              ? "danger"
                                                              : type == "PRIMARY" ? "primary"
                                                                                  : "secondary";
                                        const auto supplied =
                                            resolve(states[roleName].toObject()[state], t)
                                                .toObject();
                                        require(!supplied.isEmpty(),
                                                "Missing supplied Button material state");
                                        border = color(supplied["border"], t);
                                        fg = color(supplied["foreground"], t);
                                        if (type != "PRIMARY" || disabled)
                                            bg = color(supplied["stops"]
                                                           .toArray()
                                                           .first()
                                                           .toObject()["color"],
                                                       t);
                                        if (material["fill"] == "linearGradient") {
                                            material["stops"] = supplied["stops"];
                                            material["stateGradient"] = true;
                                        }
                                        QJsonArray shadows;
                                        for (auto value : material["shadows"].toArray()) {
                                            auto shadow = value.toObject();
                                            if (shadow["kind"] == "inset") {
                                                shadow["color"] = supplied["innerShadow"];
                                                const auto soft =
                                                    button["softInnerShadow"].toObject();
                                                require(!soft.isEmpty(),
                                                        "Missing Button soft inset contract");
                                                for (const char* key : {"alpha", "y", "blur"})
                                                    shadow[key] = resolve(soft[key], t);
                                            }
                                            shadows.append(shadow);
                                        }
                                        material["shadows"] = shadows;
                                        validatePreset(material, t);
                                    }
                                    auto r = recipe(material, t, bg, fg, border,
                                                    gradientColor("high"), gradientColor("low"),
                                                    disabled ? bp["disabledText"] : accent,
                                                    number(button["shape"].toObject()["radius"], t,
                                                           "button radius", 0, 8),
                                                    0, disabled, face && !ghost,
                                                    type == "DASHED" ? "dashed" : "solid");
                                    r["iconFilledForeground"] =
                                        disabled || type == "PRIMARY" || ghost
                                            ? r["foreground"]
                                            : QJsonValue(bp[danger ? "onBrand" : "primaryText"]);
                                    recipes[QString("button/%1/%2/%3/%4/%5")
                                                .arg(id, type, danger ? "1" : "0",
                                                     ghost ? "1" : "0", state)] = r;
                                }
                    }
                }
                QJsonObject buttonWidths;
                capabilities["solid-inner-shadow"] = QJsonObject{
                    {"types", QJsonArray{"PRIMARY", "DEFAULT", "DASHED", "TEXT", "LINK"}},
                    {"ghost", false}};
                for (auto value : button["metrics"].toArray()) {
                    auto m = value.toObject();
                    auto size = m["name"].toString();
                    auto v = m["borderWidth"];
                    auto overrides = button["compact"].toObject()[size].toObject();
                    if (density == "compact" && overrides.contains("borderWidth"))
                        v = overrides["borderWidth"];
                    buttonWidths[size] = number(v, t, "button borderWidth", 0.5, 4);
                }
                QJsonObject data{{"profile", profile},
                                 {"sourceHash", sourceHash},
                                 {"inputs", inputHashes},
                                 {"presets", presets},
                                 {"sizes", sizes},
                                 {"recipes", recipes},
                                 {"buttonBorderWidths", buttonWidths},
                                 {"buttonCapabilities", capabilities},
                                 {"buttonInnerShadow", solidInnerShadow(button, t)},
                                 {"fontFamily", resolve(contract["fontFamily"], t)},
                                 {"focusColor", cp["focus"]},
                                 {"focusWidth", number(cb["focusWidth"], t, "focus", 1, 4)},
                                 {"focusGap", number(cb["focusGap"], t, "gap", 0, 8)},
                                 {"glyphs", cb["glyphs"]}};
                allOutputs[profile + "/catalog.json"] =
                    QString::fromUtf8(QJsonDocument(data).toJson());
                allOutputs[profile + "/Z_CheckBox.qss"] = qss;
            }
        for (auto i = allOutputs.begin(); i != allOutputs.end(); ++i)
            output(out + '/' + i.key(), i.value().toString().toUtf8(), check);
        QTextStream(stdout) << "Surface presets=" << presets.size()
                            << "; profiles=4; sourceHash=" << sourceHash << "; "
                            << (check ? "consistent" : "generated") << '\n';
        return 0;
    } catch (const std::exception& e) {
        QTextStream(stderr) << e.what() << '\n';
        return 1;
    }
}
