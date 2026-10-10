#include "contract_values.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QTextStream>
using namespace contract_values;

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        const auto a = app.arguments();
        require(a.size() == 3 || (a.size() == 4 && a[3] == "--check"),
                "Usage: forms_token_compiler SOURCE OUTPUT [--check]");
        const auto root = QDir(a[1]).absolutePath(), out = QDir(a[2]).absolutePath();
        const auto runtime = json(root + "/DesignSystem/generated/runtime/tokens_qt.json");
        const auto source = json(root + "/tokens/forms-contract.json");
        const auto presets =
            json(root + "/tokens/checkbox-contract.json")["surfacePresets"].toArray();
        QCryptographicHash hash(QCryptographicHash::Sha256);
        QJsonObject inputs;
        for (auto file :
             {"DesignSystem/generated/runtime/tokens_qt.json", "tokens/forms-contract.json",
              "tokens/checkbox-contract.json", "styles/Z_Field.qss.in", "styles/Z_Select.qss.in",
              "tools/forms_token_compiler.cpp", "tools/contract_values.h",
              "tools/token_inputs.h"}) {
            auto bytes = read(root + '/' + file);
            hash.addData(file);
            hash.addData(bytes);
            inputs[file] = QString::fromLatin1(
                QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
        }
        const auto hashText = QString::fromLatin1(hash.result().toHex());
        const auto all = runtime["profiles"].toObject();
        for (auto p = all.begin(); p != all.end(); ++p) {
            const auto tokens = token_input::profile(runtime, p.key());
            auto data = bind(source, tokens).toObject();
            auto metrics = data["metrics"].toObject(), sizes = data["sizes"].toObject();
            for (auto i = metrics.begin(); i != metrics.end(); ++i)
                number(i.value(), i.key(), 0, 8192);
            require(metrics["actionSize"].toInt() >= 32 && metrics["optionHeight"].toInt() >= 32 &&
                        metrics["tagHeight"].toInt() >= 32,
                    "Interactive floor=32");
            for (auto i = sizes.begin(); i != sizes.end(); ++i) {
                auto s = i.value().toObject();
                for (auto f = s.begin(); f != s.end(); ++f)
                    number(f.value(), f.key(), 0, 1024);
                require(s["height"].toInt() >= 32 && s["fontSize"].toInt() >= 14,
                        "Field height/font floor violated");
                require(s["lineHeight"].toInt() >= s["fontSize"].toInt(),
                        "Line height below font size");
            }
            auto palette = data["palette"].toObject();
            for (auto i = palette.begin(); i != palette.end(); ++i)
                i.value() = color(i.value());
            palette["onAccent"] = color(tokens["semantic.color.text.onBrand"]);
            data["palette"] = palette;
            data["profile"] = p.key();
            data["sourceHash"] = hashText;
            data["inputs"] = inputs;
            data["canonicalRevision"] = runtime["$meta"].toObject()["commonSourceRevision"];
            QJsonObject recipes;
            for (auto pv : presets) {
                const auto preset = bind(pv, tokens).toObject();
                const auto id = preset["id"].toString();
                for (QString variant : {"outlined", "filled", "borderless", "underlined"})
                    for (QString state :
                         {"default", "hover", "focus", "error", "warning", "disabled"}) {
                        bool disabled = state == "disabled",
                             face = variant == "outlined" || variant == "filled";
                        auto bg = palette[variant == "filled" ? "filledBackground" : "background"]
                                      .toString();
                        auto border = palette["border"].toString(), fg = palette["text"].toString();
                        if (state != "default")
                            border = palette[state + "Border"].toString();
                        if (state == "error" || state == "warning")
                            bg = palette[state + "Background"].toString();
                        if (disabled) {
                            bg = palette["disabledBackground"].toString();
                            fg = palette["disabledText"].toString();
                        }
                        if (variant == "filled" && state == "default")
                            border = "transparent";
                        if (variant == "borderless")
                            border = "transparent";
                        QString fill = bg;
                        QJsonArray shadows;
                        if (face && preset["fill"] == "outline")
                            fill = "transparent";
                        if (!face)
                            fill = "transparent";
                        if (face && !disabled) {
                            // The shared preset controls material; the field palette controls
                            // colors.
                            if (preset["fill"] == "linearGradient") {
                                auto from = preset["from"].toArray(), to = preset["to"].toArray();
                                fill = QString("qlineargradient(x1:%1,y1:%2,x2:%3,y2:%4")
                                           .arg(n(from[0].toDouble()), n(from[1].toDouble()),
                                                n(to[0].toDouble()), n(to[1].toDouble()));
                                for (auto stop : preset["stops"].toArray()) {
                                    auto s = stop.toObject();
                                    auto c = s["color"].toString();
                                    c = c == "high"         ? palette["gradientHigh"].toString()
                                        : c == "low"        ? palette["gradientLow"].toString()
                                        : c == "background" ? bg
                                                            : color(s["color"]);
                                    fill += QString(",stop:%1 %2").arg(n(s["at"].toDouble()), c);
                                }
                                fill += ')';
                            }
                            for (auto value : preset["shadows"].toArray()) {
                                auto shadow = value.toObject();
                                shadow["color"] = color(shadow["color"]);
                                shadows.append(shadow);
                            }
                        }
                        double width = metrics["borderWidth"].toDouble();
                        auto radius = face ? metrics["radius"].toDouble() : 0;
                        QString css =
                            QString("background:%1; border:%2px solid %3; border-radius:%4px;")
                                .arg(fill, n(width), border, n(radius));
                        if (variant == "underlined")
                            css += "border-top:0;border-left:0;border-right:0;";
                        if (variant == "borderless")
                            css += "border:0;";
                        recipes[id + '/' + variant + '/' + state] = QJsonObject{
                            {"qss", css},       {"foreground", fg},     {"glyph", fg},
                            {"radius", radius}, {"borderWidth", width}, {"shadows", shadows},
                            {"fill", fill}};
                    }
            }
            data["recipes"] = recipes;
            output(out + '/' + p.key() + "/catalog.json", QJsonDocument(data).toJson(),
                   a.size() == 4);
            output(out + '/' + p.key() + "/Z_Field.qss",
                   read(root + "/styles/Z_Field.qss.in") + '\n' +
                       read(root + "/styles/Z_Select.qss.in"),
                   a.size() == 4);
        }
        QTextStream(stdout) << "Forms profiles=" << all.size() << "; sourceHash=" << hashText
                            << '\n';
        return 0;
    } catch (const std::exception& e) {
        QTextStream(stderr) << e.what() << '\n';
        return 1;
    }
}
