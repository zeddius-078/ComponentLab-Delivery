#pragma once

#include <QSplitter>

namespace commonWidget {

// Token-backed splitter. The root is deliberately not focusable: keyboard
// focus belongs to interactive children hosted in its panes.
class Z_Splitter : public QSplitter {
    Q_OBJECT
    Q_PROPERTY(bool showGrip READ showGrip WRITE setShowGrip NOTIFY showGripChanged)
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
  public:
    explicit Z_Splitter(QWidget* parent = nullptr);
    bool showGrip() const { return showGripValue; }
    void setShowGrip(bool value);
    QString snapshotState() const { return snapshotStateValue; }
    void setSnapshotState(const QString& value);
    QSize sizeHint() const override;
  signals:
    void showGripChanged(bool value);
    void snapshotStateChanged(const QString& value);
  protected:
    QSplitterHandle* createHandle() override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
  private:
    void refreshSnapshotState();
    bool showGripValue = true;
    QString snapshotStateValue = QStringLiteral("Default");
};

} // namespace commonWidget
