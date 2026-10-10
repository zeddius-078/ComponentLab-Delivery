#pragma once

#include <QComboBox>

namespace commonWidget {

// Project-owned component identity for Qt's QComboBox behavior.
class Z_ComboBox : public QComboBox {
    Q_OBJECT
  public:
    explicit Z_ComboBox(QWidget* parent = nullptr);
};

} // namespace commonWidget
