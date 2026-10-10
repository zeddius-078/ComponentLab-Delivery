#include "Z_MenuBar.h"
#include <QVariant>

namespace commonWidget {

Z_MenuBar::Z_MenuBar(QWidget* parent) : QMenuBar(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_MenuBar");
    setProperty("nativeQtClass", "QMenuBar");
}

} // namespace commonWidget
