#pragma once
#include <QMap>
#include <QString>
class QSlider;

namespace commonWidget {
// Applies the public Figma 467:195 Orientation x State contract. Show ticks and
// Show value are optional composition preferences translated to native Qt
// properties; Dragging remains a visual snapshot and never sets sliderDown.
void configureFigmaSlider(QSlider* slider, const QMap<QString, QString>& properties,
                          bool sourceSnapshot = false);
// Values, position, tracking, steps, inversion, ticks and signals stay QSlider-owned.
// snapshotGeometry is only for inert authored-gallery comparisons: Figma label,
// handle and active track are independent layers, not an encoded native value.
// A null valueLabel follows sliderPosition; a non-null label is independent text.
void applyFigmaSliderAppearance(QSlider* slider, const QString& snapshotState = {},
                                bool snapshotGeometry = false, bool showValue = true,
                                const QString& valueLabel = {});
}
