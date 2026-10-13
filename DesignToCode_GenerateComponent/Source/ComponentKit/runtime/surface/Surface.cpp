#include "Surface.h"
#include "TokenResources.h"
#include <QCache>
#include <QFile>
#include <QGraphicsBlurEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QJsonDocument>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QtMath>
#include <stdexcept>

namespace surface {
QColor color(const QString& value) {
    auto m = QRegularExpression("^rgba\\((\\d+), (\\d+), (\\d+), (\\d+)\\)$").match(value);
    QColor c = m.hasMatch() ? QColor(m.captured(1).toInt(), m.captured(2).toInt(),
                                     m.captured(3).toInt(), m.captured(4).toInt())
                            : QColor(value);
    if (!c.isValid())
        throw std::invalid_argument(("Invalid compiled color: " + value).toStdString());
    return c;
}
Catalog::Catalog(const QString& profile) {
    data = QJsonDocument::fromJson(token_resources::read(":/checkbox/" + profile + "/catalog.json"))
               .object();
    if (data["recipes"].toObject().isEmpty())
        throw std::runtime_error("Empty Surface catalog");
}
QStringList Catalog::presetIds() const {
    QStringList r;
    for (auto v : data["presets"].toArray())
        r << v.toObject()["id"].toString();
    return r;
}
QString Catalog::label(const QString& preset) const {
    const auto solid = data["buttonInnerShadow"].toObject();
    if (solid["id"] == preset)
        return solid["label"].toString();
    for (auto v : data["presets"].toArray())
        if (v.toObject()["id"] == preset)
            return v.toObject()["label"].toString();
    throw std::invalid_argument("Unknown preset");
}
QStringList Catalog::buttonPresetIds() const {
    auto ids = presetIds();
    const auto id = data["buttonInnerShadow"].toObject()["id"].toString();
    if (!id.isEmpty())
        ids << id;
    return ids;
}
Recipe Catalog::recipe(const QString& key) const {
    auto recipes = data["recipes"].toObject();
    if (!recipes.contains(key))
        throw std::invalid_argument(("Missing recipe " + key).toStdString());
    auto r = recipes[key].toObject();
    return {
        r["qss"].toString(),
        color(r["foreground"].toString()),
        color(r["glyph"].toString()),
        r["radius"].toDouble(),
        r["borderWidth"].toDouble(),
        r["shadows"].toArray(),
        color(r[r.contains("iconFilledForeground") ? "iconFilledForeground" : "glyph"].toString())};
}
Recipe Catalog::checkbox(const QString& preset, bool selected, const QString& state) const {
    return recipe("checkbox/" + preset + '/' + (selected ? "selected" : "unchecked") + '/' + state);
}
QString Catalog::buttonUnsupportedReason(const QString& preset, const QString& type,
                                         bool ghost) const {
    if(preset=="figma-light") return ghost?"ghost-not-in-captured-figma":QString();
    const auto capabilities = data["buttonCapabilities"].toObject()[preset].toObject();
    if (capabilities.isEmpty())
        return "missing-material-capability";
    if (!capabilities["types"].toArray().contains(type))
        return "material-requires-container";
    if (ghost && !capabilities["ghost"].toBool())
        return "ghost-conflicts-with-material";
    return {};
}
Recipe Catalog::button(const QString& preset, const QString& type, bool danger, bool ghost,
                       const QString& state, const QString& size) const {
    const auto solid = data["buttonInnerShadow"].toObject();
    if (solid["id"] == preset) {
        auto result = button("solid", type, danger, ghost, state, size);
        if (!ghost) {
            const auto role = danger                                ? "danger"
                              : type == "PRIMARY" || type == "LINK" ? "primary"
                                                                    : "secondary";
            auto visual = solid["roleStates"].toObject()[role].toObject()[state].toObject();
            if (visual.isEmpty())
                throw std::invalid_argument("Missing solid inner-shadow state");
            visual["borderStyle"] = type == "DASHED" ? "dashed" : "solid";
            result.solidInnerShadow = {
                {"geometry", solid["geometry"]}, {"visual", visual}, {"role", role}};
            result.foreground = color(visual["textColor"].toString());
            result.glyph = result.foreground;
            result.shadows = {};
            result.borderWidth = 0;
            result.qss = "background:transparent; border:none;";
        }
        return result;
    }
    auto result = recipe(QString("button/%1/%2/%3/%4/%5")
                             .arg(preset, type, danger ? "1" : "0", ghost ? "1" : "0", state));
    auto widths = data["buttonBorderWidths"].toObject();
    if (!widths.contains(size))
        throw std::invalid_argument("Missing compiled button border width");
    result.borderWidth = widths[size].toDouble();
    return result;
}
QJsonObject Catalog::checkboxMetrics(const QString& size) const {
    auto r = data["sizes"].toObject()[size].toObject();
    if (r.isEmpty())
        throw std::invalid_argument("Unknown checkbox size");
    return r;
}
QColor Catalog::focusColor() const {
    return color(data["focusColor"].toString());
}
qreal Catalog::focusWidth() const {
    return data["focusWidth"].toDouble();
}
qreal Catalog::focusGap() const {
    return data["focusGap"].toDouble();
}
QString Catalog::fontFamily() const {
    return data["fontFamily"].toString();
}
QString Catalog::glyphSource(bool mixed) const {
    return data["glyphs"].toObject()[mixed ? "mixed" : "checked"].toString();
}
QString Catalog::sourceHash() const {
    return data["sourceHash"].toString();
}

QImage shadowImage(QSize size, qreal dpr, qreal radius, qreal borderWidth,
                   const QJsonObject& shadow, bool inset, int* padding) {
    const qreal blur = shadow["blur"].toDouble(), x = shadow["x"].toDouble(),
                y = shadow["y"].toDouble(), spread = shadow["spread"].toDouble();
    const int pad = qCeil(2 * blur + qMax(qAbs(x), qAbs(y)) + qAbs(spread) + 2);
    if (padding)
        *padding = pad;
    if (size.isEmpty() || size.width() > 8192 || size.height() > 8192 || dpr <= 0 || dpr > 8)
        return {};
    const auto key = QString("%1/%2/%3/%4/%5/%6/")
                         .arg(size.width())
                         .arg(size.height())
                         .arg(dpr, 0, 'g', 12)
                         .arg(radius)
                         .arg(borderWidth)
                         .arg(inset) +
                     QString::fromUtf8(QJsonDocument(shadow).toJson(QJsonDocument::Compact));
    static QCache<QString, QImage> cache(32 * 1024);
    if (auto* image = cache.object(key))
        return *image;
    const QSize physical(qCeil((size.width() + 2 * pad) * dpr),
                         qCeil((size.height() + 2 * pad) * dpr));
    if (qint64(physical.width()) * physical.height() > 16 * 1024 * 1024)
        return {};
    QImage mask(physical, QImage::Format_ARGB32_Premultiplied);
    mask.fill(inset ? Qt::white : Qt::transparent);
    QRectF face(pad, pad, size.width(), size.height());
    {
        QPainter p(&mask);
        p.scale(dpr, dpr);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        if (inset)
            p.setCompositionMode(QPainter::CompositionMode_Clear);
        p.setBrush(Qt::white);
        const qreal delta = inset ? -spread : spread;
        const QRectF shape = face.adjusted(-delta, -delta, delta, delta).translated(x, y);
        p.drawRoundedRect(shape, qMax(qreal(0), radius + delta), qMax(qreal(0), radius + delta));
    }
    QImage blurred = mask;
    if (blur > 0) {
        QGraphicsScene scene;
        auto* item = scene.addPixmap(QPixmap::fromImage(mask));
        auto* effect = new QGraphicsBlurEffect;
        effect->setBlurRadius(blur * dpr);
        effect->setBlurHints(QGraphicsBlurEffect::QualityHint);
        item->setGraphicsEffect(effect);
        blurred = QImage(physical, QImage::Format_ARGB32_Premultiplied);
        blurred.fill(Qt::transparent);
        QPainter p(&blurred);
        scene.render(&p, QRectF(QPointF(), physical), QRectF(QPointF(), physical),
                     Qt::IgnoreAspectRatio);
    }
    QImage clip(physical, QImage::Format_ARGB32_Premultiplied);
    clip.fill(inset ? Qt::transparent : Qt::white);
    {
        QPainter p(&clip);
        p.scale(dpr, dpr);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        if (!inset)
            p.setCompositionMode(QPainter::CompositionMode_Clear);
        const qreal b = inset ? borderWidth : 0;
        p.drawRoundedRect(face.adjusted(b, b, -b, -b), qMax(qreal(0), radius - b),
                          qMax(qreal(0), radius - b));
    }
    {
        QColor tint = color(shadow["color"].toString());
        tint.setAlphaF(tint.alphaF() * shadow["alpha"].toDouble());
        QPainter p(&blurred);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(blurred.rect(), tint);
        p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        p.drawImage(0, 0, clip);
    }
    blurred.setDevicePixelRatio(dpr);
    cache.insert(key, new QImage(blurred), qMax(1, int(blurred.sizeInBytes() / 1024)));
    return blurred;
}
InsetLayer::InsetLayer(QWidget* parent) : QWidget(parent) {
    setObjectName("INSET_LAYER");
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
}
void InsetLayer::setRecipe(const Recipe& recipe) {
    value = recipe;
    update();
}
void InsetLayer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    for (auto s : value.shadows)
        if (s.toObject()["kind"] == "inset") {
            int pad = 0;
            auto image = shadowImage(size(), devicePixelRatioF(), value.radius, value.borderWidth,
                                     s.toObject(), true, &pad);
            p.drawImage(QPointF(-pad, -pad), image);
        }
}
OuterShadowEffect::OuterShadowEffect(QObject* parent) : QGraphicsEffect(parent) {}
void OuterShadowEffect::setRecipe(const Recipe& recipe) {
    value = recipe;
    bool enabled = false;
    for (auto s : value.shadows)
        enabled |= s.toObject()["kind"] == "outer";
    setEnabled(enabled);
    updateBoundingRect();
    update();
}
void OuterShadowEffect::setSurfaceRect(const QRectF& rect) {
    surfaceRect = rect;
    updateBoundingRect();
    update();
}
QRectF OuterShadowEffect::boundingRectFor(const QRectF& rect) const {
    qreal pad = 0;
    for (auto s : value.shadows)
        if (s.toObject()["kind"] == "outer") {
            auto o = s.toObject();
            pad = qMax(pad, 2 * o["blur"].toDouble() +
                                qMax(qAbs(o["x"].toDouble()), qAbs(o["y"].toDouble())) +
                                qAbs(o["spread"].toDouble()) + 2);
        }
    return rect.united((surfaceRect.isEmpty() ? rect : surfaceRect).adjusted(-pad, -pad, pad, pad));
}
void OuterShadowEffect::draw(QPainter* p) {
    const auto rect =
        surfaceRect.isEmpty() ? sourceBoundingRect(Qt::LogicalCoordinates) : surfaceRect;
    const qreal dpr = p->device()->devicePixelRatioF();
    for (auto s : value.shadows)
        if (s.toObject()["kind"] == "outer") {
            int pad = 0;
            auto image = shadowImage(rect.size().toSize(), dpr, value.radius, value.borderWidth,
                                     s.toObject(), false, &pad);
            p->drawImage(rect.topLeft() - QPointF(pad, pad), image);
        }
    drawSource(p);
}
} // namespace surface
