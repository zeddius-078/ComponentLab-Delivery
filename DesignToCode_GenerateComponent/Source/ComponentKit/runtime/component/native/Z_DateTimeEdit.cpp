#include "Z_DateTimeEdit.h"
#include <QVariant>

namespace commonWidget {

Z_DateTimeEdit::Z_DateTimeEdit(QWidget* parent) : QDateTimeEdit(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_DateTimeEdit");
    setProperty("nativeQtClass", "QDateTimeEdit");
}

} // namespace commonWidget
