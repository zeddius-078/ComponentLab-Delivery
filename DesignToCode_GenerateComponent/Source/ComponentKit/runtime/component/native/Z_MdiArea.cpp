#include "Z_MdiArea.h"
#include <QVariant>

namespace commonWidget {

Z_MdiArea::Z_MdiArea(QWidget* parent) : QMdiArea(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_MdiArea");
    setProperty("nativeQtClass", "QMdiArea");
}

} // namespace commonWidget
