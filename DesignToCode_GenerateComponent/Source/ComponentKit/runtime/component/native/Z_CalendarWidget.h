#pragma once

#include <QCalendarWidget>

namespace commonWidget {

// Token-backed Calendar identity. WeekStart, Grid and State mirror the
// public Figma component-set axes while date selection remains native Qt API.
class Z_CalendarWidget : public QCalendarWidget {
    Q_OBJECT
    Q_PROPERTY(QString weekStart READ weekStart WRITE setWeekStart NOTIFY weekStartChanged)
    Q_PROPERTY(bool grid READ grid WRITE setGrid NOTIFY gridChanged)
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
  public:
    explicit Z_CalendarWidget(QWidget* parent = nullptr);
    QString weekStart() const { return weekStartValue; }
    bool grid() const { return gridValue; }
    QString snapshotState() const { return snapshotStateValue; }
    void setWeekStart(const QString& value);
    void setGrid(bool value);
    void setSnapshotState(const QString& value);
    QSize sizeHint() const override;
  signals:
    void weekStartChanged(const QString& value);
    void gridChanged(bool value);
    void snapshotStateChanged(const QString& value);
  private:
    void refreshContract();
    QString weekStartValue = QStringLiteral("Monday");
    bool gridValue = true;
    QString snapshotStateValue = QStringLiteral("Default");
};

} // namespace commonWidget
