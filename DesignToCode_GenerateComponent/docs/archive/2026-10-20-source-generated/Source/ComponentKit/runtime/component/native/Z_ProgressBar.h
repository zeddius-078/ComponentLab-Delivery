#pragma once

#include <QProgressBar>

namespace commonWidget {

// Project-owned component identity for Qt's QProgressBar behavior.
class Z_ProgressBar : public QProgressBar {
    Q_OBJECT
  public:
    explicit Z_ProgressBar(QWidget* parent = nullptr);
};

} // namespace commonWidget
