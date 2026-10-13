#pragma once
#include <memory>
#include <QString>
class QWidget;
class QDockWidget;
class QFormLayout;
class QLineEdit;
class QVBoxLayout;
class QComboBox;
namespace commonWidget {
class ComponentFactory;
}
namespace native_gallery {
void populateDockWidgetDemo(QDockWidget* dock, const QString& caption);
void configureDockWidgetEditor(QDockWidget* dock, QFormLayout* controls, QLineEdit* title,
                               QVBoxLayout* hostLayout, QWidget* settings, QComboBox* variant);
QWidget* createDockWidgetGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                                 QWidget* parent);
} // namespace native_gallery
