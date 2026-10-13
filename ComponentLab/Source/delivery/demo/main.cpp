#include "DemoWindow.h"
#include "DemoRuntime.h"
#include "TokenResources.h"
#include "FocusIndicator.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QStyleFactory>
#include <QScreen>
#include <QTimer>
#include <cstdio>

int main(int argc, char** argv) {
    qunsetenv("QT_PLUGIN_PATH");
    qunsetenv("QT_QPA_PLATFORM_PLUGIN_PATH");
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    app.setApplicationName("Component Lab");
    QCommandLineParser p;
    p.addHelpOption();
    p.addOption({"profile", "Token Profile", "name", "light-comfortable"});
    p.addOption({"appearance", "Initial surface preset", "id", "solid"});
    p.addOption({"focus-indicator", "Focus feedback: off, keyboard or always", "mode"});
    p.addOption({"capture-dir", "Capture all tabs", "path"});
    p.addOption({"capture-page", "Capture a single Figma page and its native gallery canvas", "node-id"});
    p.addOption(
        {"token-dir", "Complete generated Token directory (verification override)", "path"});
    p.addOption({"quit-after", "Exit for verification", "ms"});
    p.process(app);
    try {
        token_resources::setGeneratedRoot(p.value("token-dir"));
        button_demo::install(app, p.value("profile"));
        if (p.isSet("focus-indicator"))
            commonWidget::FocusIndicator::instance().setMode(
                commonWidget::FocusIndicator::parseMode(p.value("focus-indicator")));
        DemoWindow window(p.value("profile"));
        if (const auto* screen = window.screen()) {
            const auto available = screen->availableGeometry().size() - QSize(48, 80);
            window.resize(window.size().boundedTo(available.expandedTo(QSize(640, 480))));
        }
        if (p.isSet("appearance"))
            window.setAppearancePreview(p.value("appearance"));
        window.show();
        if (p.isSet("capture-dir"))
            QTimer::singleShot(1000, &window, [&] {
                try {
                    window.capture(p.value("capture-dir"), p.value("capture-page"));
                } catch (const std::exception& e) {
                    std::fprintf(stderr, "Capture: %s\n", e.what());
                    app.exit(2);
                }
            });
        if (p.isSet("quit-after")) {
            bool ok;
            int ms = p.value("quit-after").toInt(&ok);
            if (!ok || ms < 1500)
                throw std::invalid_argument("quit-after must be >=1500ms");
            QTimer::singleShot(ms, &app, &QCoreApplication::quit);
        }
        return app.exec();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Startup: %s\n", e.what());
        if (!p.isSet("quit-after"))
            QMessageBox::critical(nullptr, "Startup", QString::fromUtf8(e.what()));
        return 1;
    }
}
