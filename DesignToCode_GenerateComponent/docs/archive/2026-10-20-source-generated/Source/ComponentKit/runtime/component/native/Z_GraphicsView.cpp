#include "Z_GraphicsView.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_GraphicsView::Z_GraphicsView(QWidget* parent) : QGraphicsView(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_GraphicsView");
    setProperty("nativeQtClass", "QGraphicsView");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
