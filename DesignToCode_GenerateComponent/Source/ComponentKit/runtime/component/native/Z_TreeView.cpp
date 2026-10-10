#include "Z_TreeView.h"
#include <QVariant>

namespace commonWidget {

Z_TreeView::Z_TreeView(QWidget* parent) : QTreeView(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_TreeView");
    setProperty("nativeQtClass", "QTreeView");
}

} // namespace commonWidget
