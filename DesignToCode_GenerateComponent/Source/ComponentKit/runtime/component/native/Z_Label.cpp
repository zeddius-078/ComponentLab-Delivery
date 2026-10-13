#include "Z_Label.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_Label::Z_Label(QWidget* parent) : QLabel(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_Label");
    setProperty("nativeQtClass", "QLabel");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
