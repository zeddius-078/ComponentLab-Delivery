#include "Z_FileDialog.h"
#include <QVariant>

namespace commonWidget {

Z_FileDialog::Z_FileDialog(QWidget* parent) : QFileDialog(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_FileDialog");
    setProperty("nativeQtClass", "QFileDialog");
}

} // namespace commonWidget
