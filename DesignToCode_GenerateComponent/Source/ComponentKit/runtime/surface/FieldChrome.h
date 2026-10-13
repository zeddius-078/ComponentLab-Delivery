#pragma once
#include "Surface.h"
#include <QFrame>
#include <QJsonObject>
#include "Z_PushButton.h"
#include <memory>

namespace field {
// Read-only captured Input design evidence, separate from the legacy profile recipes.
const QJsonObject& inputVisuals();
QPixmap iconPixmap(const QIcon&, int logicalSize, qreal dpr, const QColor&);
class Tokens {
  public:
    explicit Tokens(const QString& profile);
    int metric(const QString& key) const;
    qreal scalar(const QString& key) const;
    QJsonObject size(const QString& name) const;
    QColor color(const QString& key) const;
    QString colorText(const QString& key) const;
    QString family() const;
    QString iconSource(const QString& key) const;
    QString sourceHash() const;
    surface::Recipe recipe(const QString& preset, const QString& variant,
                           const QString& state) const;
    QString qss(const surface::Recipe&, const QString& size) const;
    const QJsonObject& data() const {
        return values;
    }

  private:
    QJsonObject values;
    QString styleTemplate;
};
class Chrome : public QFrame {
    Q_OBJECT
  public:
    explicit Chrome(std::shared_ptr<const Tokens>, QWidget* parent = nullptr);
    ~Chrome() override;
    void setControlSize(const QString&);
    void setVariant(const QString&);
    void setStatus(const QString&);
    void setAppearance(const QString&);
    void setFigmaInputAppearance(bool);
    void setSnapshotState(const QString&);
    bool isFigmaInputAppearance() const;
    void setControlHeight(int height);
    int controlHeight() const;
    QString controlSize() const {
        return sizeName;
    }
    QString visualState() const;
    const Tokens& tokens() const {
        return *catalog;
    }
  signals:
    void visualChanged();

  protected:
    bool event(QEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void paintEvent(QPaintEvent*) override;

  private:
    std::shared_ptr<const Tokens> catalog;
    QString sizeName = "md", variant = "outlined", status, appearance = "solid";
    bool hovered = false;
    bool figmaInput = false;
    QString snapshotState;
    surface::InsetLayer* inset;
    surface::OuterShadowEffect* shadow;
    void refresh();
};
// Input/Select actions use the same Button core, Token QSS, icon loader and Loading policy.
class Action : public commonWidget::Z_PushButton {
    Q_OBJECT
  public:
    explicit Action(std::shared_ptr<const Tokens>, QWidget* parent = nullptr);
    void setBusy(bool);
    void setIconSource(const QString&);
    // Internal Input anatomy: original 16px SVG inside the authored 32px hit slot.
    void setFigmaGlyph(const QString& glyph);
    bool isBusy() const {
        return property("loading").toBool();
    }
  protected:
    void paintEvent(QPaintEvent*) override;
  private:
    QString figmaGlyph;
};
} // namespace field
