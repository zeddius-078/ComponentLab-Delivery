#pragma once
#include <QString>
#include <memory>
class QWidget;
namespace commonWidget {
class ComponentFactory;
}
namespace native_gallery {
QWidget* createDisplayGallery(const QString& pageId,
                             const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                             QWidget* parent);
}
