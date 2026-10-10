#include "Z_ListView.h"
#include <QVariant>

namespace commonWidget {

Z_ListView::Z_ListView(QWidget* parent) : QListView(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_ListView");
    setProperty("nativeQtClass", "QListView");
}

} // namespace commonWidget
