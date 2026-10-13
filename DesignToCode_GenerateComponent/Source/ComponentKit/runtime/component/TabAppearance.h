#pragma once
#include <QIcon>
#include <QString>
class QTabBar;
class QTabWidget;

namespace commonWidget {
QIcon figmaTabDocumentIcon(bool composition = false);
// Fixed states affect painting only, never currentIndex/focus/enabled/ownership.
void applyFigmaTabBarAppearance(QTabBar* bar, const QString& snapshotState = {},
                                int previewIndex = -1, bool sourceSnapshot = false);
void applyFigmaTabWidgetAppearance(QTabWidget* tabs, const QString& snapshotState = {},
                                   int previewIndex = -1, bool sourceSnapshot = false,
                                   bool anatomyOnly = false);
} // namespace commonWidget
