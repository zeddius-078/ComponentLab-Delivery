#pragma once

#include <QDateTimeEdit>

namespace commonWidget {

// Project-owned component identity for Qt's QDateTimeEdit behavior.
class Z_DateTimeEdit : public QDateTimeEdit {
    Q_OBJECT
  public:
    explicit Z_DateTimeEdit(QWidget* parent = nullptr);
};

} // namespace commonWidget
