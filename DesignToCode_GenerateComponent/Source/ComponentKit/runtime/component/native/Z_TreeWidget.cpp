#include "Z_TreeWidget.h"
#include <QVariant>

namespace commonWidget {

Z_TreeWidget::Z_TreeWidget(QWidget* parent) : QTreeWidget(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_TreeWidget");
    setProperty("nativeQtClass", "QTreeWidget");
}

} // namespace commonWidget
