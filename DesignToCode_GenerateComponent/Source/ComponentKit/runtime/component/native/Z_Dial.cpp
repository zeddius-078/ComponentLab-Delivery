#include "Z_Dial.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_Dial::Z_Dial(QWidget* parent) : QDial(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_Dial");
    setProperty("nativeQtClass", "QDial");
    setProperty("designSystemStyleOwner", "tokens");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
