#pragma once
#include <QMap>
#include <QString>
class QMdiSubWindow;
class QMdiArea;

namespace commonWidget {
// Rebuilds the public ComponentLab/MdiSubWindow demo content. The previous
// child widget is deleted, so call this only while creating a new specimen.
void populateFigmaMdiSubWindow(QMdiSubWindow* window,
                               const QString& title = QStringLiteral("Trend"));
// Applies one governed Figma State preview and SDK-owned content. State remains
// paint metadata: it never calls showMaximized/showMinimized/showShaded,
// changes enabled, or rewrites Qt::WindowFlags.
void configureFigmaMdiSubWindow(QMdiSubWindow* window,
                                const QMap<QString, QString>& properties);
// These are paint previews only, never native window-state/activation commands.
void applyFigmaMdiSubWindowAppearance(QMdiSubWindow* window, const QString& snapshot = {},
                                      bool closeGlyph = true, bool minimizeGlyph = true,
                                      bool maximizeGlyph = true);
// Rebuilds a deterministic public MDI specimen for Presentation. Free,
// Cascade and Tile use authored geometry rather than invoking arrangement
// commands; Tabbed maps to the native TabbedView. Existing managed windows are
// deleted, so this is an initialization API rather than a runtime controller.
void configureFigmaMdiArea(QMdiArea* area, const QMap<QString, QString>& properties);
// Preserves the QMdiArea model, native tab bar, managed windows and close-event veto.
void applyFigmaMdiAreaAppearance(QMdiArea* area);
}
