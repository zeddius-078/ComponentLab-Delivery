#include "Z_SpinBox.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_SpinBox::Z_SpinBox(QWidget* parent) : QSpinBox(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_SpinBox");
    setProperty("nativeQtClass", "QSpinBox");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
