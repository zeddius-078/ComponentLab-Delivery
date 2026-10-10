#include "Z_TextEdit.h"
#include <QVariant>

namespace commonWidget {

Z_TextEdit::Z_TextEdit(QWidget* parent) : QTextEdit(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_TextEdit");
    setProperty("nativeQtClass", "QTextEdit");
}

} // namespace commonWidget
