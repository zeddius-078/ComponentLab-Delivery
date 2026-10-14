#pragma once
#include <QIcon>
#include <QString>
class QTabBar;
class QTabWidget;

namespace commonWidget {
QIcon figmaTabDocumentIcon(bool composition = false);
// Rebuild the public ComponentLab/TabWidget composition from the SDK-owned contract.
// This intentionally deletes existing pages; call it only for a new design specimen.
void populateFigmaTabWidget(QTabWidget* tabs, bool sourceSnapshot = false);
// Qt exposes closability at widget level. This helper keeps the Figma per-item
// contract explicit by removing close buttons from all tabs except Behavior.
void setFigmaTabClosePolicy(QTabWidget* tabs, bool closable, bool behaviorOnly = true);
// Fixed states affect painting only, never currentIndex/focus/enabled/ownership.
void applyFigmaTabBarAppearance(QTabBar* bar, const QString& snapshotState = {},
                                int previewIndex = -1, bool sourceSnapshot = false);
void applyFigmaTabWidgetAppearance(QTabWidget* tabs, const QString& snapshotState = {},
                                   int previewIndex = -1, bool sourceSnapshot = false,
                                   bool anatomyOnly = false);
} // namespace commonWidget
