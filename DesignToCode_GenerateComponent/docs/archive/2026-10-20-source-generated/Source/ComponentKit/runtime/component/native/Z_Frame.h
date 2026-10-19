#pragma once

#include <QFrame>

namespace commonWidget {

// Project-owned component identity for Qt's QFrame behavior.
class Z_Frame : public QFrame {
    Q_OBJECT
  public:
    explicit Z_Frame(QWidget* parent = nullptr);
};

} // namespace commonWidget
