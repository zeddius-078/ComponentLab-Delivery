#include "TabsGallery.h"
#include "TabAppearance.h"
#include "ComponentFactory.h"
#include "FigmaGalleryView.h"
#include <QFile>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QTabBar>
#include <QTabWidget>
#include <functional>
#include <stdexcept>

namespace native_gallery {
namespace {
QRect bounds(const QJsonArray& n) {
    return QRect(qRound(n[3].toDouble()), qRound(n[4].toDouble()), qRound(n[5].toDouble()),
                 qRound(n[6].toDouble()));
}
QLabel* label(QWidget* parent, const QString& text, const QRect& rect, int size, int weight,
              const QString& color) {
    auto* result = new QLabel(text, parent);
    result->setTextFormat(Qt::PlainText);
    result->setGeometry(rect);
    result->setWordWrap(true);
    result->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    QFont font("HarmonyOS Sans");
    font.setPixelSize(size);
    font.setWeight(weight);
    result->setFont(font);
    result->setStyleSheet(
        QString("background:transparent;border:0;color:%1;font-family:'HarmonyOS Sans';font-size:%2px;")
            .arg(color)
            .arg(size));
    return result;
}
} // namespace
QWidget* createTabsGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                           QWidget* parent) {
    QFile file(":/figma/gallery-layouts.json");
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Gallery evidence missing");
    QJsonArray gallery;
    for (const auto& item : QJsonDocument::fromJson(file.readAll()).object()["pages"].toArray())
        if (item.toObject()["id"] == "500:2")
            gallery = item.toObject()["galleries"].toArray()[0].toArray();
    QFontDatabase::addApplicationFont(":/figma/fonts/Inter-Medium.otf");
    QFontDatabase::addApplicationFont(":/figma/fonts/Inter-SemiBold.otf");
    QFontDatabase::addApplicationFont(":/figma/fonts/Inter-Regular.otf");
    auto* canvas = new QWidget;
    canvas->setObjectName("figmaGalleryCanvas");
    canvas->setFixedSize(bounds(gallery).size());
    canvas->setProperty("galleryNodeId", "500:29");
    canvas->setProperty("variantSpecimenCount", 20);
    canvas->setProperty("componentSetCount", 1);
    canvas->setStyleSheet("QWidget#figmaGalleryCanvas {background:white;border-radius:16px;}");
    std::function<void(const QJsonArray&, QWidget*)> build = [&](const QJsonArray& node,
                                                                 QWidget* host) {
        const auto id = node[0].toString(), name = node[1].toString();
        if (node[2] == "COMPONENT") {
            auto* tabs =
                qobject_cast<QTabWidget*>(factory->create("commonWidget::Z_TabWidget", host));
            if (!tabs)
                throw std::runtime_error("Native TabWidget unavailable");
            const bool anatomy = id != "500:849";
            if (anatomy) {
                tabs->addTab(new QWidget, "Overview");
                const auto position = name.section(", ", 0, 0).section('=', 1),
                           state = name.section(", ", 1, 1).section('=', 1);
                tabs->setTabPosition(position == "Bottom"  ? QTabWidget::South
                                     : position == "Left"  ? QTabWidget::West
                                     : position == "Right" ? QTabWidget::East
                                                           : QTabWidget::North);
                tabs->setTabEnabled(0, state != "Disabled");
                commonWidget::applyFigmaTabWidgetAppearance(tabs, state, 0, true, true);
            } else {
                commonWidget::populateFigmaTabWidget(tabs, true);
            }
            tabs->setProperty("figmaNodeId", id);
            tabs->setProperty("figmaVariant", name);
            tabs->setProperty("figmaDesignBounds", bounds(node));
            tabs->setFixedSize(bounds(node).size());
            tabs->move(bounds(node).topLeft());
            tabs->tabBar()->setFocusPolicy(Qt::NoFocus);
            tabs->setAttribute(Qt::WA_TransparentForMouseEvents);
            return;
        }
        if (id == "500:878") {
            auto* bar = new QTabBar(host);
            bar->addTab("One");
            bar->addTab("Two");
            bar->addTab("Three");
            bar->setCurrentIndex(0);
            bar->setUsesScrollButtons(true);
            bar->setElideMode(Qt::ElideNone);
            commonWidget::applyFigmaTabBarAppearance(bar, {}, -1, true);
            bar->setGeometry(bounds(node));
            bar->setProperty("figmaNodeId", id);
            bar->setFocusPolicy(Qt::NoFocus);
            bar->setAttribute(Qt::WA_TransparentForMouseEvents);
            return;
        }
        if (node[2] == "TEXT") {
            const int size = id == "500:876" ? 16 : id == "500:875" || id == "500:898" ? 11 : 13;
            auto* text = label(host, node[7].toString(), bounds(node), size,
                               id == "500:746" || id == "500:898"   ? QFont::Medium
                               : id == "500:875" || id == "500:876" ? QFont::DemiBold
                                                                    : QFont::Normal,
                               id == "500:875"   ? "#6235f7"
                               : id == "500:898" ? "#8a8a8a"
                                                 : "#191919");
            text->setProperty("figmaNodeId", id);
            if (id != "500:877") {
                // Source captions are whitespace-nowrap, with no clipping text box.
                text->setWordWrap(false);
                text->ensurePolished();
                text->resize(qMax(text->width(), text->fontMetrics().horizontalAdvance(text->text()) + 1), text->height());
            }
            return;
        }
        auto* group = new QWidget(host);
        group->setGeometry(bounds(node));
        group->setProperty("figmaNodeId", id);
        group->setObjectName("tabsGroup_" + QString(id).replace(':', '_'));
        if (node[2] == "COMPONENT_SET")
            group->setProperty("figmaComponentSetId", id);
        else if (id == "500:745" || id == "500:848")
            group->setStyleSheet(QString("QWidget#%1 {background:#f3f3f3;border-radius:%2px;}")
                                     .arg(group->objectName())
                                     .arg(id == "500:745" ? 10 : 12));
        else if (id == "500:874")
            group->setStyleSheet(
                QString(
                    "QWidget#%1 {background:white;border:1px solid #d0d0d0;border-radius:10px;}")
                    .arg(group->objectName()));
        for (const auto& child : node[7].toArray())
            build(child.toArray(), group);
    };
    for (const auto& child : gallery[7].toArray())
        build(child.toArray(), canvas);
    return new FigmaGalleryView(canvas, parent);
}
} // namespace native_gallery
