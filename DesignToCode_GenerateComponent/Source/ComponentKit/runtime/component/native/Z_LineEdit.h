#pragma once

#include <QLineEdit>

namespace commonWidget {

// Project-owned component identity for Qt's QLineEdit behavior.
class Z_LineEdit : public QLineEdit {
    Q_OBJECT
  public:
    explicit Z_LineEdit(QWidget* parent = nullptr);
};

} // namespace commonWidget
