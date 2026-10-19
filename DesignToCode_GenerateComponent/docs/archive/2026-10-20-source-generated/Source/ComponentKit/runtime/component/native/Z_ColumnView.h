#pragma once

#include <QColumnView>

namespace commonWidget {

// Project-owned component identity for Qt's QColumnView behavior.
class Z_ColumnView : public QColumnView {
    Q_OBJECT
  public:
    explicit Z_ColumnView(QWidget* parent = nullptr);
};

} // namespace commonWidget
