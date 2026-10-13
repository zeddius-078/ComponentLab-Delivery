#include "Z_FontComboBox.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_FontComboBox::Z_FontComboBox(QWidget* parent) : QFontComboBox(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_FontComboBox");
    setProperty("nativeQtClass", "QFontComboBox");
    setProperty("designSystemStyleOwner", "tokens");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
