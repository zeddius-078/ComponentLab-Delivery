#pragma once

#include <QStatusBar>

namespace commonWidget {

// Project-owned component identity for Qt's QStatusBar behavior.
class Z_StatusBar : public QStatusBar {
    Q_OBJECT
  public:
    explicit Z_StatusBar(QWidget* parent = nullptr);
};

} // namespace commonWidget
