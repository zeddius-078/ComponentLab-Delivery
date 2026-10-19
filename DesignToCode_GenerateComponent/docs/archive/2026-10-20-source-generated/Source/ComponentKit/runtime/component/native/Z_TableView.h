#pragma once

#include <QTableView>

namespace commonWidget {

// Project-owned component identity for Qt's QTableView behavior.
class Z_TableView : public QTableView {
    Q_OBJECT
  public:
    explicit Z_TableView(QWidget* parent = nullptr);
};

} // namespace commonWidget
