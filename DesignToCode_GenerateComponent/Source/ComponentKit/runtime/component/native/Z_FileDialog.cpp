#include "Z_FileDialog.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_FileDialog::Z_FileDialog(QWidget* parent) : QFileDialog(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_FileDialog");
    setProperty("nativeQtClass", "QFileDialog");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
