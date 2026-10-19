#include "Z_TableView.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_TableView::Z_TableView(QWidget* parent) : QTableView(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_TableView");
    setProperty("nativeQtClass", "QTableView");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
