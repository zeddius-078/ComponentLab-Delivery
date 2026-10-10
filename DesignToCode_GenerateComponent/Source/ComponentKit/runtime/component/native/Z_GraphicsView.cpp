#include "Z_GraphicsView.h"
#include <QVariant>

namespace commonWidget {

Z_GraphicsView::Z_GraphicsView(QWidget* parent) : QGraphicsView(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_GraphicsView");
    setProperty("nativeQtClass", "QGraphicsView");
}

} // namespace commonWidget
