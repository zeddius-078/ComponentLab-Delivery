#pragma once
#include <QString>

class QRadioButton;

namespace commonWidget {
// Opt-in captured Light appearance. Empty snapshotState follows live Qt input.
// This never changes text, checked, enabled, autoExclusive or the button group.
void applyFigmaRadioAppearance(QRadioButton* button, const QString& snapshotState = {},
                               bool showLabel = true);
}
