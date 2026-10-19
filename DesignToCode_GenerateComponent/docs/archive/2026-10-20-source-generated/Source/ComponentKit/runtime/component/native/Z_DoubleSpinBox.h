#pragma once

#include <QDoubleSpinBox>

namespace commonWidget {

// Project-owned component identity for Qt's QDoubleSpinBox behavior.
class Z_DoubleSpinBox : public QDoubleSpinBox {
    Q_OBJECT
  public:
    explicit Z_DoubleSpinBox(QWidget* parent = nullptr);
};

} // namespace commonWidget
