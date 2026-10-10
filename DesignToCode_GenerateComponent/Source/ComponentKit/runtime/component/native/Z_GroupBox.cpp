#include "Z_GroupBox.h"
#include <QVariant>

namespace commonWidget {

Z_GroupBox::Z_GroupBox(QWidget* parent) : QGroupBox(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_GroupBox");
    setProperty("nativeQtClass", "QGroupBox");
}

} // namespace commonWidget
