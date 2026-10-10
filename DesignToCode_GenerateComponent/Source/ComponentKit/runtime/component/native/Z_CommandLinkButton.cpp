#include "Z_CommandLinkButton.h"
#include <QVariant>

namespace commonWidget {

Z_CommandLinkButton::Z_CommandLinkButton(QWidget* parent) : QCommandLinkButton(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_CommandLinkButton");
    setProperty("nativeQtClass", "QCommandLinkButton");
}

} // namespace commonWidget
