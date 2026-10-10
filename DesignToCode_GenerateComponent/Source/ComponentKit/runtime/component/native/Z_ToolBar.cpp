#include "Z_ToolBar.h"
#include <QVariant>

namespace commonWidget {

Z_ToolBar::Z_ToolBar(QWidget* parent) : QToolBar(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ToolBar");
    setProperty("nativeQtClass", "QToolBar");
}

} // namespace commonWidget
