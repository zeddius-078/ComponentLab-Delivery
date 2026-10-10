// V1 单层页面演示入口：运行时读取 DSL，调用共享引擎构造控件并输出截图/测量结果。
// 嵌套页面的静态 XML 生成与加载由 Qt/V2 提供；本入口保留单层对照用途。
#include "ComponentConverter.h"
#include "LayoutConverter.h"
#include "ComponentTheme.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QLabel>
#include <QTimer>
#include <QFontDatabase>
#include <QTextStream>
#include <QCryptographicHash>
#include <QDateTime>
#include <QFileInfo>
#include <cmath>
#include <stdexcept>

namespace {
// 读取原始字节并记下绝对路径、长度和哈希，关联本次使用的输入与可执行文件。
QByteArray readInput(const QString& path, const QString& role, QJsonObject& provenance) {
    QFile f(path);
    // 输入缺失时不能继续构造页面，同时避免为不存在内容登记来源。
    if (!f.open(QIODevice::ReadOnly))
        throw std::runtime_error(("INPUT_MISSING: " + path).toStdString());
    const auto bytes = f.readAll();
    provenance[role] = QJsonObject{
        {"path", QFileInfo(path).absoluteFilePath()},
        {"bytes", static_cast<double>(bytes.size())},
        {"sha256",
         QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex())}};
    return bytes;
}

// JSON 输入必须为对象，解析错误在创建页面前暴露。
QJsonObject load(const QString& path, const QString& role, QJsonObject& provenance) {
    const auto bytes = readInput(path, role, provenance);
    QJsonParseError e;
    auto doc = QJsonDocument::fromJson(bytes, &e);
    // 要求 JSON 为合法对象，及时拒绝损坏响应或错误输入文件。
    if (e.error != QJsonParseError::NoError || !doc.isObject())
        throw std::runtime_error("INVALID_JSON");
    return doc.object();
}

void save(const QString& path, const QJsonObject& data) {
    QFile f(path);
    // 报告不可写时明确失败，避免把上次结果当作本次成功。
    if (!f.open(QIODevice::WriteOnly))
        throw std::runtime_error("REPORT_WRITE_FAILED");
    f.write(QJsonDocument(data).toJson());
}

