#pragma once

#include <QStackedWidget>

namespace commonWidget {

// Project-owned component identity for Qt's QStackedWidget behavior.
class Z_StackedWidget : public QStackedWidget {
    Q_OBJECT
  public:
    explicit Z_StackedWidget(QWidget* parent = nullptr);
};

} // namespace commonWidget
