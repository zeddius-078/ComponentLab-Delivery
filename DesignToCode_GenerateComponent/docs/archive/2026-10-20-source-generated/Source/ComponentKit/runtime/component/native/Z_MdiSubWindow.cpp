#include "Z_MdiSubWindow.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_MdiSubWindow::Z_MdiSubWindow(QWidget* parent) : QMdiSubWindow(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_MdiSubWindow");
    setProperty("nativeQtClass", "QMdiSubWindow");
    setProperty("designSystemStyleOwner", "tokens");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
