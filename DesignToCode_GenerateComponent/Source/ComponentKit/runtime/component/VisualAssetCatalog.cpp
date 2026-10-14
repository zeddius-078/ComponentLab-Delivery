#include "VisualAssetCatalog.h"

#include "TokenResources.h"

#include <QColor>
#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QSvgRenderer>
#include <QtMath>
#include <stdexcept>

namespace commonWidget {
namespace {

const QList<VisualAssetDescriptor>& catalog() {
    static const QList<VisualAssetDescriptor> value{
        {QStringLiteral("image.foundation.geometric-preview"),
         QStringLiteral("Geometric image preview"), VisualAssetKind::Image,
         QStringLiteral("SVG"),
         QStringLiteral("Project-owned vector authored from Figma node 2150:736"),
         QStringLiteral("Project-owned / repository"), QSize(230, 145), false, true,
         {QStringLiteral("Contain"), QStringLiteral("Cover")},
         {QStringLiteral("Loaded"), QStringLiteral("Empty"), QStringLiteral("Error")},
         {{QStringLiteral("Loaded"),
           QStringLiteral(":/assets/images/geometric-preview.svg")}}},
        {QStringLiteral("model.foundation.schematic"),
         QStringLiteral("2D schematic foundation"), VisualAssetKind::Model2D,
         QStringLiteral("SVG"),
         QStringLiteral("Project-owned vector authored from Figma nodes 2153:23-75"),
         QStringLiteral("Project-owned / repository"), QSize(310, 156), false, true,
         {QStringLiteral("Contain")},
         {QStringLiteral("Ready"), QStringLiteral("Loading"), QStringLiteral("Error")},
         {{QStringLiteral("Ready"),
           QStringLiteral(":/assets/models/foundation-schematic-ready.svg")},
          {QStringLiteral("Loading"),
           QStringLiteral(":/assets/models/foundation-schematic-loading.svg")},
          {QStringLiteral("Error"),
           QStringLiteral(":/assets/models/foundation-schematic-error.svg")}}},
    };
    return value;
}

[[noreturn]] void fail(const char* code, const QString& detail) {
    throw std::invalid_argument(
        QStringLiteral("%1: %2").arg(QString::fromLatin1(code), detail).toStdString());
}

QString defaultState(const VisualAssetDescriptor& descriptor) {
    return descriptor.kind == VisualAssetKind::Image ? QStringLiteral("Loaded")
                                                      : QStringLiteral("Ready");
}

QString sourceFor(const VisualAssetDescriptor& descriptor, const QString& requestedState) {
    const QString state = requestedState.trimmed().isEmpty()
        ? defaultState(descriptor) : requestedState.trimmed();
    if (!descriptor.supportedStates.contains(state))
        fail("UNMAPPED_RESOURCE", descriptor.id + QStringLiteral("/") + state);
    for (const auto& source : descriptor.stateSources)
        if (source.state == state)
            return source.qrcPath;
    fail("NO_ASSET_PAYLOAD", descriptor.id + QStringLiteral("/") + state);
}

QJsonObject profileColors(const QString& profile) {
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(
        token_resources::read(QStringLiteral(":/tokens/tokens_qt.json")), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        fail("INVALID_ASSET_TOKEN_SOURCE", profile);
    const auto profiles = document.object()[QStringLiteral("profiles")].toObject();
    const auto selected = profiles[profile].toObject();
    if (selected.isEmpty())
        fail("UNKNOWN_ASSET_PROFILE", profile);
    return selected[QStringLiteral("color")].toObject();
}

QString hexColor(const QJsonObject& colors, const QString& key) {
    const QString rgba = colors[key].toString();
    const auto start = rgba.indexOf('(');
    const auto end = rgba.indexOf(')');
    if (start < 0 || end <= start)
        fail("MISSING_ASSET_TOKEN", key);
    const auto channels = rgba.mid(start + 1, end - start - 1).split(',');
    if (channels.size() < 3)
        fail("INVALID_ASSET_TOKEN", key);
    return QColor(channels[0].trimmed().toInt(), channels[1].trimmed().toInt(),
                  channels[2].trimmed().toInt()).name(QColor::HexRgb);
}

QByteArray profiledSvg(const QString& path, const QString& profile) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() <= 0)
        fail("UNMAPPED_RESOURCE", path);
    QString svg = QString::fromUtf8(file.readAll());
    const auto colors = profileColors(profile);
    const QList<QPair<QString, QString>> replacements{
        {QStringLiteral("#6235f7"), hexColor(colors, QStringLiteral("base.stoken.color.interactive.brand.default"))},
        {QStringLiteral("#f3efff"), hexColor(colors, QStringLiteral("base.stoken.color.interactive.brand.subtle"))},
        {QStringLiteral("#d0d0d0"), hexColor(colors, QStringLiteral("base.stoken.color.border.secondary"))},
        {QStringLiteral("#f3f3f3"), hexColor(colors, QStringLiteral("base.stoken.color.background.surface"))},
        {QStringLiteral("#e02128"), hexColor(colors, QStringLiteral("base.stoken.color.status.danger.default"))},
    };
    for (const auto& replacement : replacements)
        svg.replace(replacement.first, replacement.second, Qt::CaseInsensitive);
    return svg.toUtf8();
}

}

