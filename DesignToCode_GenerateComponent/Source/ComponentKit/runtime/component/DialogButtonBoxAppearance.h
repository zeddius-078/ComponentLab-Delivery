#pragma once
#include <QMap>
#include <QString>
class QDialogButtonBox;

namespace commonWidget {
// Presentation only: never replaces native standard buttons or changes roles/signals/order.
// Call after changing the button composition; new native children are also discovered on events.
void applyFigmaDialogButtonBoxAppearance(QDialogButtonBox* box);
// Applies the complete public Figma 447:593 variant contract while retaining
// QDialogButtonBox roles, accepted/rejected/help signals and native ordering.
void configureFigmaDialogButtonGroup(QDialogButtonBox* box,
                                     const QMap<QString, QString>& properties);
} // namespace commonWidget
