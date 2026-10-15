#pragma once
#include <QIcon>
#include <QMap>
#include <QString>
class QToolButton;

namespace commonWidget {
// Captured source Light appearance; keeps native QAction, input and popup authority.
QIcon figmaToolButtonIcon();
// Applies the complete public Figma 443:222 variant contract. Runtime QAction,
// QMenu and popup ownership stay native; Open is accepted only for Split.
void configureFigmaToolButton(QToolButton* button,
                              const QMap<QString, QString>& properties,
                              bool sourceSnapshot = false);
void applyFigmaToolButtonAppearance(QToolButton* button, const QString& snapshotState = {},
                                    bool sourceSnapshot = false);
}
