#include "Z_DialogButtonBox.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_DialogButtonBox::Z_DialogButtonBox(QWidget* parent) : QDialogButtonBox(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_DialogButtonBox");
    setProperty("nativeQtClass", "QDialogButtonBox");
    setProperty("designSystemStyleOwner", "tokens");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
