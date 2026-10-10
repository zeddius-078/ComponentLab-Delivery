#include "Z_StackedWidget.h"
#include <QVariant>

namespace commonWidget {

Z_StackedWidget::Z_StackedWidget(QWidget* parent) : QStackedWidget(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_StackedWidget");
    setProperty("nativeQtClass", "QStackedWidget");
}

} // namespace commonWidget
