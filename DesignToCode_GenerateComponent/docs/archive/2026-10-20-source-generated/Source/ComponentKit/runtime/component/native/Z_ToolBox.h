#pragma once

#include <QToolBox>

namespace commonWidget {

// Project-owned component identity for Qt's QToolBox behavior.
class Z_ToolBox : public QToolBox {
    Q_OBJECT
  public:
    explicit Z_ToolBox(QWidget* parent = nullptr);
};

} // namespace commonWidget
