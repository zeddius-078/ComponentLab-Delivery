#include "Z_Dialog.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_Dialog::Z_Dialog(QWidget* parent) : QDialog(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_Dialog");
    setProperty("nativeQtClass", "QDialog");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
