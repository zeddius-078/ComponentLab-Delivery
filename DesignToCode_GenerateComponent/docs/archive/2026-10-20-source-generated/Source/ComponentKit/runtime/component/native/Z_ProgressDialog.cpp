#include "Z_ProgressDialog.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_ProgressDialog::Z_ProgressDialog(QWidget* parent) : QProgressDialog(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ProgressDialog");
    setProperty("nativeQtClass", "QProgressDialog");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
