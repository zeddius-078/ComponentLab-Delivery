#pragma once

#include <QMdiSubWindow>

namespace commonWidget {

// Project-owned component identity for Qt's QMdiSubWindow behavior.
class Z_MdiSubWindow : public QMdiSubWindow {
    Q_OBJECT
  public:
    explicit Z_MdiSubWindow(QWidget* parent = nullptr);
};

} // namespace commonWidget
