#pragma once
#include <QIcon>
#include <QString>
class QToolBox;

namespace commonWidget {
QIcon figmaToolBoxSettingsIcon(bool composition = false);
// Rebuild the public ComponentLab/ToolBox composition from the SDK-owned contract.
// This intentionally deletes existing item pages; call it only for a new specimen.
void populateFigmaToolBox(QToolBox* box, bool sourceSnapshot = false);
// Paint-only state override; item selection, focus, shortcuts and ownership stay native.
void applyFigmaToolBoxAppearance(QToolBox* box, const QString& snapshotState = {},
                                 int previewIndex = -1, bool showMnemonic = false);
}
