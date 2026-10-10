#include "Z_MessageBox.h"
#include <QVariant>

namespace commonWidget {

Z_MessageBox::Z_MessageBox(QWidget* parent) : QMessageBox(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_MessageBox");
    setProperty("nativeQtClass", "QMessageBox");
}

} // namespace commonWidget
