#pragma once

#include <QTextEdit>

namespace commonWidget {

// Project-owned component identity for Qt's QTextEdit behavior.
class Z_TextEdit : public QTextEdit {
    Q_OBJECT
  public:
    explicit Z_TextEdit(QWidget* parent = nullptr);
};

} // namespace commonWidget
