#pragma once

#include <QSpinBox>

namespace commonWidget {

// Project-owned component identity for Qt's QSpinBox behavior.
class Z_SpinBox : public QSpinBox {
    Q_OBJECT
  public:
    explicit Z_SpinBox(QWidget* parent = nullptr);
};

} // namespace commonWidget
