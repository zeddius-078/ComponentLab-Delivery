#pragma once
#include <memory>
class QWidget;
class QTabWidget;
namespace commonWidget {
class ComponentFactory;
}
namespace native_gallery {
void populateTabsDemo(QTabWidget* tabs, bool sourceSnapshot = false);
void setTabsDemoClosePolicy(QTabWidget* tabs, bool closable, bool behaviorOnly);
QWidget* createTabsGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                           QWidget* parent = nullptr);
} // namespace native_gallery
