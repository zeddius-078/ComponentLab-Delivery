#include "Z_StatusBar.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_StatusBar::Z_StatusBar(QWidget* parent) : QStatusBar(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_StatusBar");
    setProperty("nativeQtClass", "QStatusBar");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
