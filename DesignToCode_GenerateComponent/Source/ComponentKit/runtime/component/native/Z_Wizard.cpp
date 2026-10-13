#include "Z_Wizard.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_Wizard::Z_Wizard(QWidget* parent) : QWizard(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_Wizard");
    setProperty("nativeQtClass", "QWizard");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
