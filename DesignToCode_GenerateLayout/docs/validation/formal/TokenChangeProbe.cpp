// 验收程序，不进入页面交付包：使用公开 SDK 注入完整生成目录并测量真实按钮像素。
#include "ReviewPage.h"
#include <ComponentTheme.h>
#include <TokenResources.h>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <QTextStream>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    if (argc != 3) return 2;
    try {
        token_resources::setGeneratedRoot(QString::fromLocal8Bit(argv[1]));
        component_theme::install(app, "wide-light");
        ReviewPage page;
        page.show();
        QTest::qWait(100);
        auto* button = page.getConfirmReviewButton();
        const auto pixmap = button->grab();
        if (!pixmap.save(QString::fromLocal8Bit(argv[2]))) return 3;
        const auto image = pixmap.toImage();
        const auto pixel = image.pixelColor(qRound(5 * pixmap.devicePixelRatio()), image.height() / 2);
        const QJsonObject result{{"profile", component_theme::activeProfile()},
            {"buttonHeight", button->height()}, {"buttonWidth", button->width()},
            {"pixel", pixel.name(QColor::HexRgb)}, {"enabled", button->isEnabled()}};
        QTextStream(stdout) << QJsonDocument(result).toJson(QJsonDocument::Compact) << Qt::endl;
        return 0;
    } catch (const std::exception& error) {
        QTextStream(stderr) << error.what() << Qt::endl;
        return 1;
    }
}
