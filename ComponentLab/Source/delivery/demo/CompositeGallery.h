#pragma once
#include <QString>
class QWidget;
namespace commonWidget { class ComponentFactory; }
namespace native_gallery {
// Figma evidence canvas. Public component specimens are created only through the
// published GenerateComponent SDK Factory; internal anatomy remains reference-only.
bool hasCompositeGallery(const QString& pageId);
QWidget* createCompositeGallery(const QString& pageId,
                                const commonWidget::ComponentFactory& factory,
                                QWidget* parent);
} // namespace native_gallery
