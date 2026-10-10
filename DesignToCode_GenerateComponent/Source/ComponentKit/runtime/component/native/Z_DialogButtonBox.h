#pragma once

#include <QDialogButtonBox>

namespace commonWidget {

// Project-owned component identity for Qt's QDialogButtonBox behavior.
class Z_DialogButtonBox : public QDialogButtonBox {
    Q_OBJECT
  public:
    explicit Z_DialogButtonBox(QWidget* parent = nullptr);
};

} // namespace commonWidget
