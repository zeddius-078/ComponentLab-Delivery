#include "Z_KeySequenceEdit.h"
#include <QVariant>

namespace commonWidget {

Z_KeySequenceEdit::Z_KeySequenceEdit(QWidget* parent) : QKeySequenceEdit(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_KeySequenceEdit");
    setProperty("nativeQtClass", "QKeySequenceEdit");
}

} // namespace commonWidget
