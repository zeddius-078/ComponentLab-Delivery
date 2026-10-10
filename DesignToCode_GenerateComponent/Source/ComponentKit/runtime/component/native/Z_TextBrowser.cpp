#include "Z_TextBrowser.h"
#include <QVariant>

namespace commonWidget {

Z_TextBrowser::Z_TextBrowser(QWidget* parent) : QTextBrowser(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_TextBrowser");
    setProperty("nativeQtClass", "QTextBrowser");
}

} // namespace commonWidget
