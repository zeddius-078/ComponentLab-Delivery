#include "BaselineGallery.h"

#include <MyDesignSystem/MyDesignSystem.h>

#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("Component Lab"));
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({QStringLiteral("capture-dir"), QStringLiteral("保存基线图库截图"),
                      QStringLiteral("path")});
    parser.addOption({QStringLiteral("quit-after"), QStringLiteral("验证时自动退出"),
                      QStringLiteral("ms")});
    parser.process(app);

    commonWidget::initializeNativeComponentResources();
    app.setStyleSheet(commonWidget::nativeComponentStyleSheet() +
                      baselineGalleryCanvasStyleSheet());
    BaselineWindow window;
    window.show();
    if (parser.isSet(QStringLiteral("capture-dir"))) {
        QTimer::singleShot(350, &window, [&] {
            const bool captured = window.capture(parser.value(QStringLiteral("capture-dir")));
            if (!captured)
                app.exit(2);
            // A verification runner also needs a live process long enough to
            // inspect its loaded Qt plugins. Interactive one-shot capture
            // remains backwards-compatible when no quit timer is requested.
            else if (!parser.isSet(QStringLiteral("quit-after")))
                app.exit(0);
        });
    }
    if (parser.isSet(QStringLiteral("quit-after"))) {
        bool valid = false;
        const int milliseconds = parser.value(QStringLiteral("quit-after")).toInt(&valid);
        if (!valid || milliseconds < 100)
            return 2;
        QTimer::singleShot(milliseconds, &app, &QCoreApplication::quit);
    }
    return app.exec();
}
