#pragma once

#include <QGroupBox>

namespace commonWidget {

// Project-owned component identity for Qt's QGroupBox behavior.
class Z_GroupBox : public QGroupBox {
    Q_OBJECT
  public:
    explicit Z_GroupBox(QWidget* parent = nullptr);
};

} // namespace commonWidget
