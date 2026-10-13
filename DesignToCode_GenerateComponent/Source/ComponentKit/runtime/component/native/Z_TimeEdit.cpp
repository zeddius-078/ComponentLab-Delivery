#include "Z_TimeEdit.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_TimeEdit::Z_TimeEdit(QWidget* parent) : QTimeEdit(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_TimeEdit");
    setProperty("nativeQtClass", "QTimeEdit");
    setProperty("designSystemStyleOwner", "tokens");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
