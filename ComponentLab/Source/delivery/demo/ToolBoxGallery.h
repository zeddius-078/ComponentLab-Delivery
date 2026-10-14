#pragma once
#include <memory>
class QWidget;
namespace commonWidget {
class ComponentFactory;
}
namespace native_gallery {
QWidget* createToolBoxGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                              QWidget* parent = nullptr);
} // namespace native_gallery
