#include "Z_InputDialog.h"
#include <QVariant>

namespace commonWidget {

Z_InputDialog::Z_InputDialog(QWidget* parent) : QInputDialog(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_InputDialog");
    setProperty("nativeQtClass", "QInputDialog");
}

} // namespace commonWidget
