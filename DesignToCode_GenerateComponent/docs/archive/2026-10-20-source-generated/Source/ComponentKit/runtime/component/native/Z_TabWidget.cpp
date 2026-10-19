#include "Z_TabWidget.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_TabWidget::Z_TabWidget(QWidget* parent) : QTabWidget(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_TabWidget");
    setProperty("nativeQtClass", "QTabWidget");
    setProperty("designSystemStyleOwner", "tokens");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
