#pragma once

#include <QListWidget>

namespace commonWidget {

// Project-owned component identity for Qt's QListWidget behavior.
class Z_ListWidget : public QListWidget {
    Q_OBJECT
  public:
    explicit Z_ListWidget(QWidget* parent = nullptr);
};

} // namespace commonWidget
