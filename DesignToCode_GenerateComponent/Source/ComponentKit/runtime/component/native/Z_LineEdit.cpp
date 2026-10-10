#include "Z_LineEdit.h"
#include <QVariant>

namespace commonWidget {

Z_LineEdit::Z_LineEdit(QWidget* parent) : QLineEdit(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_LineEdit");
    setProperty("nativeQtClass", "QLineEdit");
}

} // namespace commonWidget
