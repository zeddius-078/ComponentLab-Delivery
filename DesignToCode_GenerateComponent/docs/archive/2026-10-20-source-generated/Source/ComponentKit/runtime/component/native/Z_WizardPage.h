#pragma once

#include <QWizardPage>

namespace commonWidget {

// Project-owned component identity for Qt's QWizardPage behavior.
class Z_WizardPage : public QWizardPage {
    Q_OBJECT
  public:
    explicit Z_WizardPage(QWidget* parent = nullptr);
};

} // namespace commonWidget
