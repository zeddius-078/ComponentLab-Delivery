#pragma once
#include <QGraphicsEffect>
#include <QJsonObject>
#include <QJsonArray>
#include <QColor>
#include <QWidget>

namespace surface {
// Generated recipes contain resolved values only. No token alias lookup in widgets.
struct Recipe {
    QString qss;
    QColor foreground;
    QColor glyph;
    qreal radius = 0;
    qreal borderWidth = 0;
    QJsonArray shadows;
    QColor iconFilledForeground;
};
class Catalog {
  public:
    explicit Catalog(const QString& profile);
    QStringList presetIds() const;
    QString label(const QString& preset) const;
    Recipe checkbox(const QString& preset, bool selected, const QString& state) const;
    QJsonObject checkboxMetrics(const QString& size) const;
    QColor focusColor() const;
    qreal focusWidth() const;
    qreal focusGap() const;
    QString fontFamily() const;
    QString glyphSource(bool mixed) const;
    QString sourceHash() const;

  private:
    QJsonObject data;
    Recipe recipe(const QString& key) const;
};
class InsetLayer : public QWidget {
  public:
    explicit InsetLayer(QWidget* parent);
    void setRecipe(const Recipe& recipe);

  protected:
    void paintEvent(QPaintEvent*) override;

  private:
    Recipe value;
};
class OuterShadowEffect : public QGraphicsEffect {
  public:
    explicit OuterShadowEffect(QObject* parent = nullptr);
    void setRecipe(const Recipe& recipe);
    void setSurfaceRect(const QRectF& rect);

  protected:
    QRectF boundingRectFor(const QRectF& rect) const override;
    void draw(QPainter* painter) override;

  private:
    Recipe value;
    QRectF surfaceRect;
};
// QImage caches use physical pixels and DPR in the key; no blur is recomputed per frame.
QImage shadowImage(QSize size, qreal dpr, qreal radius, qreal borderWidth,
                   const QJsonObject& shadow, bool inset, int* padding = nullptr);
QColor color(const QString& value);
} // namespace surface
