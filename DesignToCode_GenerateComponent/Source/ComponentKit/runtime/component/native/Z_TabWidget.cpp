#include "Z_TabWidget.h"
#include <QVariant>

namespace commonWidget {

Z_TabWidget::Z_TabWidget(QWidget* parent) : QTabWidget(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_TabWidget");
    setProperty("nativeQtClass", "QTabWidget");
}

} // namespace commonWidget
