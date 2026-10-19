#pragma once

#include <QMessageBox>

namespace commonWidget {

// Project-owned component identity for Qt's QMessageBox behavior.
class Z_MessageBox : public QMessageBox {
    Q_OBJECT
  public:
    explicit Z_MessageBox(QWidget* parent = nullptr);
};

} // namespace commonWidget
