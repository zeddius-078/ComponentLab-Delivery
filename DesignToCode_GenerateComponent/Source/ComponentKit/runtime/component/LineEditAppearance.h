#pragma once
#include <QString>
class QLineEdit;

namespace commonWidget {
// Standalone native QLineEdit appearance, not the semantic Z_InputField API.
// Error is a presentation preview, never inferred from validator acceptability.
void applyFigmaLineEditAppearance(QLineEdit* edit, const QString& size = "Medium",
                                  const QString& snapshotState = {});
// Optional source glyphs attached through native QAction slots. Clear visibility
// and behavior remain QLineEdit::clearButtonEnabled, not this adapter.
void applyFigmaLineEditActions(QLineEdit* edit, bool showPrefix, bool showSuffix);
}
