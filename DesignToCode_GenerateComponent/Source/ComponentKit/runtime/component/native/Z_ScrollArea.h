#pragma once

#include <QScrollArea>

namespace commonWidget {

// Token-backed ScrollArea identity. Bars and resize behavior are explicit
// Figma axes; content ownership and scrolling remain native QScrollArea APIs.
class Z_ScrollArea : public QScrollArea {
    Q_OBJECT
    Q_PROPERTY(QString bars READ bars WRITE setBars NOTIFY barsChanged)
    Q_PROPERTY(QString resizeMode READ resizeMode WRITE setResizeMode NOTIFY resizeModeChanged)
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
  public:
    explicit Z_ScrollArea(QWidget* parent = nullptr);
    QString bars() const { return barsValue; }
    QString resizeMode() const { return resizeModeValue; }
    QString snapshotState() const { return snapshotStateValue; }
    void setBars(const QString& value);
    void setResizeMode(const QString& value);
    void setSnapshotState(const QString& value);
    QSize sizeHint() const override;
  signals:
    void barsChanged(const QString& value);
    void resizeModeChanged(const QString& value);
    void snapshotStateChanged(const QString& value);
  private:
    void refreshContract();
    QString barsValue = QStringLiteral("None");
    QString resizeModeValue = QStringLiteral("Fixed");
    QString snapshotStateValue = QStringLiteral("Default");
};

} // namespace commonWidget
