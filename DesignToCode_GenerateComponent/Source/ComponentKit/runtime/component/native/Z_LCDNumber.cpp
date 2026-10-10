#include "Z_LCDNumber.h"
#include <QVariant>

namespace commonWidget {

Z_LCDNumber::Z_LCDNumber(QWidget* parent) : QLCDNumber(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_LCDNumber");
    setProperty("nativeQtClass", "QLCDNumber");
}

} // namespace commonWidget
