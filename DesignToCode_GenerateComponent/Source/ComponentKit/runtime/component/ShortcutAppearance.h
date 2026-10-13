#pragma once
#include <QString>
class QKeySequenceEdit;

namespace commonWidget {
// Presentation adapter only. QKeySequenceEdit still owns capture, parsing and timing.
// sourceSnapshot is reserved for inert authored gallery specimens.
void applyFigmaShortcutAppearance(QKeySequenceEdit* edit, const QString& snapshotState = {},
                                  bool showClear = true, bool showHint = true,
                                  bool sourceSnapshot = false);
// The host registry supplies the message. This never assigns a command or shortcut.
void setFigmaShortcutConflict(QKeySequenceEdit* edit, const QString& message = {});
}
