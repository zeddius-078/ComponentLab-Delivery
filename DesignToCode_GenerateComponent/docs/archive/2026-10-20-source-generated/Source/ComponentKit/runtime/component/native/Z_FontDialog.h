#pragma once

#include <QFontDialog>

namespace commonWidget {

// Project-owned component identity for Qt's QFontDialog behavior.
class Z_FontDialog : public QFontDialog {
    Q_OBJECT
  public:
    explicit Z_FontDialog(QWidget* parent = nullptr);
};

} // namespace commonWidget
