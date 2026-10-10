#include "Z_PlainTextEdit.h"
#include <QVariant>

namespace commonWidget {

Z_PlainTextEdit::Z_PlainTextEdit(QWidget* parent) : QPlainTextEdit(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_PlainTextEdit");
    setProperty("nativeQtClass", "QPlainTextEdit");
}

} // namespace commonWidget
