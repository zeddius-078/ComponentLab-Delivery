#pragma once
#include <QString>
#include <memory>
class QWidget;
namespace commonWidget {
class ComponentFactory;
}
namespace native_gallery {
// Native-widget galleries for 20.9 Window & System pages: real Z_* dialog/status
// specimens placed at the captured Figma canvas geometry. The Figma mockup internals
// (authored texts/colors) are not reproduced pixel-for-pixel; the source itself
// declares platform-native ownership for these surfaces.
bool hasWindowsGallery(const QString& pageId);
QWidget* createWindowsGallery(const QString& pageId,
                              const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                              QWidget* parent);
} // namespace native_gallery
