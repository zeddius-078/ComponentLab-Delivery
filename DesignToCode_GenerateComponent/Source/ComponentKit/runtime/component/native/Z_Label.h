#pragma once

#include <QLabel>

namespace commonWidget {

// Project-owned component identity for Qt's QLabel behavior.
class Z_Label : public QLabel {
    Q_OBJECT
  public:
    explicit Z_Label(QWidget* parent = nullptr);
};

} // namespace commonWidget
