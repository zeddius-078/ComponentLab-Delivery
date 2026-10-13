#pragma once
#include <QString>
class QMdiSubWindow;
class QMdiArea;

namespace commonWidget {
// These are paint previews only, never native window-state/activation commands.
void applyFigmaMdiSubWindowAppearance(QMdiSubWindow* window, const QString& snapshot = {},
                                      bool closeGlyph = true, bool minimizeGlyph = true,
                                      bool maximizeGlyph = true);
// Preserves the QMdiArea model, native tab bar, managed windows and close-event veto.
void applyFigmaMdiAreaAppearance(QMdiArea* area);
}
