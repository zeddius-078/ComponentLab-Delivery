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
                     const QString& profileMode) {
    Q_UNUSED(t);
    return token_input::profile(profiles, profileMode + '-' + theme);
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
} // namespace
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        auto a = app.arguments();
        require(a.size() == 3 || (a.size() == 4 && a[3] == "--check"),
                "Usage: checkbox_token_compiler SOURCE OUTPUT [--check]");
        QString root = QDir(a[1]).absolutePath(), out = QDir(a[2]).absolutePath();
        QDir project(QCoreApplication::applicationDirPath());
        while (!project.exists("Source/Publish-ComponentSDK.ps1"))
            require(project.cdUp(), "COMPONENT_PROJECT_NOT_FOUND");
        const QString buildRoot = project.filePath("output/work/generated") + '/';
        require(QDir::cleanPath(out).startsWith(buildRoot, Qt::CaseInsensitive),
                "OUTPUT_MUST_BE_IN_WORK");
        bool check = a.size() == 4;
        auto contract = json(root + "/tokens/checkbox-contract.json"),
             profiles = json(root + "/DesignSystem/generated/runtime/tokens_qt.json");
        Tokens base = token_input::profile(
            profiles, profiles["$meta"].toObject()["defaultProfile"].toString());
        QCryptographicHash hash(QCryptographicHash::Sha256);
        QJsonObject inputHashes;
        for (auto path :
             {"DesignSystem/generated/runtime/tokens_qt.json", "tokens/checkbox-contract.json",
              "styles/Z_CheckBox.qss.in",
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
            for (QString profileMode : {"standard", "wide"}) {
                QString profile = profileMode + '-' + theme;
                auto t = profileTokens(base, profiles, theme, profileMode);
                auto cb = contract["checkbox"].toObject();
                auto cp = palette(cb["palette"].toObject(), t);
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
                QJsonObject recipes;
                QString qss = QString::fromUtf8(read(root + "/styles/Z_CheckBox.qss.in"));
                for (auto pv : presets) {
                    auto preset = resolve(pv, t).toObject();
                    validatePreset(preset, t);
                    QString id = preset["id"].toString();
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
                    }
                }
                QJsonObject data{{"profile", profile},
                                 {"sourceHash", sourceHash},
                                 {"inputs", inputHashes},
                                 {"presets", presets},
                                 {"sizes", sizes},
                                 {"recipes", recipes},
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
