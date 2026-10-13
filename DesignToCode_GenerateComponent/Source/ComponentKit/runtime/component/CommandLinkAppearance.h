#pragma once
#include <QIcon>
#include <QString>
class QCommandLinkButton;

namespace commonWidget {
QIcon figmaCommandLinkIcon();
// Rendering adapter only; the button retains every native action/property contract.
void applyFigmaCommandLinkAppearance(QCommandLinkButton* button, const QString& snapshotState = {},
                                     bool showDescription = true, bool showIcon = true,
                                     bool sourceSnapshot = false);
}
