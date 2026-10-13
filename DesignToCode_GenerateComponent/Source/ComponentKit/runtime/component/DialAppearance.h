#pragma once
#include <QString>
class QDial;

namespace commonWidget {
// Snapshot geometry is for inert Figma-gallery specimens only. Live geometry
// preserves QDial's native angular input mapping, not the authored 270-degree arc.
// Null label follows sliderPosition; explicit text is independent composition.
void applyFigmaDialAppearance(QDial* dial, const QString& snapshotState = {},
                              bool snapshotGeometry = false, bool showValue = true,
                              const QString& valueLabel = {});
}
