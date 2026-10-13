#pragma once
#include <memory>
class QWidget;
class QFormLayout;
class QLineEdit;
class QVBoxLayout;
class QComboBox;
class QString;
namespace commonWidget { class ComponentFactory; }
namespace native_gallery {
QWidget* createPageFlowPreview(const QString& sdkClass,const std::shared_ptr<commonWidget::ComponentFactory>& factory,QWidget* parent);
QWidget* createPageFlowGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,QWidget* parent);
void configurePageFlowEditor(QWidget* preview,QFormLayout* controls,QLineEdit* title,QVBoxLayout* hostLayout,QWidget* settings,QComboBox* variant,const std::shared_ptr<commonWidget::ComponentFactory>& factory);
}
