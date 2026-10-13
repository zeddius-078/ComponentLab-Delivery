#pragma once
#include <QString>
class QLabel;

namespace commonWidget {
// Captured Light specimen of ComponentLab/Label 414:39. Alignment is logical;
// text, wrapping, interaction, links and accessibility remain native QLabel.
void applyFigmaLabelAppearance(QLabel* label, const QString& alignment, bool wrap);
}
