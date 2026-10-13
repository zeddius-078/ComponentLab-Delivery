#pragma once
#include <memory>
class QWidget;
namespace commonWidget {
class ComponentFactory;
}
namespace native_gallery {
QWidget* createButtonGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                             QWidget* parent);
}
