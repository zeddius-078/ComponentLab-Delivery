#pragma once

#include <QWizard>

namespace commonWidget {

// Project-owned component identity for Qt's QWizard behavior.
class Z_Wizard : public QWizard {
    Q_OBJECT
  public:
    explicit Z_Wizard(QWidget* parent = nullptr);
};

} // namespace commonWidget
