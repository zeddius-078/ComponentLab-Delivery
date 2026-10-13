#pragma once

#include <QList>
#include <QRect>
#include <QString>

namespace native_gallery {

struct FigmaVariantLayout {
    QString componentSetId;
    QString componentSetName;
    QString nodeId;
    QString variant;
    QRect rect;
};

struct FigmaGalleryLayout {
    QString pageId;
    QString galleryId;
    QSize size;
    int componentSetCount = 0;
    QList<FigmaVariantLayout> variants;
};

FigmaGalleryLayout figmaGalleryLayout(const QString& pageId);
QString figmaAxisValue(const QString& variant, const QString& axis);

} // namespace native_gallery
