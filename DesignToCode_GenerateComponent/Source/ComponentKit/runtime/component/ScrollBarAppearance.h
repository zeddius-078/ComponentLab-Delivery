#pragma once
#include <QMap>
#include <QString>
class QScrollBar;

namespace commonWidget {
// Applies the public Figma 471:477 Orientation x State contract. Hover and
// Dragging are visual snapshots; range, pageStep and sliderDown remain native.
void configureFigmaScrollBar(QScrollBar* bar, const QMap<QString, QString>& properties,
                             bool sourceSnapshot = false);
// Captured Light styling for the existing native complex control. Neither
// snapshot option changes the range, pageStep, value, focus policy or sliderDown.
// Show line buttons is an adapter option, not a new QScrollBar property.
void applyFigmaScrollBarAppearance(QScrollBar* bar, const QString& snapshotState = {},
                                   bool snapshotGeometry = false, bool showLineButtons = true);
}
