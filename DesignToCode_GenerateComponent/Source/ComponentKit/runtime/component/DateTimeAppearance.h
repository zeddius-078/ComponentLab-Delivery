#pragma once
#include <QMap>
#include <QString>
class QDateTimeEdit;
class QDateEdit;
class QTimeEdit;

namespace commonWidget {
// Applies the public Figma 464:111 Calendar x State contract without changing
// QDate, range, displayFormat, current section, locale or popup ownership.
void configureFigmaDatePicker(QDateEdit* edit, const QMap<QString, QString>& properties);
// Applies the public Figma 463:136 Calendar x State contract. Optional Show
// time / Show time spec properties control presentation only; the Time spec
// string is an inert annotation and never writes Qt::TimeSpec.
void configureFigmaDateTimePicker(QDateTimeEdit* edit,
                                  const QMap<QString, QString>& properties);
// Applies the public Figma 465:189 Format x State contract. Format changes
// preserve QTime, range and current section; locale and wall-clock semantics
// remain native Qt responsibilities.
void configureFigmaTimePicker(QTimeEdit* edit, const QMap<QString, QString>& properties);
// Captured Light rendering only. Values, ranges, displayFormat, calendarPopup,
// sections and timeSpec remain native Qt properties owned by the caller.
// previewTimeSpec is an inert Figma annotation, never a Qt::TimeSpec mapping.
void applyFigmaDateTimeAppearance(QDateTimeEdit* edit, const QString& snapshotState = {},
                                  const QString& previewTimeSpec = {});
// Presentation-only format changes must not erase hidden time sections or emit
// a transient midnight value (Qt 5.15 setDisplayFormat otherwise does both).
void setDateTimeDisplayFormatPreservingValue(QDateTimeEdit* edit, const QString& format);
}
