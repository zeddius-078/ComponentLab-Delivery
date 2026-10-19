#pragma once

#include <QMenuBar>

namespace commonWidget {

// Project-owned component identity for Qt's QMenuBar behavior.
class Z_MenuBar : public QMenuBar {
    Q_OBJECT
  public:
    explicit Z_MenuBar(QWidget* parent = nullptr);
};

} // namespace commonWidget
