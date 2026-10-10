#pragma once

#include <QTabWidget>

namespace commonWidget {

// Project-owned component identity for Qt's QTabWidget behavior.
class Z_TabWidget : public QTabWidget {
    Q_OBJECT
  public:
    explicit Z_TabWidget(QWidget* parent = nullptr);
};

} // namespace commonWidget
