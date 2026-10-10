#pragma once

#include <QSlider>

namespace commonWidget {

// Project-owned component identity for Qt's QSlider behavior.
class Z_Slider : public QSlider {
    Q_OBJECT
  public:
    explicit Z_Slider(QWidget* parent = nullptr);
};

} // namespace commonWidget
