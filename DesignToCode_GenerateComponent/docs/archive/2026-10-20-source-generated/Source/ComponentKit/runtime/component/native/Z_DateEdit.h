#pragma once

#include <QDateEdit>

namespace commonWidget {

// Project-owned component identity for Qt's QDateEdit behavior.
class Z_DateEdit : public QDateEdit {
    Q_OBJECT
  public:
    explicit Z_DateEdit(QWidget* parent = nullptr);
};

} // namespace commonWidget
