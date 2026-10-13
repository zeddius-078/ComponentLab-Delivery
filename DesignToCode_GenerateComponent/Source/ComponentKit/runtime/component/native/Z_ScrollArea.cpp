#include "Z_ScrollArea.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_ScrollArea::Z_ScrollArea(QWidget* parent) : QScrollArea(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ScrollArea");
    setProperty("nativeQtClass", "QScrollArea");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
