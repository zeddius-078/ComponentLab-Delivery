#include "Z_ToolButton.h"
#include <QVariant>

namespace commonWidget {

Z_ToolButton::Z_ToolButton(QWidget* parent) : QToolButton(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ToolButton");
    setProperty("nativeQtClass", "QToolButton");
}

} // namespace commonWidget
