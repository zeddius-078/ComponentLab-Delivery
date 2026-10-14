#pragma once
#include <QMap>
#include <QString>
class QLabel;

namespace commonWidget {
// Captured Light specimen of ComponentLab/Label 414:39. Alignment is logical;
// text, wrapping, interaction, links and accessibility remain native QLabel.
void applyFigmaLabelAppearance(QLabel* label, const QString& alignment, bool wrap);
// Applies the complete governed variant tuple. State only controls QWidget
// enabled state; it never converts Label into an interactive focus target.
void configureFigmaLabel(QLabel* label, const QMap<QString, QString>& properties);
}
