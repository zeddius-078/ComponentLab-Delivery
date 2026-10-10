#pragma once

#include <QPlainTextEdit>

namespace commonWidget {

// Project-owned component identity for Qt's QPlainTextEdit behavior.
class Z_PlainTextEdit : public QPlainTextEdit {
    Q_OBJECT
  public:
    explicit Z_PlainTextEdit(QWidget* parent = nullptr);
};

} // namespace commonWidget
