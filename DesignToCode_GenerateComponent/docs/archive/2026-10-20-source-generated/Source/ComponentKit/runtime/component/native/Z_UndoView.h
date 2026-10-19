#pragma once

#include <QUndoView>

namespace commonWidget {

// Project-owned component identity for Qt's QUndoView behavior.
class Z_UndoView : public QUndoView {
    Q_OBJECT
  public:
    explicit Z_UndoView(QWidget* parent = nullptr);
};

} // namespace commonWidget
