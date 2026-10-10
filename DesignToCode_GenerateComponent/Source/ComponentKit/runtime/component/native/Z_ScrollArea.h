#pragma once

#include <QScrollArea>

namespace commonWidget {

// Project-owned component identity for Qt's QScrollArea behavior.
class Z_ScrollArea : public QScrollArea {
    Q_OBJECT
  public:
    explicit Z_ScrollArea(QWidget* parent = nullptr);
};

} // namespace commonWidget
