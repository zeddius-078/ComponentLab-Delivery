#pragma once
#include <QString>
class QScrollBar;

namespace commonWidget {
// Captured Light styling for the existing native complex control. Neither
// snapshot option changes the range, pageStep, value, focus policy or sliderDown.
// Show line buttons is an adapter option, not a new QScrollBar property.
void applyFigmaScrollBarAppearance(QScrollBar* bar, const QString& snapshotState = {},
                                   bool snapshotGeometry = false, bool showLineButtons = true);
}
