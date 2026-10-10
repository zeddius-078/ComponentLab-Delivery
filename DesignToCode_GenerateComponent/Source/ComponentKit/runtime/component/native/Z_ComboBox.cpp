#include "Z_ComboBox.h"
#include <QVariant>

namespace commonWidget {

Z_ComboBox::Z_ComboBox(QWidget* parent) : QComboBox(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ComboBox");
    setProperty("nativeQtClass", "QComboBox");
}

} // namespace commonWidget
