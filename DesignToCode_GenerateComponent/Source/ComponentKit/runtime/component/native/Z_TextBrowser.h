#pragma once

#include <QTextBrowser>

namespace commonWidget {

// Project-owned component identity for Qt's QTextBrowser behavior.
class Z_TextBrowser : public QTextBrowser {
    Q_OBJECT
  public:
    explicit Z_TextBrowser(QWidget* parent = nullptr);
};

} // namespace commonWidget
