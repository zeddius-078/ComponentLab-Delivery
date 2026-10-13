#pragma once
class QDialogButtonBox;

namespace commonWidget {
// Presentation only: never replaces native standard buttons or changes roles/signals/order.
// Call after changing the button composition; new native children are also discovered on events.
void applyFigmaDialogButtonBoxAppearance(QDialogButtonBox* box);
} // namespace commonWidget
