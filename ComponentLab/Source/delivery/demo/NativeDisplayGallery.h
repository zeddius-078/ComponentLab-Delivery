#pragma once
#include <QString>
#include <memory>
class QWidget;
namespace commonWidget {
class ComponentFactory;
}
namespace native_gallery {
// Native-widget galleries for the remaining instantiable 20.2 Display pages: real
// Z_LCDNumber, Z_TextBrowser, Z_GraphicsView, Z_CalendarWidget, Z_TextEdit,
// Z_PlainTextEdit and Z_OpenGLWidget specimens at captured source geometry.
bool hasNativeDisplayGallery(const QString& pageId);
QWidget* createNativeDisplayGallery(
    const QString& pageId, const std::shared_ptr<commonWidget::ComponentFactory>& factory,
    QWidget* parent);
} // namespace native_gallery
