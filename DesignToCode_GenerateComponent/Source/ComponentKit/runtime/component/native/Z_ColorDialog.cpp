#include "Z_ColorDialog.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_ColorDialog::Z_ColorDialog(QWidget* parent) : QColorDialog(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ColorDialog");
    setProperty("nativeQtClass", "QColorDialog");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
