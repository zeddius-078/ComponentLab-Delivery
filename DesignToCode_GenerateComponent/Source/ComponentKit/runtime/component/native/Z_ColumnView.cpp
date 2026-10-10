#include "Z_ColumnView.h"
#include <QVariant>

namespace commonWidget {

Z_ColumnView::Z_ColumnView(QWidget* parent) : QColumnView(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ColumnView");
    setProperty("nativeQtClass", "QColumnView");
}

} // namespace commonWidget
