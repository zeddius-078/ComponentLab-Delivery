#include "Z_ToolButton.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_ToolButton::Z_ToolButton(QWidget* parent) : QToolButton(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ToolButton");
    setProperty("nativeQtClass", "QToolButton");
    setProperty("designSystemStyleOwner", "tokens");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
