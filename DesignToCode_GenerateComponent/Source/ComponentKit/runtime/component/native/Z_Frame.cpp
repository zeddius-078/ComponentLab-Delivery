#include "Z_Frame.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_Frame::Z_Frame(QWidget* parent) : QFrame(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_Frame");
    setProperty("nativeQtClass", "QFrame");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
