#include "Z_ProgressBar.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_ProgressBar::Z_ProgressBar(QWidget* parent) : QProgressBar(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ProgressBar");
    setProperty("nativeQtClass", "QProgressBar");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
