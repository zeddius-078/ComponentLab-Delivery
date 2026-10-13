#pragma once
#include <QString>
class QDockWidget;

namespace commonWidget {
// Preview parameters never change QMainWindow placement, features or floating state.
// Empty placement follows the native dock state; Area is ignored while floating.
void applyFigmaDockWidgetAppearance(QDockWidget* dock, const QString& area = {},
                                    const QString& placement = {}, bool showCloseGlyph = true,
                                    bool showFloatGlyph = true);
}
