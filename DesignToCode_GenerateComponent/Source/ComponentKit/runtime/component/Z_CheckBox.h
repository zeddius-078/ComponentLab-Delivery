#pragma once
#include "Surface.h"
#include <QCheckBox>
#include <memory>

class QFrame;
class QLabel;
namespace commonWidget {
class Z_CheckBox : public QCheckBox {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(
        bool indeterminate READ isIndeterminate WRITE setIndeterminate NOTIFY indeterminateChanged)
    Q_PROPERTY(bool controlled READ isControlled WRITE setControlled)
    Q_PROPERTY(QString appearance READ appearance WRITE setAppearance NOTIFY appearanceChanged)
    Q_PROPERTY(QString controlSize READ controlSize WRITE setControlSize)
    Q_PROPERTY(QString interactionState READ interactionState)
  public:
    explicit Z_CheckBox(std::shared_ptr<const surface::Catalog> catalog, const QString& text = {},
                        QWidget* parent = nullptr);
    bool isIndeterminate() const {
        return mixed;
    }
    bool isControlled() const {
        return controlled;
    }
    QString appearance() const {
        return preset;
    }
    QString controlSize() const {
        return sizeName;
    }
    QString interactionState() const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    QRect indicatorRect() const;
  public slots:
    void setText(const QString& text);
    void setIndeterminate(bool value);
    void setControlled(bool value) {
        controlled = value;
    }
    void setAppearance(const QString& id);
    void setControlSize(const QString& size);
  signals:
    // A controlled owner commits via setChecked(); no speculative local state.
    void checkedChangeRequested(bool checked);
    void indeterminateChanged(bool value);
    void appearanceChanged(const QString& id);

  protected:
    void nextCheckState() override;
    bool hitButton(const QPoint& pos) const override {
        return rect().contains(pos);
    }
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;

  private:
    std::shared_ptr<const surface::Catalog> catalog;
    QFrame* face;
    QLabel* label;
    QLabel* glyph;
    surface::InsetLayer* inset;
    surface::OuterShadowEffect* outer;
    bool mixed = false, controlled = false, hovering = false, ready = false;
    QString preset = "solid", sizeName = "sm";
    QIcon checkIcon, mixedIcon;
    void refresh();
    void arrange();
};
} // namespace commonWidget
