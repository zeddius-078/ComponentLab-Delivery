#include "Z_ColumnView.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_ColumnView::Z_ColumnView(QWidget* parent) : QColumnView(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ColumnView");
    setProperty("nativeQtClass", "QColumnView");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
