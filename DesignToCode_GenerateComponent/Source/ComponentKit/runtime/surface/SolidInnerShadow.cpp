#include "SolidInnerShadow.h"
#include "Surface.h"
#include <QJsonDocument>
#include <QLinearGradient>
#include <QPainter>
#include <QResizeEvent>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace surface::solid_inner_shadow {
namespace {
qreal clamp(qreal value, qreal low, qreal high) {
    return std::max(low, std::min(high, value));
}
QGradientStops suppliedGradient(const QJsonObject& visual) {
    const auto values = visual["backgroundStops"].toArray();
    if (values.size() < 2)
        throw std::invalid_argument("A background gradient needs two or more stops");
    QGradientStops result;
    qreal previous = -1;
    for (const auto& value : values) {
        const auto stop = value.toObject();
        const auto at = stop["at"].toDouble(-1);
        if (!std::isfinite(at) || at <= previous || at < 0 || at > 1)
            throw std::invalid_argument("Background stops must be ordered within 0..1");
        result.append({at, color(stop["color"].toString())});
        previous = at;
    }
    if (result.first().first != 0 || result.last().first != 1)
        throw std::invalid_argument("Background stops must cover 0..1");
    return result;
}
} // namespace
ArcProfile arcProfile(qreal width, qreal height, qreal thickness, qreal radius, qreal fade) {
    const qreal topY = fade * height, bottomY = height - thickness;
    const qreal slope = thickness / std::max(height - topY, qreal(.001));
    const qreal pointX = slope * (bottomY - topY);
    ArcProfile base{topY, bottomY, slope, false, bottomY, pointX, bottomY, 0};
    if (thickness <= 0 || radius <= 0 || bottomY <= topY)
        return base;
    const qreal hyp = std::hypot(slope, 1), e1x = -slope / hyp, e1y = -1 / hyp;
    const qreal theta = std::acos(clamp(e1x, -1, 1));
    const qreal tanHalf = std::tan(theta / 2), sinHalf = std::sin(theta / 2);
    if (tanHalf <= .001 || sinHalf <= .001)
        return base;
    const qreal slopeLength = std::hypot(pointX, bottomY - topY);
    const qreal centerRoom = std::max(qreal(0), width / 2 - pointX);
    radius = std::min({radius, thickness, slopeLength * tanHalf, centerRoom * tanHalf});
    if (radius <= .001)
        return base;
    const qreal tangentDistance = radius / tanHalf;
    const qreal arcStartY = bottomY + tangentDistance * e1y;
    const qreal sumX = e1x + 1, sumY = e1y, sumLen = std::hypot(sumX, sumY);
    if (sumLen <= .001 || !(topY < arcStartY && arcStartY < bottomY))
        return base;
    base.useArc = true;
    base.arcStartY = arcStartY;
    base.centerX = pointX + (sumX / sumLen) * radius / sinHalf;
    base.centerY = bottomY + (sumY / sumLen) * radius / sinHalf;
    base.radius = radius;
    return base;
}
qreal profileX(qreal y, const ArcProfile& p) {
    if (y <= p.topY)
        return 0;
    if (!p.useArc || y < p.arcStartY)
        return p.slope * (y - p.topY);
    const qreal inside = p.radius * p.radius - (y - p.centerY) * (y - p.centerY);
    if (inside < 0)
        return p.slope * (y - p.topY);
    return p.centerX - std::sqrt(std::max(inside, qreal(0)));
}
QVector<QPointF> boundaryPoints(qreal width, qreal height, qreal thickness, qreal radius,
                                qreal fade) {
    const qreal topY = fade * height, span = height - thickness - topY;
    if (span <= 0)
        return {};
    const auto profile = arcProfile(width, height, thickness, radius, fade);
    const int steps = std::max(10, std::min(64, int(std::ceil(span / 2))));
    QVector<QPointF> points;
    for (int index = 0; index <= steps; ++index) {
        const qreal y = topY + span * index / steps;
        points.append(QPointF(clamp(profileX(y, profile), 0, width / 2), y));
    }
    return points;
}
QPainterPath sidePath(qreal width, qreal height, qreal thickness, qreal radius, qreal fade,
                      bool left) {
    QPainterPath path;
    const qreal topY = fade * height, bottomY = height - thickness;
    if (thickness <= 0 || bottomY <= topY)
        return path;
    const auto points = boundaryPoints(width, height, thickness, radius, fade);
    const qreal edge = left ? 0 : width;
    path.moveTo(edge, topY);
    path.lineTo(edge, bottomY);
    for (auto i = points.crbegin(); i != points.crend(); ++i)
        path.lineTo(left ? i->x() : width - i->x(), i->y());
    path.closeSubpath();
    return path;
}
QPixmap renderBackground(QSize size, qreal dpr, const QJsonObject& g, const QJsonObject& v) {
    if (!std::isfinite(dpr) || dpr <= 0 || dpr > 8 || size.width() > 8192 || size.height() > 8192)
        throw std::invalid_argument("Invalid inner-shadow raster size/DPR");
    const int width = std::max(1, size.width()), height = std::max(1, size.height());
    // nearbyint preserves Python round's ties-to-even physical-pixel allocation.
    const QSize physical(std::max(1, int(std::nearbyint(width * dpr))),
                         std::max(1, int(std::nearbyint(height * dpr))));
    if (qint64(physical.width()) * physical.height() > 16 * 1024 * 1024)
        throw std::invalid_argument("Inner-shadow raster exceeds 16 MP");
    QPixmap pixmap(physical);
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);
    const qreal half = std::min(width, height) / 2.;
    const qreal thickness = clamp(g["width"].toDouble(), 0, half);
    const qreal radius = clamp(g["cornerRadius"].toDouble(), 0, thickness);
    const qreal fade = clamp(g["fadeStartRatio"].toDouble(), 0, .95);
    const qreal outerSource = g["matchButtonRadiusToInnerShadow"].toBool()
                                  ? std::max(g["width"].toDouble(), g["cornerRadius"].toDouble())
                                  : g["buttonRadius"].toDouble();
    const qreal outer = clamp(outerSource, 0, half);
    const qreal border = clamp(g["borderWidth"].toDouble(), 0, thickness);
    const qreal bottomTop = height - thickness;
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    QPainterPath outerPath;
    outerPath.addRoundedRect(QRectF(0, 0, width, height), outer, outer);
    painter.setClipPath(outerPath);
    QLinearGradient gradient(0, 0, 0, height);
    if (v.contains("backgroundStops")) {
        gradient.setStops(suppliedGradient(v));
    } else {
        gradient.setColorAt(0, color(v["bgTop"].toString()));
        gradient.setColorAt(clamp(v["bgMidStop"].toDouble(), 0, 1), color(v["bgMid"].toString()));
        gradient.setColorAt(1, color(v["bgBottom"].toString()));
    }
    painter.fillPath(outerPath, QBrush(gradient));
    if (thickness > 0 && bottomTop > 0) {
        const auto shadow = color(v["shadowColor"].toString());
        painter.fillPath(sidePath(width, height, thickness, radius, fade, true), QBrush(shadow));
        painter.fillPath(sidePath(width, height, thickness, radius, fade, false), QBrush(shadow));
        painter.fillRect(QRectF(0, bottomTop, width, thickness), shadow);
    }
    if (border > 0) {
        const auto style = v["borderStyle"] == "dashed" ? Qt::DashLine : Qt::SolidLine;
        painter.setPen(QPen(color(v["borderColor"].toString()), border, style));
        painter.setBrush(Qt::NoBrush);
        const qreal inset = border / 2;
        painter.drawRoundedRect(QRectF(inset, inset, width - border, height - border),
                                std::max(qreal(0), outer - inset),
                                std::max(qreal(0), outer - inset));
    }
    painter.end();
    return pixmap;
}
Layer::Layer(QWidget* parent) : QWidget(parent) {
    setObjectName("SOLID_INNER_SHADOW");
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::NoFocus);
}
void Layer::setTokens(const QJsonObject& g, const QJsonObject& v, const QString& state) {
    if (!QStringList{"default", "hover", "pressed", "disabled"}.contains(state))
        throw std::invalid_argument("Unknown inner-shadow state");
    for (const char* key :
         {"width", "cornerRadius", "fadeStartRatio", "buttonRadius", "borderWidth"})
        if (!g[key].isDouble() || !std::isfinite(g[key].toDouble()))
            throw std::invalid_argument("Missing/non-finite inner-shadow geometry");
    if (!g["matchButtonRadiusToInnerShadow"].isBool() || !v["bgMidStop"].isDouble() ||
        !std::isfinite(v["bgMidStop"].toDouble()))
        throw std::invalid_argument("Invalid inner-shadow switch/gradient stop");
    for (const char* key :
         {"shadowColor", "borderColor", "textColor", "bgTop", "bgMid", "bgBottom"})
        color(v[key].toString());
    if (v.contains("backgroundStops"))
        suppliedGradient(v);
    if (g != geometry)
        clearCache();
    geometry = g;
    visual = v;
    currentState = state;
    update();
}
void Layer::clearCache() {
    cache.clear();
    stateKeys.clear();
    update();
}
QPixmap Layer::backgroundPixmap(QSize size, qreal dpr) {
    const auto key = QByteArray::number(size.width()) + '/' + QByteArray::number(size.height()) +
                     '/' + QByteArray::number(dpr, 'g', 17) + '/' + currentState.toUtf8() + '/' +
                     QJsonDocument(geometry).toJson(QJsonDocument::Compact) +
                     QJsonDocument(visual).toJson(QJsonDocument::Compact);
    if (!cache.contains(key)) {
        cache.remove(stateKeys.value(currentState));
        cache.insert(key, renderBackground(size, dpr, geometry, visual));
        stateKeys[currentState] = key;
        ++renders;
    }
    return cache.value(key);
}
void Layer::paintEvent(QPaintEvent*) {
    if (geometry.isEmpty() || visual.isEmpty())
        return;
    QPainter painter(this);
    painter.drawPixmap(0, 0, backgroundPixmap(size(), devicePixelRatioF()));
}
void Layer::resizeEvent(QResizeEvent* event) {
    clearCache();
    QWidget::resizeEvent(event);
}
} // namespace surface::solid_inner_shadow
