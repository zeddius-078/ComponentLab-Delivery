#pragma once
#include <QString>
#include <memory>
class QWidget;
namespace commonWidget {
class ComponentFactory;
}
namespace native_gallery {
QWidget* createActionGallery(const QString& pageId,
                             const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                             QWidget* parent = nullptr);
}
