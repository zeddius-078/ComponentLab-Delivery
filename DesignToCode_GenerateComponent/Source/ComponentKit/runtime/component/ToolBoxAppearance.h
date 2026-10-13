#pragma once
#include <QIcon>
#include <QString>
class QToolBox;

namespace commonWidget {
QIcon figmaToolBoxSettingsIcon(bool composition = false);
// Paint-only state override; item selection, focus, shortcuts and ownership stay native.
void applyFigmaToolBoxAppearance(QToolBox* box, const QString& snapshotState = {},
                                 int previewIndex = -1, bool showMnemonic = false);
}
