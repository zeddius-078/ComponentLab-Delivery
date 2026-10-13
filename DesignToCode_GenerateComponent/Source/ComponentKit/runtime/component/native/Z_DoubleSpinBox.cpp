#include "Z_DoubleSpinBox.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_DoubleSpinBox::Z_DoubleSpinBox(QWidget* parent) : QDoubleSpinBox(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_DoubleSpinBox");
    setProperty("nativeQtClass", "QDoubleSpinBox");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
