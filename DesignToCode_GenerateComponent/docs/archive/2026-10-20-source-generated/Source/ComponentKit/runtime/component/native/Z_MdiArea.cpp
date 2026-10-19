#include "Z_MdiArea.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_MdiArea::Z_MdiArea(QWidget* parent) : QMdiArea(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_MdiArea");
    setProperty("nativeQtClass", "QMdiArea");
    setProperty("designSystemStyleOwner", "tokens");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
