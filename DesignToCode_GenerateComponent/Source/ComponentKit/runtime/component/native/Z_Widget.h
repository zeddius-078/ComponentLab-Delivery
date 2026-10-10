#pragma once

#include <QWidget>

namespace commonWidget {

// Project-owned component identity for Qt's QWidget behavior.
class Z_Widget : public QWidget {
    Q_OBJECT
  public:
    explicit Z_Widget(QWidget* parent = nullptr);
};

} // namespace commonWidget
