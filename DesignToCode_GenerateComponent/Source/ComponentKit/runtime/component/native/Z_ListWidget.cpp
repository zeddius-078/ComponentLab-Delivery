#include "Z_ListWidget.h"
#include <QVariant>

namespace commonWidget {

Z_ListWidget::Z_ListWidget(QWidget* parent) : QListWidget(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ListWidget");
    setProperty("nativeQtClass", "QListWidget");
}

} // namespace commonWidget
