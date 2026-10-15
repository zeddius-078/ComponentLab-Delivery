#pragma once
#include <QMap>
#include <QString>
class QLineEdit;

namespace commonWidget {
// Applies the public Figma 1116:18 Size x State contract. Filled remains
// content-derived and Error remains an explicit visual snapshot; neither is
// inferred from validators. Optional action booleans use native QAction slots.
void configureFigmaLineEdit(QLineEdit* edit, const QMap<QString, QString>& properties);
// Standalone native QLineEdit appearance, not the semantic Z_InputField API.
// Error is a presentation preview, never inferred from validator acceptability.
void applyFigmaLineEditAppearance(QLineEdit* edit, const QString& size = "Medium",
                                  const QString& snapshotState = {});
// Optional source glyphs attached through native QAction slots. Clear visibility
// and behavior remain QLineEdit::clearButtonEnabled, not this adapter.
void applyFigmaLineEditActions(QLineEdit* edit, bool showPrefix, bool showSuffix);
}
