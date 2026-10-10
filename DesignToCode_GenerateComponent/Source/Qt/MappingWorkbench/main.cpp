#include "ComponentTheme.h"
#include "MappingGuide.h"
#include "MappingPage.h"
#include "QtMapping.h"
#include "TokenResources.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QTimer>
#include <QDir>
#include <stdexcept>

int main(int argc, char **argv) {
    QApplication application(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({"profile", "Private Token profile", "name", "light-comfortable"});
    parser.addOption({"token-dir", "Private generated Token directory", "path"});
    parser.addOption({"mapping-input", "Normalized UI Contract JSON", "path"});
    parser.addOption({"mapping-export", "Export into a new child directory", "parent"});
    parser.addOption({"capture-dir", "Capture the three component mapping tabs", "path"});
    parser.addOption({"quit-after", "Exit after milliseconds", "ms"});
    parser.process(application);
    try {
        const auto profile = parser.value("profile");
        token_resources::setGeneratedRoot(parser.value("token-dir"));
        component_theme::install(application, profile);
        // 设计导出在本程序执行；组件演示入口只负责组件自身的行为。
        if (parser.isSet("mapping-export")) {
            const auto contract = parser.isSet("mapping-input")
                ? design_mapping::parse(design_mapping::read(parser.value("mapping-input")))
                : design_mapping::example();
            qInfo("%s", qPrintable(design_mapping::exportFiles(contract, profile, parser.value("mapping-export"))));
            return 0;
        }
        if (parser.isSet("mapping-input"))
            throw std::invalid_argument("mapping-input requires mapping-export");
        QTabWidget window;
        window.setWindowTitle("DesignToCode_GenerateComponent | Component Mapping");
        window.resize(1200, 800);
        auto *guide = new QWidget(&window);
        auto *flow = new QWidget(&window);
        auto *files = new QWidget(&window);
        // 三个视图仅属于设计转换演示；组件状态演示留在独立的 ComponentLab。
        for (auto *page : {guide, flow, files})
            new QVBoxLayout(page);
        window.addTab(guide, QStringLiteral("方案与设计源"));
        window.addTab(flow, QStringLiteral("流程与预览"));
        window.addTab(files, QStringLiteral("契约与产物"));
        design_mapping::populateGuide(guide);
        new design_mapping::MappingPage(profile, flow, files, &window);
        window.show();
        // 截图与定时退出只供组件映射验证，逐页覆盖三个工作台视图。
        if (parser.isSet("capture-dir")) {
            QTimer::singleShot(800, &window, [&] {
                QDir directory(parser.value("capture-dir"));
                if (!directory.mkpath(".")) {
                    application.exit(2);
                    return;
                }
                // 逐页触发布局和重绘，保存实际窗口画面。
                for (int index = 0; index < window.count(); ++index) {
                    window.setCurrentIndex(index);
                    application.processEvents();
                    if (!window.grab().save(directory.filePath(QString("tab-%1.png").arg(index))))
                        application.exit(2);
                }
            });
        }
        if (parser.isSet("quit-after")) {
            bool valid = false;
            const auto timeout = parser.value("quit-after").toInt(&valid);
            if (!valid || timeout < 1500)
                throw std::invalid_argument("quit-after must be >= 1500ms");
            QTimer::singleShot(timeout, &application, &QCoreApplication::quit);
        }
        return application.exec();
    } catch (const std::exception &error) {
        qCritical("%s", error.what());
        return 1;
    }
}
