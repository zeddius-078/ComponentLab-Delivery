#include "Z_ScrollBar.h"
#include <QVariant>

namespace commonWidget {

Z_ScrollBar::Z_ScrollBar(QWidget* parent) : QScrollBar(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ScrollBar");
    setProperty("nativeQtClass", "QScrollBar");
}

} // namespace commonWidget
