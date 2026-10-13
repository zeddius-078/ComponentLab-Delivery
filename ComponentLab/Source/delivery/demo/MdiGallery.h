#pragma once
#include <memory>
class QWidget;
class QFormLayout;
class QLineEdit;
class QVBoxLayout;
class QComboBox;
namespace commonWidget {
class ComponentFactory;
}
namespace native_gallery {
QWidget* createMdiGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                          QWidget* parent);
void configureMdiEditor(QWidget* preview, QFormLayout* controls, QLineEdit* title,
                        QVBoxLayout* hostLayout, QWidget* settings, QComboBox* variant,
                        const std::shared_ptr<commonWidget::ComponentFactory>& factory);
} // namespace native_gallery
