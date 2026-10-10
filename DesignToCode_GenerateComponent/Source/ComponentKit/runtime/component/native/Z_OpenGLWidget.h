#pragma once

#include <QOpenGLWidget>

namespace commonWidget {

// Project-owned component identity for Qt's QOpenGLWidget behavior.
class Z_OpenGLWidget : public QOpenGLWidget {
    Q_OBJECT
  public:
    explicit Z_OpenGLWidget(QWidget* parent = nullptr);
};

} // namespace commonWidget
