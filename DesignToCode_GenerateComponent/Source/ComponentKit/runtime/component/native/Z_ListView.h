#pragma once

#include <QListView>

namespace commonWidget {

// Project-owned component identity for Qt's QListView behavior.
class Z_ListView : public QListView {
    Q_OBJECT
  public:
    explicit Z_ListView(QWidget* parent = nullptr);
};

} // namespace commonWidget
