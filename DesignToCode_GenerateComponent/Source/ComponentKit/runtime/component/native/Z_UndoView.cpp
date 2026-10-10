#include "Z_UndoView.h"
#include <QVariant>

namespace commonWidget {

Z_UndoView::Z_UndoView(QWidget* parent) : QUndoView(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_UndoView");
    setProperty("nativeQtClass", "QUndoView");
}

} // namespace commonWidget