QList<VisualAssetDescriptor> VisualAssetCatalog::descriptors() { return catalog(); }

QList<VisualAssetDescriptor> VisualAssetCatalog::imageDescriptors() {
    QList<VisualAssetDescriptor> result;
    for (const auto& item : catalog())
        if (item.kind == VisualAssetKind::Image)
            result.append(item);
    return result;
}

QList<VisualAssetDescriptor> VisualAssetCatalog::modelDescriptors() {
    QList<VisualAssetDescriptor> result;
    for (const auto& item : catalog())
        if (item.kind == VisualAssetKind::Model2D)
            result.append(item);
    return result;
}

bool VisualAssetCatalog::isSemanticId(const QString& value) {
    const QString key = value.trimmed();
    return key.startsWith(QStringLiteral("image.")) ||
           key.startsWith(QStringLiteral("model."));
}

bool VisualAssetCatalog::contains(const QString& id) {
    const QString key = id.trimmed();
    for (const auto& item : catalog())
        if (item.id == key)
            return true;
    return false;
}

const VisualAssetDescriptor& VisualAssetCatalog::descriptor(const QString& id) {
    const QString key = id.trimmed();
    for (const auto& item : catalog())
        if (item.id == key)
            return item;
    fail("UNMAPPED_RESOURCE", key);
}

QString VisualAssetCatalog::resolve(const QString& id, const QString& state) {
    const QString path = sourceFor(descriptor(id), state);
    if (!QFile::exists(path))
        fail("UNMAPPED_RESOURCE", id);
    return path;
}

QString VisualAssetCatalog::sha256(const QString& id, const QString& state) {
    QFile file(resolve(id, state));
    if (!file.open(QIODevice::ReadOnly))
        fail("UNMAPPED_RESOURCE", id);
    return QString::fromLatin1(
        QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256).toHex());
}

QPixmap VisualAssetCatalog::render(const QString& id, const QString& state,
                                   const QSize& logicalSize, const QString& fit,
                                   const QString& profile, qreal devicePixelRatio) {
    if (!logicalSize.isValid() || logicalSize.isEmpty() || devicePixelRatio <= 0)
        fail("INVALID_ASSET_RENDER_SIZE", id);
    if (fit != QStringLiteral("Contain") && fit != QStringLiteral("Cover"))
        fail("INVALID_ASSET_FIT", fit);
    const auto& item = descriptor(id);
    if (!item.supportedFits.contains(fit))
        fail("INVALID_ASSET_FIT", id + QStringLiteral("/") + fit);
    QSvgRenderer renderer(profiledSvg(resolve(id, state), profile));
    if (!renderer.isValid())
        fail("INVALID_ASSET_PAYLOAD", id);
    const QSize physical(qCeil(logicalSize.width() * devicePixelRatio),
                         qCeil(logicalSize.height() * devicePixelRatio));
    QPixmap result(physical);
    result.setDevicePixelRatio(devicePixelRatio);
    result.fill(Qt::transparent);
    const QSizeF intrinsic = item.intrinsicSize;
    const qreal scale = fit == QStringLiteral("Cover")
        ? qMax(logicalSize.width() / intrinsic.width(),
               logicalSize.height() / intrinsic.height())
        : qMin(logicalSize.width() / intrinsic.width(),
               logicalSize.height() / intrinsic.height());
    const QSizeF painted(intrinsic.width() * scale, intrinsic.height() * scale);
    const QRectF target((logicalSize.width() - painted.width()) / 2.0,
                        (logicalSize.height() - painted.height()) / 2.0,
                        painted.width(), painted.height());
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setClipRect(QRectF(QPointF(0, 0), logicalSize));
    renderer.render(&painter, target);
    return result;
}

}
