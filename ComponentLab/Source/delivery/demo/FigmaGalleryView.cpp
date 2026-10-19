#include "FigmaGalleryView.h"
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QHideEvent>
#include <QPainter>
#include <QShowEvent>
#include <QVariant>

namespace native_gallery {
FigmaGalleryView::FigmaGalleryView(QWidget* canvas, QWidget* parent)
    : QGraphicsView(parent), designWidth(canvas->width()) {
    auto* scene = new QGraphicsScene(this);
    setScene(scene);
    // The state overview is review evidence, not an interaction surface.
    // Its real SDK widgets remain available to tests and inspectors, while
    // pointer/focus input is blocked before the first frame is presented.
    setInteractive(false);
    setFocusPolicy(Qt::NoFocus);
    viewport()->setFocusPolicy(Qt::NoFocus);
    canvas->setAttribute(Qt::WA_TransparentForMouseEvents);
    canvasProxy = scene->addWidget(canvas);
    canvasProxy->setFlag(QGraphicsItem::ItemIsFocusable, false);
    // A QGraphicsProxyWidget owns a parentless QWidget, so hiding the view does
    // not automatically hide that widget or suspend its child animations.
    // Galleries start inactive and are activated only while their tab is visible.
    canvasProxy->setVisible(false);
    scene->setSceneRect(0, 0, canvas->width(), canvas->height());
    setObjectName("figmaVariantGallery");
    setFrameShape(QFrame::NoFrame);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setProperty("visualOverviewInteractionBlocked", true);
    setProperty("liveInteractionEnabled", false);
    setProperty("animatedLoadingSpecimens", 0);
    for (const auto* key : {"galleryNodeId", "variantSpecimenCount", "componentSetCount"})
        setProperty(key, canvas->property(key));
}
void FigmaGalleryView::hideEvent(QHideEvent* event) {
    if (canvasProxy)
        canvasProxy->setVisible(false);
    QGraphicsView::hideEvent(event);
}
void FigmaGalleryView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    const qreal factor = qMin(1., qMax(1, viewport()->width() - 4) / designWidth);
    resetTransform();
    scale(factor, factor);
}
void FigmaGalleryView::showEvent(QShowEvent* event) {
    QGraphicsView::showEvent(event);
    if (canvasProxy)
        canvasProxy->setVisible(true);
}
} // namespace native_gallery
