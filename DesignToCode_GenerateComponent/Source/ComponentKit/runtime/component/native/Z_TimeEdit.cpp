#include "Z_TimeEdit.h"
#include <QVariant>

namespace commonWidget {

Z_TimeEdit::Z_TimeEdit(QWidget* parent) : QTimeEdit(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_TimeEdit");
    setProperty("nativeQtClass", "QTimeEdit");
}

} // namespace commonWidget
