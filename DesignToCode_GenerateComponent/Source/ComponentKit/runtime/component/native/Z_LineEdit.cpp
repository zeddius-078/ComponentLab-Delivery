#include "Z_LineEdit.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_LineEdit::Z_LineEdit(QWidget* parent) : QLineEdit(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_LineEdit");
    setProperty("nativeQtClass", "QLineEdit");
    setProperty("designSystemStyleOwner", "tokens");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
