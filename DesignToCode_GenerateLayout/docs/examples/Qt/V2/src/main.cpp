// V2 桌面入口：载入离线 data、字体和主线 Token 样式，再打开工单流程或仪表空窗口。
#include "WorkflowWindow.h"
#include "ComponentTheme.h"
#include "Z_InputField.h"
#include "Z_PushButton.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QFontDatabase>
#include <QMessageBox>
#include <QTextStream>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <stdexcept>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({"data", "Input directory", "directory"});
    parser.addOption({"result", "Static result form prefix without extension", "path"});
    parser.addOption({"instrument-host", "Open the empty XML instrument extension window"});
    parser.addOption({"page", "Open one generated page prefix (point UI) without flow logic", "prefix"});
    parser.addOption({"smoke-report", "Run deterministic deployment check and exit", "directory"});
    parser.process(app);
    try {
        // 默认按 EXE 所在目录寻找 data，保证从其他工作目录启动也可定位资源。
        const auto base = QCoreApplication::applicationDirPath();
        const auto data =
            parser.isSet("data") ? parser.value("data") : base + QString(D2C_DEFAULT_DATA);
        // 离线包字体决定排版，加载失败时报告依赖缺失，避免系统字体替代造成假通过。
        if (QFontDatabase::addApplicationFont(data + "/HarmonyOS_Sans_SC_Regular.ttf") < 0)
            throw std::runtime_error("FONT_LOAD_FAILED");
        component_theme::install(app,
                             v2::readJson(data + "/review.components.json")["profile"].toString());
        // 仪表空窗口与工单流程分开启动，便于验证扩展宿主而不触发业务路由。
        if (parser.isSet("instrument-host")) {
            // 单独展示 XML 中的空宿主，具体仪表可通过注册表和挂载接口补充。
            // 仪表展示不执行工单自动流程，两种参数同时存在时明确拒绝。
            if (parser.isSet("smoke-report"))
                throw std::runtime_error("INCOMPATIBLE_DEMO_ARGUMENTS");
            auto registry = std::make_shared<v2::WidgetRegistry>("standard-light");
            v2::UiPage host(data + "/instrument_host", registry);
            std::unique_ptr<QWidget> window(host.load());
            window->setWindowTitle("仪表扩展 / XML 容器");
            window->show();
            return app.exec();
        }
        // 单页点位视图：按组件库配置展示同一设计的不同映射结果，不执行业务流程。
        if (parser.isSet("page")) {
            if (parser.isSet("smoke-report") || parser.isSet("result"))
                throw std::runtime_error("INCOMPATIBLE_DEMO_ARGUMENTS");
            const auto prefix = data + "/" + parser.value("page");
            const auto profile =
                v2::readJson(prefix + ".components.json")["profile"].toString();
            auto registry = std::make_shared<v2::WidgetRegistry>(profile);
            v2::UiPage point(prefix, registry);
            std::unique_ptr<QWidget> window(point.load());
            window->setWindowTitle(QStringLiteral("点位视图 / %1 / %2")
                                       .arg(parser.value("page"), profile));
            window->show();
            return app.exec();
        }
        v2::WorkflowWindow window(data, parser.value("result"));
        window.show();
        // 部署检查自动输入工单并跳转，写截图、节点状态及文件哈希后退出。
        // 等待首轮事件处理后截图，验证本机部署结果；不代表其他设备上的实测。
        if (parser.isSet("smoke-report"))
            QTimer::singleShot(200, &window, [&] {
                QJsonObject result;
                int exitCode = 0;
                const auto report = parser.value("smoke-report");
                QDir().mkpath(report);
                try {
                    const auto flow = v2::readJson(data + "/workflow.json");
                    auto* input = qobject_cast<commonWidget::Z_InputField*>(
                        window.scene(false).widget(flow["input"].toString()));
                    auto* confirm = qobject_cast<commonWidget::Z_PushButton*>(
                        window.scene(false).widget(flow["confirm"].toString()));
                    input->setValue("WO-2026-001");
                    confirm->click();
                    // 只有跳到结果页且保存了输入值，才能证明部署环境下的确认流程有效。
                    if (window.currentIndex() != 1 || window.workOrder() != "WO-2026-001")
                        throw std::runtime_error("DEPLOY_FLOW_FAILED");
                    // 截图前清除焦点，减少焦点环对不同运行截图的干扰。
                    if (qApp->focusWidget())
                        qApp->focusWidget()->clearFocus();
                    qApp->processEvents();
                    // 截图写入失败必须反映为检查失败，避免报告缺少对应画面。
                    if (!window.grab().save(report + "/native-result.png"))
                        throw std::runtime_error("CAPTURE_FAILED");
                    result = window.scene(true).trace();
                    result["status"] = "PASS";
                    result["qtVersion"] = QString(qVersion());
                    result["platform"] = QGuiApplication::platformName();
                    result["events"] = window.events();
                    // 绑定本次 EXE 和加载资源，检查工具据此识别过期部署或文件混用。
                    QJsonObject fingerprints;
                    QStringList files{"review.ui",     "review.components.json",       "review.qss",
                                      "result.ui",     "result.components.json",       "result.qss",
                                      "workflow.json", "HarmonyOS_Sans_SC_Regular.ttf"};
                    // 对每个运行必需文件取哈希，把测试结果绑定到本次实际加载的资源版本。
                    for (const auto& name : files) {
                        QFile f(data + "/" + name);
                        // 缺失任何部署输入就停止指纹采集，防止以空内容的哈希通过。
                        if (!f.open(QIODevice::ReadOnly))
                            throw std::runtime_error("FINGERPRINT_INPUT_MISSING");
                        fingerprints[name] = QString::fromLatin1(
                            QCryptographicHash::hash(f.readAll(), QCryptographicHash::Sha256)
                                .toHex());
                    }
                    QFile exe(QCoreApplication::applicationFilePath());
                    exe.open(QIODevice::ReadOnly);
                    fingerprints["executable"] = QString::fromLatin1(
                        QCryptographicHash::hash(exe.readAll(), QCryptographicHash::Sha256)
                            .toHex());
                    result["fingerprints"] = fingerprints;
                } catch (const std::exception& e) {
                    result = {{"status", "FAIL"}, {"error", QString::fromUtf8(e.what())}};
                    exitCode = 1;
                }
                QFile f(report + "/result.json");
                // 报告写不出时返回失败码，防止自动检查把没有结果的运行认作成功。
                if (!f.open(QIODevice::WriteOnly))
                    exitCode = 1;
                else
                    f.write(QJsonDocument(result).toJson());
                app.exit(exitCode);
            });
        return app.exec();
    } catch (const std::exception& e) {
        // 自动检查通过退出码反馈，交互启动额外显示错误窗口。
        QTextStream(stderr) << e.what() << Qt::endl;
        // 自动模式通过退出码和日志反馈；交互模式显示错误窗口便于用户定位。
        if (!parser.isSet("smoke-report"))
            QMessageBox::critical(nullptr, "V2", QString::fromUtf8(e.what()));
        return 1;
    }
}
