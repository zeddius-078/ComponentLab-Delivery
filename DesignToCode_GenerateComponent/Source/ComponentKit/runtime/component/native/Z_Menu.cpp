#include "Z_Menu.h"
#include <QVariant>

namespace commonWidget {

Z_Menu::Z_Menu(QWidget* parent) : QMenu(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_Menu");
    setProperty("nativeQtClass", "QMenu");
}

} // namespace commonWidget
