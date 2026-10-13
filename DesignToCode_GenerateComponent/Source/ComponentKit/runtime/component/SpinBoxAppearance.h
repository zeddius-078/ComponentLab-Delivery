#pragma once
#include <QString>
class QAbstractSpinBox;

namespace commonWidget {
// Captured Light chrome over the existing native spin box and its existing editor.
// SplitPlusMinus repositions Qt's Up/Down subcontrols; it is not a new Qt enum.
// Value, range, decimals, locale, validation, repeat, keyboard and accessibility remain Qt-owned.
// snapshotState is empty (live), Default or Focus; readOnly/enabled remain real native properties.
void applyFigmaSpinBoxAppearance(QAbstractSpinBox* spin, const QString& controls = "UpDown",
                                 const QString& snapshotState = {});
}