// V1 示例读取首个填色的 RGBA；组件自身的材质仍由既有组件实现处理。
QColor color(const QJsonObject& node) {
    const auto paints = node["fillPaints"].toArray();
    // 无填色返回无效颜色，后续保留窗口或标签默认外观。
    if (paints.isEmpty())
        return {};
    const auto c = paints[0].toObject()["color"].toObject();
    return QColor(c["r"].toInt(), c["g"].toInt(), c["b"].toInt(), qRound(c["a"].toDouble(1) * 255));
}
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({"raw", "Full original DSL file", "path"});
    parser.addOption({"mapping", "Mapping directory", "directory"});
    parser.addOption({"report", "Validation output directory", "directory"});
    parser.addOption({"font", "Explicit local font file", "path"});
    parser.addOption({"exit-after-capture", "Capture and exit"});
    parser.process(app);
    // 默认路径以 EXE 为基准，命令行可替换 DSL、映射目录、字体和报告目录。
    const auto base = QCoreApplication::applicationDirPath();
    const auto rawPath =
        parser.isSet("raw") ? parser.value("raw") : base + "/data/canonical.raw.json";
    const auto mapPath = parser.isSet("mapping") ? parser.value("mapping") : base + "/mapping";
    const auto report = parser.isSet("report") ? parser.value("report") : base + "/../../reports/V1/default";
    QDir().mkpath(report);
    QJsonObject provenance{
        {"startedAtUtc", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {"qtVersion", QString::fromLatin1(qVersion())}};
    // 构造或截图失败时写 FAIL；若异常内含结构化节点信息，一并保存在 diagnostic。
    const auto failure = [&](const std::exception& error) {
        const auto message = QString::fromUtf8(error.what());
        QJsonObject result{{"status", "FAIL"},
                           {"error", message},
                           {"source", rawPath},
                           {"provenance", provenance}};
        const auto structured = QJsonDocument::fromJson(message.toUtf8());
        // 异常若带节点化诊断就保留结构，普通文本错误仍使用 error 字段。
        if (structured.isObject())
            result["diagnostic"] = structured.object();
        try {
            save(report + "/result.json", result);
        } catch (const std::exception& writeError) {
            QTextStream(stderr) << writeError.what() << Qt::endl;
        }
        QTextStream(stderr) << message << Qt::endl;
    };
    try {
        readInput(QCoreApplication::applicationFilePath(), "executable", provenance);
        const auto font = parser.isSet("font") ? parser.value("font")
                                               : base + "/fonts/HarmonyOS_Sans_SC_Regular.ttf";
        // 必须使用指定字体完成测量，拒绝自动回退造成的几何偏差。
        if (QFontDatabase::addApplicationFontFromData(readInput(font, "font", provenance)) < 0)
            throw std::runtime_error("FONT_LOAD_FAILED");
        const auto raw = load(rawPath, "raw", provenance),
                   rules =
                       load(mapPath + "/component-mapping.json", "componentMapping", provenance);
        const auto layoutRules =
            load(mapPath + "/layout-mapping.json", "layoutMapping", provenance);
        component_theme::install(app, rules["profile"].toString());
        const auto root = raw["pixTreeNodes"].toArray()[0].toObject();
        QWidget window;
        window.setWindowTitle("DesignToCode / Raw DSL / " + root["name"].toString());
        window.resize(qRound(root["width"].toDouble()), qRound(root["height"].toDouble()));
        auto palette = window.palette();
        const auto bg = color(root);
        // 仅有有效设计填色时覆盖窗口调色板，避免无填色被解释为黑色。
        if (bg.isValid())
            palette.setColor(QPalette::Window, bg);
        window.setPalette(palette);
        window.setAutoFillBackground(true);
        d2c::ComponentConverter components(raw, rules);
        d2c::LayoutConverter layouts(layoutRules, raw);
        // 实例交给组件引擎，普通文本在此创建；其他节点类型须有明确支持后再接入。
        layouts.convert(root, &window, [&](const QJsonObject& n, QWidget* parent) -> QWidget* {
            // 实例直接进入共享组件转换器，复用契约及 Token 消费实现。
            if (n["type"] == "INSTANCE")
                return components.convert(n, parent);
            // 普通文字由 QLabel 表达，显式设置纯文本和局部样式保留设计语义。
            if (n["type"] == "PARAGRAPH") {
                auto* text = new QLabel(n["nodeText"].toString(), parent);
                text->setTextFormat(Qt::PlainText);
                text->setObjectName("node_" + n["guid"].toString().replace(':', '_'));
                text->setProperty("designNodeId", n["guid"].toString());
                const auto fontSize = n["fontSize"].toDouble(14);
                // 拒绝非有限或异常字号，防止生成无效 QSS 与不可测量的布局。
                if (!std::isfinite(fontSize) || fontSize <= 0 || fontSize > 4096)
                    throw std::runtime_error("INVALID_TEXT_SIZE");
                auto family = n["fontFamily"].toString();
                family.replace('\\', "\\\\")
                    .replace('"', "\\\"")
                    .replace('\n', ' ')
                    .replace('\r', ' ');
                QString style = QString("QLabel { font-family: \"%1\"; font-size: %2px;")
                                    .arg(family)
                                    .arg(qRound(fontSize));
                const auto fg = color(n);
                // 有有效文字填色才输出颜色规则，缺失时沿用字体默认颜色。
                if (fg.isValid())
                    style += QString(" color: rgba(%1,%2,%3,%4);")
                                 .arg(fg.red())
                                 .arg(fg.green())
                                 .arg(fg.blue())
                                 .arg(fg.alpha());
                text->setStyleSheet(style + " }");
                text->setProperty("designFontSize", qRound(fontSize));
                return text;
            }
            // 带自布局的未映射节点需要 V2 递归路径，V1 不应扁平化处理。
            if (!n["autoLayout"].toObject()["stackMode"].toString().isEmpty())
                throw std::runtime_error("NESTED_LAYOUT_V2_REQUIRED");
            throw std::runtime_error("UNMAPPED_NODE");
        });
        window.show();
        // 等窗口完成首次布局再测量；这里只证明原生构造及截图结果，未做像素相似度判断。
        QTimer::singleShot(300, &window, [&] {
            try {
                QJsonArray children;
                // 只采集根页面直接子控件作为设计节点，组件内部另列为 parts。
                for (auto* w :
                     window.findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
                    const auto r = w->geometry();
                    QJsonObject child{
                        {"nodeId", w->property("designNodeId").toString()},
                        {"qtClass", QString(w->metaObject()->className())},
                        {"geometry", QJsonArray{r.x(), r.y(), r.width(), r.height()}},
                        {"visible", w->isVisible()},
                        {"enabled", w->isEnabled()},
                        {"minimumSizeHint",
                         QJsonArray{w->minimumSizeHint().width(), w->minimumSizeHint().height()}}};
                    // 仅普通标签核对设计字号，组件内部字体由其 Token 契约管理。
                    if (auto* text = qobject_cast<QLabel*>(w)) {
                        child["fontPixelSize"] = text->font().pixelSize();
                        // 实际字体必须等于声明字号，否则样式未正确应用，停止报告成功。
                        if (text->font().pixelSize() != text->property("designFontSize").toInt())
                            throw std::runtime_error("TEXT_STYLE_MISMATCH");
                    }
                    QJsonArray parts;
                    // 逐项记录组件内部可见子部件，便于查图标、文本等内部几何。
                    for (auto* part : w->findChildren<QWidget*>()) {
                        // 跳过隐藏或无名称部件，减少内部实现细节对报告的噪音。
                        if (!part->isVisible() || part->objectName().isEmpty())
                            continue;
                        const auto rect = part->geometry();
                        parts.append(
                            QJsonObject{{"name", part->objectName()},
                                        {"geometry", QJsonArray{rect.x(), rect.y(), rect.width(),
                                                                rect.height()}},
                                        {"fontPixelSize", part->font().pixelSize()}});
                    }
                    child["parts"] = parts;
                    children.append(child);
                }
                // 截图是本次原生构造的结果文件，保存失败必须终止成功路径。
                if (!window.grab().save(report + "/native.png"))
                    throw std::runtime_error("SCREENSHOT_WRITE_FAILED");
                save(report + "/result.json", {{"status", "PASS"},
                                               {"scope", "native construction and capture"},
                                               {"source", rawPath},
                                               {"provenance", provenance},
                                               {"components", components.trace()},
                                               {"layout", layouts.trace()},
                                               {"children", children}});
                // 自动验证只运行到截图完成；交互展示不带此参数时继续事件循环。
                if (parser.isSet("exit-after-capture"))
                    app.quit();
            } catch (const std::exception& e) {
                failure(e);
                app.exit(1);
            }
        });
        return app.exec();
    } catch (const std::exception& e) {
        failure(e);
        return 1;
    }
}
