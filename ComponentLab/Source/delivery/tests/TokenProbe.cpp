#include "DemoRuntime.h"
#include "TokenResources.h"
#include "Z_CheckBox.h"
#include "Z_InputField.h"
#include "Z_Select.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QSaveFile>
#include <QStyleFactory>
#include <QTest>
#include <cstdio>
using namespace commonWidget;
namespace {
QJsonArray rect(const QRect& r) {
    return {r.x(), r.y(), r.width(), r.height()};
}
void require(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
} // namespace
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    QCommandLineParser parser;
    parser.addOptions({{"token-dir", "Generated Token directory", "path"},
                       {"output", "Evidence directory", "path"},
                       {"compare", "Baseline images", "path"},
                       {"profile", "Profile", "name", "standard-light"},
                       {"button-appearance", "Button V2 appearance", "name", "filled"},
                       {"field-appearance", "Input/Select V2 appearance", "name", "outlined"},
                       {"checkbox-appearance", "Checkbox appearance", "name", "solid"}});
    parser.process(app);
    try {
        token_resources::setGeneratedRoot(parser.value("token-dir"));
        button_demo::install(app, parser.value("profile"));
        auto surface = std::make_shared<surface::Catalog>(parser.value("profile"));
        auto fields = std::make_shared<field::Tokens>(parser.value("profile"));
        QString output = parser.value("output");
        require(!output.isEmpty() && QDir().mkpath(output), "Invalid output");
        QFile mappingFile(QString::fromUtf8(COMPONENT_SDK_COMPONENT_MAP));
        require(mappingFile.open(QIODevice::ReadOnly), "Missing component mapping");
        const auto mappings =
            QJsonDocument::fromJson(mappingFile.readAll()).object()["components"].toObject();
        QJsonObject results;
        for (QString name : {"button", "checkbox", "input", "select"}) {
            QWidget canvas;
            canvas.resize(520, 180);
            canvas.setAutoFillBackground(true);
            QWidget* widget = nullptr;
            QJsonObject metrics;
            if (name == "button") {
                auto* button = new Z_PushButton(&canvas);
                button->setText("Execute recipe");
                button->setIntent(ButtonIntent::Primary);
                const QMap<QString, ButtonAppearance> appearances{
                    {"filled", ButtonAppearance::Filled},
                    {"outlined", ButtonAppearance::Outlined},
                    {"dashed", ButtonAppearance::Dashed},
                    {"text", ButtonAppearance::Text},
                    {"link", ButtonAppearance::Link}};
                require(appearances.contains(parser.value("button-appearance")),
                        "Unsupported Button V2 appearance");
                button->setAppearance(appearances.value(parser.value("button-appearance")));
                widget = button;
            } else if (name == "checkbox") {
                auto* check = new Z_CheckBox(surface, "Enable process check", &canvas);
                check->setChecked(true);
                check->setAppearance(parser.value("checkbox-appearance"));
                widget = check;
            } else if (name == "input") {
                auto* input = new Z_InputField(fields, &canvas);
                input->setLabel("Recipe name");
                input->setValue("ETCH_CF4_Standard");
                input->setHint("LOT-0915");
                input->setPrefix("ID");
                input->setAppearance(parser.value("field-appearance"));
                widget = input;
            } else {
                auto* select = new Z_Select(fields, &canvas);
                select->setLabel("Target chamber");
                select->setOptions({{"a", "ETCH-01", {}, false}});
                select->setValues({"a"});
                select->setHint("Process ready");
                select->setAppearance(parser.value("field-appearance"));
                widget = select;
            }
            widget->move(24, 24);
            widget->resize(name == "input" || name == "select"
                               ? QSize(456, widget->sizeHint().height())
                               : widget->sizeHint());
            const auto componentName = name == "button" ? QStringLiteral("Button")
                                     : name == "checkbox" ? QStringLiteral("Checkbox")
                                     : name == "input" ? QStringLiteral("Input")
                                                       : QStringLiteral("Select");
            const auto mapping = mappings[componentName].toObject();
            require(!mapping.isEmpty() && QString(widget->metaObject()->className()) ==
                        mapping["class"].toString(), "Mapped Qt class mismatch");
            const QMap<QString, QStringList> expectedParts{
                {"button", {"LEADING", "LABEL", "TRAILING"}},
                {"checkbox", {"INDICATOR_FACE", "LABEL"}},
                {"input", {"CONTROL", "EDITOR"}},
                {"select", {"CONTROL", "DISPLAY"}},
            };
            QJsonArray mapped;
            for (const auto& part : expectedParts[name]) {
                require(widget->findChild<QWidget*>(part),
                        qPrintable(name + ": mapped QWidget part missing: " + part));
                mapped.append(part);
            }
            require(!mapped.isEmpty(), "Component has no mapping");
            metrics["mappingParts"] = mapped;
            canvas.show();
            QTest::qWait(35);
            QApplication::processEvents();
            metrics["rect"] = rect(widget->geometry());
            if (auto* check = qobject_cast<Z_CheckBox*>(widget))
                metrics["indicator"] = rect(check->indicatorRect());
            for (auto partName :
                 {"LABEL", "CONTROL", "EDITOR", "DISPLAY", "PREFIX", "LEADING", "CONTENT"}) {
                auto* child = widget->findChild<QWidget*>(partName);
                if (!child)
                    continue;
                QJsonObject part{{"rect", rect(child->geometry())},
                                 {"fontSize", child->font().pixelSize()}};
                if (child->layout())
                    part["layout"] = QJsonArray{child->layout()->contentsMargins().left(),
                                                child->layout()->contentsMargins().top(),
                                                child->layout()->contentsMargins().right(),
                                                child->layout()->contentsMargins().bottom(),
                                                child->layout()->spacing()};
                metrics[partName] = part;
            }
            auto image = canvas.grab().toImage().convertToFormat(QImage::Format_ARGB32);
            require(image.save(output + '/' + name + ".png"), "Capture failed");
            if (parser.isSet("compare")) {
                QImage before(parser.value("compare") + '/' + name + ".png");
                require(!before.isNull() && before.size() == image.size(),
                        "Baseline dimensions differ");
                qint64 changed = 0;
                for (int y = 0; y < image.height(); ++y)
                    for (int x = 0; x < image.width(); ++x)
                        if (image.pixel(x, y) != before.pixel(x, y))
                            ++changed;
                metrics["changedPixels"] = double(changed);
                metrics["totalPixels"] = image.width() * image.height();
            }
            results[name] = metrics;
            canvas.hide();
        }
        QSaveFile file(output + "/measurements.json");
        auto bytes = QJsonDocument(QJsonObject{{"profile", parser.value("profile")},
                                               {"fieldSourceHash", fields->sourceHash()},
                                               {"components", results}})
                         .toJson();
        require(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() &&
                    file.commit(),
                "Measurement write failed");
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Token probe: %s\n", e.what());
        return 1;
    }
}
