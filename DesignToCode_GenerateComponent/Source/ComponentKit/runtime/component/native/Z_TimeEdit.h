#pragma once

#include <QTimeEdit>

namespace commonWidget {

// Project-owned component identity for Qt's QTimeEdit behavior.
class Z_TimeEdit : public QTimeEdit {
    Q_OBJECT
  public:
    explicit Z_TimeEdit(QWidget* parent = nullptr);
};

} // namespace commonWidget
