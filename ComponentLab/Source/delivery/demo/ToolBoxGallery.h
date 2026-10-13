#pragma once
#include <memory>
class QWidget;
class QToolBox;
namespace commonWidget {
class ComponentFactory;
}
namespace native_gallery {
void populateToolBoxDemo(QToolBox* box, bool sourceSnapshot = false);
QWidget* createToolBoxGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                              QWidget* parent = nullptr);
} // namespace native_gallery
