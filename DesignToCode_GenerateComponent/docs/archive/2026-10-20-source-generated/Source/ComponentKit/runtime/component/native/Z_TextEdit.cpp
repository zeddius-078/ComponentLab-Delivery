#include "Z_TextEdit.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_TextEdit::Z_TextEdit(QWidget* parent) : QTextEdit(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_TextEdit");
    setProperty("nativeQtClass", "QTextEdit");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
