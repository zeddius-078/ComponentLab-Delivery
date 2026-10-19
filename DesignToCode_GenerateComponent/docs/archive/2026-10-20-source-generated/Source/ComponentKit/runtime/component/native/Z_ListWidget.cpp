#include "Z_ListWidget.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_ListWidget::Z_ListWidget(QWidget* parent) : QListWidget(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ListWidget");
    setProperty("nativeQtClass", "QListWidget");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
