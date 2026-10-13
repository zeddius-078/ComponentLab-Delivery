#include "FigmaGalleryView.h"
#include <QGraphicsScene>
#include <QPainter>
#include <QVariant>

namespace native_gallery {
FigmaGalleryView::FigmaGalleryView(QWidget* canvas, QWidget* parent)
    : QGraphicsView(parent), designWidth(canvas->width()) {
    auto* scene = new QGraphicsScene(this);
    setScene(scene);
    scene->addWidget(canvas);
    scene->setSceneRect(0, 0, canvas->width(), canvas->height());
    setObjectName("figmaVariantGallery");
    setFrameShape(QFrame::NoFrame);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    for (const auto* key : {"galleryNodeId", "variantSpecimenCount", "componentSetCount"})
        setProperty(key, canvas->property(key));
}
void FigmaGalleryView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    const qreal factor = qMin(1., qMax(1, viewport()->width() - 4) / designWidth);
    resetTransform();
    scale(factor, factor);
}
} // namespace native_gallery
