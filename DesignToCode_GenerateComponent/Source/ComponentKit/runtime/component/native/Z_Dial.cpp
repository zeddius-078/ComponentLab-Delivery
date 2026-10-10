#include "Z_Dial.h"
#include <QVariant>

namespace commonWidget {

Z_Dial::Z_Dial(QWidget* parent) : QDial(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_Dial");
    setProperty("nativeQtClass", "QDial");
}

} // namespace commonWidget
