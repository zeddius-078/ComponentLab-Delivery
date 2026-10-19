#pragma once

#include <QTreeWidget>

namespace commonWidget {

// Project-owned component identity for Qt's QTreeWidget behavior.
class Z_TreeWidget : public QTreeWidget {
    Q_OBJECT
  public:
    explicit Z_TreeWidget(QWidget* parent = nullptr);
};

} // namespace commonWidget
