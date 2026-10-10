#pragma once
#include <QHash>
#include <QJsonObject>
#include <QPainterPath>
#include <QPixmap>
#include <QVector>
#include <QWidget>

namespace surface::solid_inner_shadow {
// Direct port of PythonGUI solid_inner_shadow_button.py, not a blur/spread model.
struct ArcProfile {
    qreal topY, bottomY, slope;
    bool useArc;
    qreal arcStartY, centerX, centerY, radius;
};
ArcProfile arcProfile(qreal width, qreal height, qreal thickness, qreal radius, qreal fade);
qreal profileX(qreal y, const ArcProfile& profile);
QVector<QPointF> boundaryPoints(qreal width, qreal height, qreal thickness, qreal radius,
                                qreal fade);
QPainterPath sidePath(qreal width, qreal height, qreal thickness, qreal radius, qreal fade,
                      bool left);
QPixmap renderBackground(QSize size, qreal dpr, const QJsonObject& geometry,
                         const QJsonObject& visual);

class Layer : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString interactionState READ state)
  public:
    explicit Layer(QWidget* parent = nullptr);
    void setTokens(const QJsonObject& geometry, const QJsonObject& visual, const QString& state);
    void clearCache();
    QString state() const {
        return currentState;
    }
    QPixmap backgroundPixmap(QSize size, qreal dpr);
    int cacheEntries() const {
        return cache.size();
    }
    int renderCount() const {
        return renders;
    }

  protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;

  private:
    QJsonObject geometry, visual;
    QString currentState;
    QHash<QByteArray, QPixmap> cache;
    QHash<QString, QByteArray> stateKeys;
    int renders = 0;
};
} // namespace surface::solid_inner_shadow
