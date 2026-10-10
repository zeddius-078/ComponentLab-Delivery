#pragma once

#include <QMdiArea>

namespace commonWidget {

// Project-owned component identity for Qt's QMdiArea behavior.
class Z_MdiArea : public QMdiArea {
    Q_OBJECT
  public:
    explicit Z_MdiArea(QWidget* parent = nullptr);
};

} // namespace commonWidget
