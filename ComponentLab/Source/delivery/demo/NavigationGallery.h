#pragma once
#include <QString>
class QWidget;
namespace native_gallery {
bool hasNavigationGallery(const QString& pageId);
QWidget* createNavigationAppearance(const QString& pageId, QWidget* parent,
                                    const QString& profile = QStringLiteral("standard-light"));
QWidget* createNavigationGallery(const QString& pageId, QWidget* parent);
} // namespace native_gallery
