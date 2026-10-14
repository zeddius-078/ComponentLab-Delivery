#pragma once
#include <QMap>
#include <QString>
class QStackedWidget;
class QWizardPage;
class QWizard;

namespace commonWidget {
// Rebuilds SDK-owned specimen pages. Use only while initializing a new stack.
void populateFigmaStackedWidget(QStackedWidget* stack, int pageCount = 3);
void configureFigmaStackedWidget(QStackedWidget* stack,
                                 const QMap<QString, QString>& properties);
// Rebuilds the public page specimen. Kind maps to the explicit Qt page flags;
// Completion remains preview metadata and never overrides isComplete().
void configureFigmaWizardPage(QWizardPage* page,
                              const QMap<QString, QString>& properties);
// Rebuilds a four-page native wizard. Style maps to wizardStyle; Phase is
// preview metadata and never writes the read-only currentId.
void configureFigmaWizard(QWizard* wizard, const QMap<QString, QString>& properties);
// Paint adapters only: page ownership, indices, fields, flags and lifecycle stay native.
void applyFigmaStackedWidgetAppearance(QStackedWidget* stack);
void applyFigmaWizardPageAppearance(QWizardPage* page);
void applyFigmaWizardAppearance(QWizard* wizard);
}
