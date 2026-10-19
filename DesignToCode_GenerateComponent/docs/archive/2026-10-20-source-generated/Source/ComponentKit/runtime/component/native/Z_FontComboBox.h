#pragma once

#include <QFontComboBox>

namespace commonWidget {

// Project-owned component identity for Qt's QFontComboBox behavior.
class Z_FontComboBox : public QFontComboBox {
    Q_OBJECT
  public:
    explicit Z_FontComboBox(QWidget* parent = nullptr);
};

} // namespace commonWidget
