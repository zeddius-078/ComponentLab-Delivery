#pragma once

#include <QToolButton>

namespace commonWidget {

// Project-owned component identity for Qt's QToolButton behavior.
class Z_ToolButton : public QToolButton {
    Q_OBJECT
  public:
    explicit Z_ToolButton(QWidget* parent = nullptr);
};

} // namespace commonWidget
