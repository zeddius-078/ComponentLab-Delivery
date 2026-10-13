#include "Z_Splitter.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_Splitter::Z_Splitter(QWidget* parent) : QSplitter(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_Splitter");
    setProperty("nativeQtClass", "QSplitter");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
