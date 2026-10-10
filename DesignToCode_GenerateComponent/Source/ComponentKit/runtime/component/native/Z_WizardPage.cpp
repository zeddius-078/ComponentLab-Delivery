#include "Z_WizardPage.h"
#include <QVariant>

namespace commonWidget {

Z_WizardPage::Z_WizardPage(QWidget* parent) : QWizardPage(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_WizardPage");
    setProperty("nativeQtClass", "QWizardPage");
}

} // namespace commonWidget
