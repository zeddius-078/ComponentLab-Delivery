#include "Z_TableWidget.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_TableWidget::Z_TableWidget(QWidget* parent) : QTableWidget(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_TableWidget");
    setProperty("nativeQtClass", "QTableWidget");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
