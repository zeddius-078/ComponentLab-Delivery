#include "FigmaLayoutEvidence.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <stdexcept>

namespace native_gallery {
namespace {

QRect tupleRect(const QJsonArray& node) {
    return QRect(qRound(node.at(3).toDouble()), qRound(node.at(4).toDouble()),
                 qRound(node.at(5).toDouble()), qRound(node.at(6).toDouble()));
}

const QJsonDocument& layoutDocument() {
    static const QJsonDocument document = [] {
        QFile file(QStringLiteral(":/figma/gallery-layouts.json"));
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("Figma gallery layout resource unavailable");
        const auto parsed = QJsonDocument::fromJson(file.readAll());
        if (!parsed.isObject())
            throw std::runtime_error("Figma gallery layout resource is invalid");
        return parsed;
    }();
    return document;
}

void collectComponentSets(const QJsonArray& node, const QPoint& inheritedOffset,
                          FigmaGalleryLayout& gallery) {
    if (node.size() < 7)
        return;
    const QRect local = tupleRect(node);
    const QPoint absolute = inheritedOffset + local.topLeft();
    if (node.at(2).toString() == QStringLiteral("COMPONENT_SET")) {
        ++gallery.componentSetCount;
        if (node.size() > 7 && node.at(7).isArray()) {
            for (const auto& childValue : node.at(7).toArray()) {
                const auto child = childValue.toArray();
                if (child.size() < 7 || child.at(2).toString() != QStringLiteral("COMPONENT"))
                    continue;
                const QRect childRect = tupleRect(child);
                gallery.variants.append(
                    {node.at(0).toString(), node.at(1).toString(), child.at(0).toString(),
                     child.at(1).toString(), QRect(absolute + childRect.topLeft(), childRect.size())});
            }
        }
        return;
    }
    if (node.size() > 7 && node.at(7).isArray()) {
        for (const auto& childValue : node.at(7).toArray())
            collectComponentSets(childValue.toArray(), absolute, gallery);
    }
}

} // namespace

FigmaGalleryLayout figmaGalleryLayout(const QString& pageId) {
    QJsonObject page;
    for (const auto& value :
         layoutDocument().object().value(QStringLiteral("pages")).toArray()) {
        if (value.toObject().value(QStringLiteral("id")).toString() == pageId) {
            page = value.toObject();
            break;
        }
    }
    const auto galleries = page.value(QStringLiteral("galleries")).toArray();
    if (galleries.isEmpty())
        throw std::runtime_error("Figma gallery page layout unavailable");
    const auto root = galleries.first().toArray();
    FigmaGalleryLayout result;
    result.pageId = pageId;
    result.galleryId = root.at(0).toString();
    result.size = QSize(qRound(root.at(5).toDouble()), qRound(root.at(6).toDouble()));
    if (root.size() > 7 && root.at(7).isArray()) {
        for (const auto& childValue : root.at(7).toArray())
            collectComponentSets(childValue.toArray(), QPoint(), result);
    }
    return result;
}

QString figmaAxisValue(const QString& variant, const QString& axis) {
    const auto entries = variant.split(QStringLiteral(", "));
    for (const auto& entry : entries) {
        const int equals = entry.indexOf('=');
        if (equals > 0 && entry.left(equals).trimmed() == axis)
            return entry.mid(equals + 1).trimmed();
    }
    return {};
}

} // namespace native_gallery
