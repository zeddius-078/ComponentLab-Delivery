#pragma once
class QStackedWidget;
class QWizardPage;
class QWizard;

namespace commonWidget {
// Paint adapters only: page ownership, indices, fields, flags and lifecycle stay native.
void applyFigmaStackedWidgetAppearance(QStackedWidget* stack);
void applyFigmaWizardPageAppearance(QWizardPage* page);
void applyFigmaWizardAppearance(QWizard* wizard);
}
