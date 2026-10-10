#include "Z_MdiSubWindow.h"
#include <QVariant>

namespace commonWidget {

Z_MdiSubWindow::Z_MdiSubWindow(QWidget* parent) : QMdiSubWindow(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_MdiSubWindow");
    setProperty("nativeQtClass", "QMdiSubWindow");
}

} // namespace commonWidget
