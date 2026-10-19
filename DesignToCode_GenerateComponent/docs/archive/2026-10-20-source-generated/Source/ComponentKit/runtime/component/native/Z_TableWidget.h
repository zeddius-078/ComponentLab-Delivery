#pragma once

#include <QTableWidget>

namespace commonWidget {

// Project-owned component identity for Qt's QTableWidget behavior.
class Z_TableWidget : public QTableWidget {
    Q_OBJECT
  public:
    explicit Z_TableWidget(QWidget* parent = nullptr);
};

} // namespace commonWidget
