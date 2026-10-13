#include "Z_TextBrowser.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_TextBrowser::Z_TextBrowser(QWidget* parent) : QTextBrowser(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_TextBrowser");
    setProperty("nativeQtClass", "QTextBrowser");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
