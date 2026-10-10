#include "Z_OpenGLWidget.h"
#include <QVariant>

namespace commonWidget {

Z_OpenGLWidget::Z_OpenGLWidget(QWidget* parent) : QOpenGLWidget(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_OpenGLWidget");
    setProperty("nativeQtClass", "QOpenGLWidget");
}

} // namespace commonWidget
