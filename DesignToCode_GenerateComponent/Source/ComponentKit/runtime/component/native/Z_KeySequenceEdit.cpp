#include "Z_KeySequenceEdit.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_KeySequenceEdit::Z_KeySequenceEdit(QWidget* parent) : QKeySequenceEdit(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_KeySequenceEdit");
    setProperty("nativeQtClass", "QKeySequenceEdit");
    setProperty("designSystemStyleOwner", "tokens");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
