#include "Z_FontDialog.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_FontDialog::Z_FontDialog(QWidget* parent) : QFontDialog(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_FontDialog");
    setProperty("nativeQtClass", "QFontDialog");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
