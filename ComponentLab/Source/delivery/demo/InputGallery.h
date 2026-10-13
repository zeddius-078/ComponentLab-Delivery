#pragma once
#include <memory>
class QWidget;
namespace commonWidget {
class ComponentFactory;
}
namespace native_gallery {
QWidget* createInputGallery(const std::shared_ptr<commonWidget::ComponentFactory>&,
                            QWidget* parent = nullptr);
}
