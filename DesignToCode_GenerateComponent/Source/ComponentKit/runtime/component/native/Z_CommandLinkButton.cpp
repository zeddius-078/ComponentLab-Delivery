#include "Z_CommandLinkButton.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_CommandLinkButton::Z_CommandLinkButton(QWidget* parent) : QCommandLinkButton(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_CommandLinkButton");
    setProperty("nativeQtClass", "QCommandLinkButton");
    setProperty("designSystemStyleOwner", "tokens");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
