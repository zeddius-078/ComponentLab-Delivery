#include "Z_DockWidget.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_DockWidget::Z_DockWidget(QWidget* parent) : QDockWidget(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_DockWidget");
    setProperty("nativeQtClass", "QDockWidget");
    setProperty("designSystemStyleOwner", "tokens");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
