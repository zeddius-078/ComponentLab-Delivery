#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

class QWidget;
class QAction;
class QMenu;
class QMenuBar;
class QToolBar;

namespace commonWidget::navigation {
// Visual adaptation of the Figma navigation contract. Native Qt owns actions,
// popup lifecycle, keyboard input, selection and docking. Preview state is
// explicitly separate from writable runtime properties.
void install(QWidget* widget);
void setPreviewState(QAction* action, const QString& state);
void populateMenu(QMenu* menu);
void configureMenuItem(QMenu* menu, const QString& type, const QString& state);
void populateMenuBar(QMenuBar* bar, const QStringList& labels = {});
void configureToolBar(QToolBar* bar, const QMap<QString, QString>& properties);
} // namespace commonWidget::navigation
