#include <QCoreApplication>
#include <QColor>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
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
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "Cannot read: " + path);
    return file.readAll();
}
QJsonObject json(const QString& path) {
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(read(path), &error);
    require(error.error == QJsonParseError::NoError && doc.isObject(),
            "JSON error in " + path + ": " + error.errorString());
    return doc.object();
}
QJsonValue resolve(QJsonValue value, const Tokens& tokens, QSet<QString> stack = {}) {
    if (!value.isString())
        return value;
    const auto s = value.toString();
    if (!s.startsWith('{') || !s.endsWith('}'))
        return value;
    const auto key = s.mid(1, s.size() - 2);
    require(tokens.contains(key), "Missing Token: " + key);
    require(!stack.contains(key), "Alias cycle at: " + key);
    stack.insert(key);
    return resolve(tokens[key], tokens, stack);
}
double number(const QJsonValue& value, const Tokens& tokens, const QString& field) {
    const auto v = resolve(value, tokens);
    require(v.isDouble() && std::isfinite(v.toDouble()), "Invalid numeric Token: " + field);
    return v.toDouble();
}
QString n(double value) {
    return QString::number(value, 'g', 12);
}
QString qssColor(const QJsonValue& value, const Tokens& tokens) {
    const auto v = resolve(value, tokens);
    QColor c;
    if (v.isObject()) {
        const auto obj = v.toObject();
        const auto a = obj["components"].toArray();
        require(obj["colorSpace"].toString() == "srgb" && a.size() >= 3, "Unsupported color space");
        const double alpha = a.size() > 3 ? a[3].toDouble() : obj["alpha"].toDouble(1);
        for (int i = 0; i < a.size(); ++i)
            require(a[i].toDouble(-1) >= 0 && a[i].toDouble(2) <= 1, "Invalid sRGB component");
        require(alpha >= 0 && alpha <= 1, "Invalid alpha");
        c = QColor::fromRgbF(a[0].toDouble(), a[1].toDouble(), a[2].toDouble(), alpha);
    } else {
        const auto s = v.toString();
        static const QRegularExpression rgba(
            "^rgba\\(\\s*([0-9.]+)\\s*,\\s*([0-9.]+)\\s*,\\s*([0-9.]+)\\s*,\\s*([0-9.]+)\\s*\\)$");
        const auto m = rgba.match(s);
        if (m.hasMatch()) {
            const double a = m.captured(4).toDouble();
            require(a >= 0 && a <= 1, "Source alpha must be in 0..1: " + s);
            for (int i = 1; i <= 3; ++i)
                require(m.captured(i).toDouble() <= 255, "Invalid RGB: " + s);
            c = QColor(m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt());
            c.setAlphaF(a);
        } else
            c = QColor(s);
    }
    require(c.isValid(), "Invalid color: " + v.toString());
    if (c.alpha() == 255)
        return c.name(QColor::HexRgb);
    return QString("rgba(%1, %2, %3, %4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
}
QString opacity(const QString& color, double alpha) {
    QColor c(color);
    require(c.isValid() && alpha >= 0 && alpha <= 1, "Invalid opacity source");
    return QString("rgba(%1, %2, %3, %4)")
        .arg(c.red())
        .arg(c.green())
        .arg(c.blue())
        .arg(qRound(alpha * 255));
}
Tokens profileTokens(Tokens base, const QJsonObject& profiles, const QString& theme,
                     const QString& density) {
    Q_UNUSED(base);
    return token_input::profile(profiles, theme + '-' + density);
}
void lint(const QString& qss) {
    require(!QRegularExpression("@[A-Za-z][A-Za-z0-9]*@").match(qss).hasMatch(),
            "Unresolved QSS placeholder");
    require(!QRegularExpression(":\\s*;").match(qss).hasMatch(), "Empty QSS declaration");
    require(!QRegularExpression("(?m)^\\s*(width|height|margin(?:-[a-z]+)?)\\s*:")
                 .match(qss)
                 .hasMatch(),
            "Forbidden Button QSS dimension");
    require(qss.count('{') == qss.count('}'), "Unbalanced QSS blocks");
}
void output(const QString& path, const QByteArray& bytes, bool check) {
    if (QFile::exists(path) && read(path) == bytes)
        return;
    require(!check, "Generated file differs: " + path);
    require(QDir().mkpath(QFileInfo(path).absolutePath()), "Cannot create output directory");
    QSaveFile file(path);
    require(file.open(QIODevice::WriteOnly), "Cannot write: " + path);
    require(file.write(bytes) == bytes.size() && file.commit(), "Cannot commit: " + path);
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        const auto args = app.arguments();
        require(args.size() == 3 || (args.size() == 4 && args[3] == "--check"),
                "Usage: button_token_compiler SOURCE OUTPUT [--check]");
        const QString root = QDir(args[1]).absolutePath(), dest = QDir(args[2]).absolutePath();
        const bool check = args.size() == 4;
        const auto contract = json(root + "/tokens/button-contract.json");
        const auto profiles = json(root + "/DesignSystem/generated/runtime/tokens_qt.json");
        Tokens base = token_input::profile(
            profiles, profiles["$meta"].toObject()["defaultProfile"].toString());
        QCryptographicHash hasher(QCryptographicHash::Sha256);
        const QStringList inputs = {"DesignSystem/generated/runtime/tokens_qt.json",
                                    "tokens/button-contract.json", "styles/Z_PushButton.qss.in",
                                    "tools/token_compiler.cpp", "tools/token_inputs.h"};
        QJsonObject inputHashes;
        for (const auto& name : inputs) {
            const auto bytes = read(root + '/' + name);
            inputHashes[name] = QString::fromLatin1(
                QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
            hasher.addData(name.toUtf8());
            hasher.addData(bytes);
        }
        const auto hash = QString::fromLatin1(hasher.result().toHex());
        const QString templateText = QString::fromUtf8(read(root + "/styles/Z_PushButton.qss.in"));
        const auto presets = contract["presets"].toArray();
        const QStringList expected = {"PRIMARY,PRIMARY,SOLID", "DEFAULT,NEUTRAL,OUTLINED",
                                      "DASHED,NEUTRAL,DASHED", "TEXT,NEUTRAL,TEXT",
                                      "LINK,PRIMARY,LINK"};
        require(presets.size() == 5, "Exactly five Presets required");
        for (int i = 0; i < 5; ++i) {
            QStringList fields;
            for (const auto& value : presets[i].toArray())
                fields << value.toString();
            require(fields.join(',') == expected[i], "Preset Contract mismatch");
        }
        const auto font = contract["font"].toObject();
        const QString family = resolve(font["family"], base).toString();
        require(QRegularExpression("^[A-Za-z0-9 _-]+$").match(family).hasMatch(),
                "Unsafe QSS font family");
        const auto motion = contract["loading"].toObject();
        const int cycle = motion["cycleMs"].toInt(), frame = motion["frameMs"].toInt(),
                  sweep = motion["sweepDegrees"].toInt();
        require(cycle > 0 && frame > 0 && frame <= cycle && sweep > 0 && sweep <= 360,
                "Invalid Loading motion");
        QString header = "// GENERATED. Edit tokens/button-contract.json or refresh the locked "
                         "source.\n#pragma once\n#include \"Z_PushButton.h\"\n#include "
                         "<stdexcept>\nnamespace button_generated {\n";
        header += "inline constexpr const char* sourceHash = \"" + hash + "\";\n";
        header += "inline constexpr const char* fontFamily = \"" + family + "\";\n";
        header += "inline constexpr int bodyFontSize = " +
                  n(number(base["base.fontSize.body-sm"], base, "body")) + ";\n";
        const auto occupancy =
            number(contract["demoIcons"].toObject()["glyphOccupancy"], base, "demoGlyphOccupancy");
        require(occupancy > 0 && occupancy <= 1, "Invalid demo glyph occupancy");
        header += "inline constexpr double demoGlyphOccupancy = " + n(occupancy) + ";\n";
        header += "inline commonWidget::button_detail::Runtime metrics(const QString& profile) {\n "
                  "   commonWidget::button_detail::Runtime r;\n";
        QJsonArray summaries;
        for (const QString theme : {QString("light"), QString("dark")}) {
            for (const QString density : {QString("comfortable"), QString("compact")}) {
                const QString profile = theme + '-' + density;
                const auto tokens = profileTokens(base, profiles, theme, density);
                const auto focus = contract["focusIndicator"].toObject();
                const auto focusMode = resolve(focus["mode"], tokens).toString();
                const auto focusOpacity = number(focus["opacity"], tokens, "focusOpacity");
                require(QStringList{"off", "keyboard", "always"}.contains(focusMode),
                        "Invalid focus indicator mode");
                require(focusOpacity >= 0 && focusOpacity <= 1, "Invalid focus opacity");
                QMap<QString, QString> palette;
                const auto sourcePalette = contract["palette"].toObject();
                for (auto it = sourcePalette.begin(); it != sourcePalette.end(); ++it)
                    palette[it.key()] = qssColor(it.value(), tokens);
                const auto shape = contract["shape"].toObject(),
                           ghost = contract["ghost"].toObject();
                const auto weight = number(font["weight"], tokens, "fontWeight");
                require(weight == 400 || weight == 500 || weight == 600, "Unsupported font weight");
                const QStringList metricKeys = {"minimumHeight", "paddingInline", "paddingBlock",
                                                "gap",           "borderWidth",   "iconCanvas",
                                                "iconContainer", "indicatorSize", "indicatorStroke",
                                                "iconOnlySize"};
                const QStringList sizes = {"XS", "S", "M", "L"};
                const auto sizesInput = contract["metrics"].toArray();
                require(sizesInput.size() == 4, "Exactly four sizes required");
                QJsonArray resolvedSizes;
                QString qss = "/* GENERATED | " + profile + " | " + hash + " | DEMO-PROPOSAL */\n";
                header += "    if (profile == \"" + profile + "\") {\n        r.sizes = {{\n";
                int rows = 0;
                for (int si = 0; si < 4; ++si) {
                    auto size = sizesInput[si].toObject();
                    require(size["name"].toString() == sizes[si], "Invalid size order");
                    if (density == "compact") {
                        const auto overrides = contract["compact"].toObject()[sizes[si]].toObject();
                        for (auto it = overrides.begin(); it != overrides.end(); ++it)
                            size[it.key()] = it.value();
                    }
                    QMap<QString, double> m;
                    for (const auto& key : metricKeys)
                        m[key] = number(size[key], tokens, key);
                    m["fontSize"] = number(size["fontSize"], tokens, "fontSize");
                    require(m["minimumHeight"] >= 32 && m["fontSize"] >= 12,
                            "Interaction/font floor violated");
                    require(m["iconOnlySize"] >= 32 &&
                                m["iconOnlySize"] >= qMax(m["iconContainer"], m["indicatorSize"]) +
                                                         2 * m["borderWidth"],
                            "Icon-only interaction floor or content fit violated");
                    require(m["iconContainer"] >=
                                m["iconCanvas"] +
                                    2 * number(shape["iconBorderWidth"], tokens, "iconBorderWidth"),
                            "Icon container clips glyph");
                    require(m["indicatorStroke"] > 0 && m["indicatorStroke"] < m["indicatorSize"],
                            "Invalid indicator geometry");
                    QJsonObject resolved;
                    QStringList initializer;
                    for (const auto& key : metricKeys) {
                        require(m[key] >= 0, "Negative dimension: " + key);
                        if (key != "indicatorStroke")
                            require(std::floor(m[key]) == m[key],
                                    "Non-integral logical dimension: " + key);
                        initializer << n(m[key]);
                        resolved[key] = m[key];
                    }
                    resolved["fontSize"] = m["fontSize"];
                    resolved["name"] = sizes[si];
                    resolvedSizes.append(resolved);
                    header += "            commonWidget::button_detail::Metrics{" +
                              initializer.join(',') + "}" + (si == 3 ? "\n" : ",\n");
                    for (const auto& presetValue : presets) {
                        const auto preset = presetValue.toArray();
                        const QString variant = preset[2].toString();
                        for (bool danger : {false, true})
                            for (bool isGhost : {false, true})
                                for (int state = 0; state < 4; ++state) {
                                    const QString color = danger ? "DANGER" : preset[1].toString();
                                    const QString role = color == "DANGER" ? "danger" : "primary";
                                    const QString accent = palette[role + (state == 1   ? "Hover"
                                                                           : state == 2 ? "Pressed"
                                                                                        : "")];
                                    const QString onAccent = color == "DANGER"
                                                                 ? palette["onBrand"]
                                                                 : palette["primaryText"];
                                    QString bg = "transparent",
                                            fg = color == "NEUTRAL" && state == 0
                                                     ? palette["foreground"]
                                                     : accent;
                                    QString border = "transparent";
                                    QString borderStyle = variant == "DASHED" ? "dashed" : "solid";
                                    if (variant == "SOLID") {
                                        bg = accent;
                                        fg = onAccent;
                                        border = accent;
                                    } else if (variant == "OUTLINED" || variant == "DASHED") {
                                        bg = palette["surface"];
                                        border = color == "NEUTRAL" && state == 0
                                                     ? palette["border"]
                                                     : accent;
                                    } else if (state == 1 || state == 2) {
                                        bg = color == "NEUTRAL"
                                                 ? palette[state == 1 ? "neutralHover"
                                                                      : "neutralPressed"]
                                                 : palette[role + "Soft"];
                                    }
                                    if (isGhost) {
                                        fg = color == "DANGER" ? accent : palette["onBrand"];
                                        bg = state == 1 || state == 2
                                                 ? opacity(palette["onBrand"],
                                                           ghost[state == 1 ? "hoverOpacity"
                                                                            : "pressedOpacity"]
                                                               .toDouble())
                                                 : "transparent";
                                        border = variant == "TEXT" || variant == "LINK"
                                                     ? "transparent"
                                                     : fg;
                                    }
                                    if (state == 3) {
                                        fg = isGhost ? opacity(palette["onBrand"],
                                                               ghost["disabledOpacity"].toDouble())
                                                     : palette["disabledText"];
                                        bg = isGhost || variant == "TEXT" || variant == "LINK"
                                                 ? "transparent"
                                                 : palette["disabled"];
                                        border = variant == "TEXT" || variant == "LINK"
                                                     ? "transparent"
                                                     : (isGhost ? fg : palette["disabledBorder"]);
                                    }
                                    QString selector =
                                        QString("commonWidget--Z_PushButton[buttonColor=\"%1\"]["
                                                "buttonVariant=\"%2\"][ghost=\"%3\"][buttonSize=\"%"
                                                "4\"]")
                                            .arg(color, variant, isGhost ? "true" : "false",
                                                 sizes[si]);
                                    const QStringList visualStates{"DEFAULT", "HOVER", "PRESSED",
                                                                   "DISABLED"};
                                    const QString partSelector =
                                        selector + "[visualState=\"" + visualStates[state] + "\"]";
                                    if (state == 1)
                                        selector += ":enabled:hover[loading=\"false\"]";
                                    if (state == 2)
                                        selector += ":enabled:pressed[loading=\"false\"]";
                                    if (state == 3)
                                        selector += ":disabled[disabledByPolicy=\"true\"]";
                                    QMap<QString, QString> row{
                                        {"selector", selector},
                                        {"background", bg},
                                        {"foreground", fg},
                                        {"borderWidth", n(m["borderWidth"])},
                                        {"borderStyle", borderStyle},
                                        {"borderColor", border},
                                        {"radius", n(number(shape["radius"], tokens, "radius"))},
                                        {"fontFamily", family},
                                        {"fontSize", n(m["fontSize"])},
                                        {"fontWeight", n(weight)},
                                        {"focusBorder",
                                         opacity(danger ? palette["danger"]
                                                 : isGhost ? palette["onBrand"] : palette["focus"],
                                                 focusOpacity)},
                                        {"iconBorderWidth", n(number(shape["iconBorderWidth"],
                                                                     tokens, "iconBorderWidth"))},
                                        {"iconRadius",
                                         n(number(shape["iconRadius"], tokens, "iconRadius"))}};
                                    row["partSelector"] = partSelector;
                                    row["iconFilledBackground"] =
                                        state == 3
                                            ? (isGhost ? "transparent" : palette["disabled"])
                                            : (variant == "SOLID" || isGhost
                                                   ? opacity(palette["onBrand"],
                                                             number(shape["iconFilledOpacity"],
                                                                    tokens, "iconFilledOpacity"))
                                                   : accent);
                                    row["iconFilledBorder"] = "transparent";
                                    row["iconFilledForeground"] =
                                        state == 3 || variant == "SOLID" || isGhost ? fg : onAccent;
                                    row["iconOutlinedBorder"] = fg;
                                    row["iconOutlinedForeground"] = fg;
                                    row["iconBareForeground"] = fg;
                                    QString rule = templateText;
                                    for (auto it = row.begin(); it != row.end(); ++it)
                                        rule.replace('@' + it.key() + '@', it.value());
                                    qss += rule + '\n';
                                    ++rows;
                                }
                    }
                }
                header += "        }};\n        r.cycleMs = " + n(cycle) +
                          "; r.frameMs = " + n(frame) + "; r.sweepDegrees = " + n(sweep) +
                          ";\n        return r;\n    }\n";
                lint(qss);
                output(dest + '/' + profile + "/Z_PushButton.qss", qss.toUtf8(), check);
                const QString demo =
                    QString("/* GENERATED DEMO SHELL | %1 */\n"
                            "QMainWindow, QDialog { background: %2; }\n"
                            "QWidget { color: %3; font-family: '%4'; font-size: %5px; }\n"
                            "QLineEdit, QComboBox, QTreeWidget, QTableWidget, QPlainTextEdit { "
                            "background: %6; color: %3; selection-background-color: %7; "
                            "selection-color: %8; }\n"
                            "QGroupBox { border: 1px solid %9; border-radius: 4px; padding-top: "
                            "14px; }\n"
                            "QGroupBox::title { subcontrol-origin: margin; left: 8px; }\n"
                            "QWidget#previewHost { background: %6; }\n"
                            "QWidget#previewHost[ghostSurface=\"true\"], QWidget#ghostSampleHost { "
                            "background: %10; }\n"
                            "QLabel#titleLabel { font-size: %11px; font-weight: 600; }\n")
                        .arg(hash, palette["app"], palette["foreground"], family,
                             n(number(base["base.fontSize.body-sm"], tokens, "body")),
                             palette["surface"], palette["primary"], palette["primaryText"],
                             palette["border"]);
                QString demoFinal = demo;
                demoFinal.replace("%10", qssColor(ghost["surface"], tokens));
                demoFinal.replace(
                    "%11", n(number(base["base.fontSize.subheading-sm"], tokens, "heading")));
                demoFinal +=
                    "QLineEdit, QComboBox, QCheckBox, QRadioButton, QToolButton { min-height: " +
                    n(number(tokens["semantic.dimension.control.height-xs"], tokens, "control")) +
                    "px; }\n";
                output(dest + '/' + profile + "/demo.qss", demoFinal.toUtf8(), check);
                QJsonObject resolvedPalette;
                for (auto it = palette.begin(); it != palette.end(); ++it)
                    resolvedPalette[it.key()] = it.value();
                summaries.append(QJsonObject{{"profile", profile},
                                             {"rows", rows},
                                             {"sizes", resolvedSizes},
                                             {"palette", resolvedPalette},
                                             {"focusIndicator", QJsonObject{{"mode", focusMode},
                                                                           {"opacity", focusOpacity}}},
                                             {"qssBytes", qss.toUtf8().size()}});
            }
        }
        header += "    throw std::invalid_argument(\"Unknown Button Profile\");\n}\n}\n";
        output(dest + "/tokens.h", header.toUtf8(), check);
        const QJsonObject manifest{
            {"contract", contract["contract"]},
            {"status", contract["status"]},
            {"sourceHash", hash},
            {"inputs", inputHashes},
            {"profiles", summaries},
            {"icons", contract["demoIcons"]},
            {"decisions", contract["decisions"]},
            {"loading", motion},
            {"fontFamily", family},
            {"bodyFontSize", number(base["base.fontSize.body-sm"], base, "body")}};
        output(dest + "/manifest.json", QJsonDocument(manifest).toJson(QJsonDocument::Indented),
               check);
        QTextStream(stdout) << (check ? "CHECK OK" : "GENERATED") << " | 4 profiles | " << hash
                            << '\n';
        return 0;
    } catch (const std::exception& e) {
        QTextStream(stderr) << "Token compiler: " << e.what() << '\n';
        return 1;
    }
}
