#include "Z_Splitter.h"
#include <QVariant>

namespace commonWidget {

Z_Splitter::Z_Splitter(QWidget* parent) : QSplitter(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_Splitter");
    setProperty("nativeQtClass", "QSplitter");
}

} // namespace commonWidget
