#pragma once

#include <QLCDNumber>

namespace commonWidget {

// Project-owned component identity for Qt's QLCDNumber behavior.
class Z_LCDNumber : public QLCDNumber {
    Q_OBJECT
  public:
    explicit Z_LCDNumber(QWidget* parent = nullptr);
};

} // namespace commonWidget
