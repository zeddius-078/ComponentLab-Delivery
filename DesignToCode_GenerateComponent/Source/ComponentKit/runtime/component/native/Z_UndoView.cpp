#include "Z_UndoView.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_UndoView::Z_UndoView(QWidget* parent) : QUndoView(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_UndoView");
    setProperty("nativeQtClass", "QUndoView");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
