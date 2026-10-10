#pragma once

#include <QProgressDialog>

namespace commonWidget {

// Project-owned component identity for Qt's QProgressDialog behavior.
class Z_ProgressDialog : public QProgressDialog {
    Q_OBJECT
  public:
    explicit Z_ProgressDialog(QWidget* parent = nullptr);
};

} // namespace commonWidget
