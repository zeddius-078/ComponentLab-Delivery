#pragma once
#include <QMap>
#include <QString>
class QAbstractSpinBox;

namespace commonWidget {
// Applies the public Figma 458:155 / 460:175 Controls x State contract to an
// existing QSpinBox or QDoubleSpinBox. Optional affix visibility changes only
// native prefix/suffix presentation; raw value, bounds, step, decimals, locale,
// validation, wrapping and keyboard tracking remain Qt-owned.
void configureFigmaSpinBox(QAbstractSpinBox* spin,
                           const QMap<QString, QString>& properties);
// Captured Light chrome over the existing native spin box and its existing editor.
// SplitPlusMinus repositions Qt's Up/Down subcontrols; it is not a new Qt enum.
// Value, range, decimals, locale, validation, repeat, keyboard and accessibility remain Qt-owned.
// snapshotState is empty (live), Default or Focus; readOnly/enabled remain real native properties.
void applyFigmaSpinBoxAppearance(QAbstractSpinBox* spin, const QString& controls = "UpDown",
                                 const QString& snapshotState = {});
}
