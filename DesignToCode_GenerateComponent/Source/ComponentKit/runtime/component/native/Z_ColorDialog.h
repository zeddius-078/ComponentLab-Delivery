#pragma once

#include <QColorDialog>

namespace commonWidget {

// Project-owned component identity for Qt's QColorDialog behavior.
class Z_ColorDialog : public QColorDialog {
    Q_OBJECT
  public:
    explicit Z_ColorDialog(QWidget* parent = nullptr);
};

} // namespace commonWidget
