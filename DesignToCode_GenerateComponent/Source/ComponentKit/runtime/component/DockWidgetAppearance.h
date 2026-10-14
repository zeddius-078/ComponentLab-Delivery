#pragma once
#include <QMap>
#include <QString>
class QDockWidget;

namespace commonWidget {
// Rebuild the public ComponentLab/DockWidget child composition. This deletes the
// previous child widget and must only be used for a new design specimen.
void populateFigmaDockWidget(QDockWidget* dock, const QString& caption,
                             const QString& title = QStringLiteral("Inspector"));
// Applies the governed Figma Area/Placement preview and SDK-owned composition.
// It never changes QMainWindow placement, floating, allowedAreas or feature flags.
void configureFigmaDockWidget(QDockWidget* dock, const QMap<QString, QString>& properties);
// Preview parameters never change QMainWindow placement, features or floating state.
// Empty placement follows the native dock state; Area is ignored while floating.
void applyFigmaDockWidgetAppearance(QDockWidget* dock, const QString& area = {},
                                    const QString& placement = {}, bool showCloseGlyph = true,
                                    bool showFloatGlyph = true);
}
