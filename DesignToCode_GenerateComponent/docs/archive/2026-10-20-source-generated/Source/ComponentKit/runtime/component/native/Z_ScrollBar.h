#pragma once

#include <QScrollBar>

namespace commonWidget {

// Project-owned component identity for Qt's QScrollBar behavior.
class Z_ScrollBar : public QScrollBar {
    Q_OBJECT
  public:
    explicit Z_ScrollBar(QWidget* parent = nullptr);
};

} // namespace commonWidget
