#pragma once
#include <QString>
class QSlider;

namespace commonWidget {
// Values, position, tracking, steps, inversion, ticks and signals stay QSlider-owned.
// snapshotGeometry is only for inert authored-gallery comparisons: Figma label,
// handle and active track are independent layers, not an encoded native value.
// A null valueLabel follows sliderPosition; a non-null label is independent text.
void applyFigmaSliderAppearance(QSlider* slider, const QString& snapshotState = {},
                                bool snapshotGeometry = false, bool showValue = true,
                                const QString& valueLabel = {});
}
