#pragma once
#include <memory>
class QWidget;
class QString;
namespace commonWidget {
class ComponentFactory;
}
namespace native_gallery {
QWidget* createRadioGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                            QWidget* parent);
// Demo composition only: native QButtonGroup + project-owned radio identities.
QWidget* createRadioGroupPreview(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                                 bool vertical, bool enabled, bool snapshot, QWidget* parent);
} // namespace native_gallery
