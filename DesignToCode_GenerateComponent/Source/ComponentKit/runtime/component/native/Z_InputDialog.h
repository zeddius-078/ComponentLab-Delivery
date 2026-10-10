#pragma once

#include <QInputDialog>

namespace commonWidget {

// Project-owned component identity for Qt's QInputDialog behavior.
class Z_InputDialog : public QInputDialog {
    Q_OBJECT
  public:
    explicit Z_InputDialog(QWidget* parent = nullptr);
};

} // namespace commonWidget
