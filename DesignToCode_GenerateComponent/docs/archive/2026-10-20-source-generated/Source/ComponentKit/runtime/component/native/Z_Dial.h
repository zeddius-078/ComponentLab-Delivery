#pragma once

#include <QDial>

namespace commonWidget {

// Project-owned component identity for Qt's QDial behavior.
class Z_Dial : public QDial {
    Q_OBJECT
  public:
    explicit Z_Dial(QWidget* parent = nullptr);
};

} // namespace commonWidget
