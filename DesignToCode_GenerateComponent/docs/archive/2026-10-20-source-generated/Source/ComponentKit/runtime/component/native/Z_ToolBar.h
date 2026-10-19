#pragma once

#include <QToolBar>

namespace commonWidget {

// Project-owned component identity for Qt's QToolBar behavior.
class Z_ToolBar : public QToolBar {
    Q_OBJECT
  public:
    explicit Z_ToolBar(QWidget* parent = nullptr);
};

} // namespace commonWidget
