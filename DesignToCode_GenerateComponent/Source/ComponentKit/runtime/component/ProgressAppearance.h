#pragma once
#include <QString>
class QProgressBar;

namespace commonWidget {
// Captured Light styling for native QProgressBar. Range/value, enabled state,
// orientation, invertedAppearance, format and textVisible remain native-owned.
// Status is visual only: Completed must not silently set a caller's value to 100.
// Snapshot is for the static Figma gallery; live busy motion is an engineering
// fallback, not a designer-approved timing. reducedMotion=true freezes it.
void applyFigmaProgressAppearance(QProgressBar* progress, const QString& status = "Determinate",
                                  bool snapshot = false);
}
