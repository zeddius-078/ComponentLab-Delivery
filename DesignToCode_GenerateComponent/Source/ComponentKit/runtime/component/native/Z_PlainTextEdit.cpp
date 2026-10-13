#include "Z_PlainTextEdit.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_PlainTextEdit::Z_PlainTextEdit(QWidget* parent) : QPlainTextEdit(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_PlainTextEdit");
    setProperty("nativeQtClass", "QPlainTextEdit");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
