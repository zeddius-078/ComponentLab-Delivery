#pragma once

#include <QSplitter>

namespace commonWidget {

// Token-backed splitter. The root is deliberately not focusable: keyboard
// focus belongs to interactive children hosted in its panes.
class Z_Splitter : public QSplitter {
    Q_OBJECT
    Q_PROPERTY(bool showGrip READ showGrip WRITE setShowGrip NOTIFY showGripChanged)
  public:
    explicit Z_Splitter(QWidget* parent = nullptr);
    bool showGrip() const { return showGripValue; }
    void setShowGrip(bool value);
    QSize sizeHint() const override;
  signals:
    void showGripChanged(bool value);
  protected:
    QSplitterHandle* createHandle() override;
  private:
    bool showGripValue = true;
};

} // namespace commonWidget
