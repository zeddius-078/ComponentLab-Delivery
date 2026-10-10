#include "Z_ScrollArea.h"
#include <QVariant>

namespace commonWidget {

Z_ScrollArea::Z_ScrollArea(QWidget* parent) : QScrollArea(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ScrollArea");
    setProperty("nativeQtClass", "QScrollArea");
}

} // namespace commonWidget
