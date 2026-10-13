#include "Z_TreeWidget.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_TreeWidget::Z_TreeWidget(QWidget* parent) : QTreeWidget(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_TreeWidget");
    setProperty("nativeQtClass", "QTreeWidget");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
