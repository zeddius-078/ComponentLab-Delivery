#pragma once
#include <QIcon>
#include <QString>
class QToolButton;

namespace commonWidget {
// Captured source Light appearance; keeps native QAction, input and popup authority.
QIcon figmaToolButtonIcon();
void applyFigmaToolButtonAppearance(QToolButton* button, const QString& snapshotState = {},
                                    bool sourceSnapshot = false);
}
