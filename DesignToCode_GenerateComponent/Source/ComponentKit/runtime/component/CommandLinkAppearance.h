#pragma once
#include <QIcon>
#include <QMap>
#include <QString>
class QCommandLinkButton;

namespace commonWidget {
QIcon figmaCommandLinkIcon();
// Applies the complete public Figma 450:102 variant contract. Title and
// Description remain native QCommandLinkButton properties.
void configureFigmaCommandLinkButton(QCommandLinkButton* button,
                                     const QMap<QString, QString>& properties,
                                     bool sourceSnapshot = false);
// Rendering adapter only; the button retains every native action/property contract.
void applyFigmaCommandLinkAppearance(QCommandLinkButton* button, const QString& snapshotState = {},
                                     bool showDescription = true, bool showIcon = true,
                                     bool sourceSnapshot = false);
}
