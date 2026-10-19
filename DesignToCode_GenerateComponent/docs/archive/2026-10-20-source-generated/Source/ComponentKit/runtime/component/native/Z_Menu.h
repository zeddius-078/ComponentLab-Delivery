#pragma once

#include <QMenu>

namespace commonWidget {

// Project-owned component identity for Qt's QMenu behavior.
class Z_Menu : public QMenu {
    Q_OBJECT
  public:
    explicit Z_Menu(QWidget* parent = nullptr);
};

} // namespace commonWidget
