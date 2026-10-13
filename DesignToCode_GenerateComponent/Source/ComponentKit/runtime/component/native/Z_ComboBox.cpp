#include "Z_ComboBox.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_ComboBox::Z_ComboBox(QWidget* parent) : QComboBox(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ComboBox");
    setProperty("nativeQtClass", "QComboBox");
    setProperty("designSystemStyleOwner", "tokens");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
