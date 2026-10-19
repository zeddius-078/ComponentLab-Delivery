#pragma once

#include <QGraphicsView>

namespace commonWidget {

// Project-owned component identity for Qt's QGraphicsView behavior.
class Z_GraphicsView : public QGraphicsView {
    Q_OBJECT
  public:
    explicit Z_GraphicsView(QWidget* parent = nullptr);
};

} // namespace commonWidget
