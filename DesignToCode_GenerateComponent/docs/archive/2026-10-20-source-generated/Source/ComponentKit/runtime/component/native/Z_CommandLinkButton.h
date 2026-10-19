#pragma once

#include <QCommandLinkButton>

namespace commonWidget {

// Project-owned component identity for Qt's QCommandLinkButton behavior.
class Z_CommandLinkButton : public QCommandLinkButton {
    Q_OBJECT
  public:
    explicit Z_CommandLinkButton(QWidget* parent = nullptr);
};

} // namespace commonWidget
