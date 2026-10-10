#pragma once

#include <QRadioButton>

namespace commonWidget {

// Project-owned component identity for Qt's QRadioButton behavior.
class Z_RadioButton : public QRadioButton {
    Q_OBJECT
  public:
    explicit Z_RadioButton(QWidget* parent = nullptr);
};

} // namespace commonWidget
