#pragma once

#include <QFileDialog>

namespace commonWidget {

// Project-owned component identity for Qt's QFileDialog behavior.
class Z_FileDialog : public QFileDialog {
    Q_OBJECT
  public:
    explicit Z_FileDialog(QWidget* parent = nullptr);
};

} // namespace commonWidget
