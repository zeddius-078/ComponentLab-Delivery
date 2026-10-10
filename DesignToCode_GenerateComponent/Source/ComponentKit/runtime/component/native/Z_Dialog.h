#pragma once

#include <QDialog>

namespace commonWidget {

// Project-owned component identity for Qt's QDialog behavior.
class Z_Dialog : public QDialog {
    Q_OBJECT
  public:
    explicit Z_Dialog(QWidget* parent = nullptr);
};

} // namespace commonWidget
