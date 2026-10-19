#include "Z_RadioButton.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_RadioButton::Z_RadioButton(QWidget* parent) : QRadioButton(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_RadioButton");
    setProperty("nativeQtClass", "QRadioButton");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
