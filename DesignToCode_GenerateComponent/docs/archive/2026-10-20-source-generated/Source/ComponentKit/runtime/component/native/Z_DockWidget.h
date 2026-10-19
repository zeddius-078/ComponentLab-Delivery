#pragma once

#include <QDockWidget>

namespace commonWidget {

// Project-owned component identity for Qt's QDockWidget behavior.
class Z_DockWidget : public QDockWidget {
    Q_OBJECT
  public:
    explicit Z_DockWidget(QWidget* parent = nullptr);
};

} // namespace commonWidget
