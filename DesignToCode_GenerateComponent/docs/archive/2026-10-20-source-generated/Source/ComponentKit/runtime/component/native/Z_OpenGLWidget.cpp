#include "Z_OpenGLWidget.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_OpenGLWidget::Z_OpenGLWidget(QWidget* parent) : QOpenGLWidget(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_OpenGLWidget");
    setProperty("nativeQtClass", "QOpenGLWidget");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
