#pragma once

#include <QKeySequenceEdit>

namespace commonWidget {

// Project-owned component identity for Qt's QKeySequenceEdit behavior.
class Z_KeySequenceEdit : public QKeySequenceEdit {
    Q_OBJECT
  public:
    explicit Z_KeySequenceEdit(QWidget* parent = nullptr);
};

} // namespace commonWidget
