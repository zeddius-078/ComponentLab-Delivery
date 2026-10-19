#include "CaseWindow.h"
#include "ComponentTheme.h"
#include "JsonIO.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QMessageBox>
#include <QTimer>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({"verify", "Run checks and exit; write report here", "directory"});
    parser.process(app);
    const auto data = QCoreApplication::applicationDirPath() + "/data";
    try {
        // 部署字体和参数均为必需输入，缺失时停止而非使用不确定默认值。
        if (QFontDatabase::addApplicationFont(data + "/HarmonyOS_Sans_SC_Regular.ttf") < 0)
            throw std::runtime_error("FONT_LOAD_FAILED");
        component_theme::install(app,
                             v2::readJson(data + "/page.components.json")["profile"].toString());
        CaseWindow window(data);
        window.show();
        // 检查模式等待布局稳定再采集几何和执行鼠标操作。
        if (parser.isSet("verify")) {
            const auto report = parser.value("verify");
            QTimer::singleShot(300, &app, [&app, &window, report] {
                QJsonObject result;
                try {
                    result = window.verify(report);
                } catch (const std::exception& e) {
                    result = {{"status", "FAIL"}, {"error", e.what()}};
                }
                QDir().mkpath(report);
                QFile file(report + "/qt-result.json");
                // 无法写出结果也必须返回失败，避免沿用上次的通过报告。
                if (!file.open(QIODevice::WriteOnly)) {
                    app.exit(2);
                    return;
                }
                file.write(QJsonDocument(result).toJson());
                app.exit(result["status"] == "PASS" ? 0 : 1);
            });
        }
        return app.exec();
    } catch (const std::exception& e) {
        qCritical("%s", e.what());
        // 自动检查时避免弹出阻塞对话框；交互启动时给出实际错误原因。
        if (!parser.isSet("verify"))
            QMessageBox::critical(nullptr, "27:450", QString::fromUtf8(e.what()));
        return 1;
    }
}
