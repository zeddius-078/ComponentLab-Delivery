#pragma once

#include <QTreeView>

namespace commonWidget {

// Project-owned component identity for Qt's QTreeView behavior.
class Z_TreeView : public QTreeView {
    Q_OBJECT
  public:
    explicit Z_TreeView(QWidget* parent = nullptr);
};

} // namespace commonWidget
