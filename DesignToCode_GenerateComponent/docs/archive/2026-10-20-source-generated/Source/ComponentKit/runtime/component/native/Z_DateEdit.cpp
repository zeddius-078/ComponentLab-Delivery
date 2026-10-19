#include "Z_DateEdit.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_DateEdit::Z_DateEdit(QWidget* parent) : QDateEdit(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_DateEdit");
    setProperty("nativeQtClass", "QDateEdit");
    setProperty("designSystemStyleOwner", "tokens");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
