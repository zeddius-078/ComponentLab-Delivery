#pragma once
#include <QString>
#include <memory>
class QWidget;
namespace commonWidget {
class ComponentFactory;
}
namespace native_gallery {
// Native-widget galleries for the instantiable 20.6 Container pages: real Z_Splitter,
// Z_GroupBox, Z_ScrollArea, Z_Widget and Z_Frame specimens at captured source geometry.
bool hasContainersGallery(const QString& pageId);
QWidget* createContainersGallery(const QString& pageId,
                                 const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                                 QWidget* parent);
} // namespace native_gallery
