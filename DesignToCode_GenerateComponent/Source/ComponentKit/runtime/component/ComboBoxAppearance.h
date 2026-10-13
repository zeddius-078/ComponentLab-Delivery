#pragma once
#include <QString>
class QComboBox;

namespace commonWidget {
// Presentation only: retains the native model, delegate, editor and selection.
// sourceSnapshot is reserved for inert FontComboBox gallery text (Arial).
void applyFigmaComboBoxAppearance(QComboBox* combo, const QString& size = "Middle",
                                  const QString& snapshotState = {}, bool sourceSnapshot = false);
}
