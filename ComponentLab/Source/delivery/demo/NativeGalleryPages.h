#pragma once

#include <QString>
#include <QMap>

class QWidget;

namespace native_gallery {

QWidget* createGallery(const QString& profile, const QMap<QString, QWidget*>& detailedPages,
                       QWidget* parent = nullptr);
QWidget* createRegistry(QWidget* parent = nullptr);

} // namespace native_gallery
