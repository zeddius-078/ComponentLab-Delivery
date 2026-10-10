#include "Z_Widget.h"
#include <QVariant>

namespace commonWidget {

Z_Widget::Z_Widget(QWidget* parent) : QWidget(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_Widget");
    setProperty("nativeQtClass", "QWidget");
}

} // namespace commonWidget
