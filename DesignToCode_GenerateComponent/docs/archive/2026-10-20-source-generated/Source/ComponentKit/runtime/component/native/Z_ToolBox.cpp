#include "Z_ToolBox.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_ToolBox::Z_ToolBox(QWidget* parent) : QToolBox(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ToolBox");
    setProperty("nativeQtClass", "QToolBox");
    setProperty("designSystemStyleOwner", "tokens");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
