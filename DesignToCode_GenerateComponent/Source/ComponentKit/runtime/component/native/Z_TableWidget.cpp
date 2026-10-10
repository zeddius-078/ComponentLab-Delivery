#include "Z_TableWidget.h"
#include <QVariant>

namespace commonWidget {

Z_TableWidget::Z_TableWidget(QWidget* parent) : QTableWidget(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_TableWidget");
    setProperty("nativeQtClass", "QTableWidget");
}

} // namespace commonWidget
