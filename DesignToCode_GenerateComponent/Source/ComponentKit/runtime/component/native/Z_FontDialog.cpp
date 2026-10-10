#include "Z_FontDialog.h"
#include <QVariant>

namespace commonWidget {

Z_FontDialog::Z_FontDialog(QWidget* parent) : QFontDialog(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_FontDialog");
    setProperty("nativeQtClass", "QFontDialog");
}

} // namespace commonWidget
