#pragma once
#include "IconAssetCatalog.h"
#include "VisualAssetCatalog.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QSaveFile>
#include <QXmlStreamWriter>
#include <stdexcept>

// 只打包实际消费的页面资产。公共资产保持 SDK 引用；来源与许可留在包外记录。
class FormalResources {
public:
    explicit FormalResources(const QJsonObject& mapping) {
        // 设计侧 Profile 解析结果由调用方预先写入 resolvedComponents 映射；
        // 资源器只保留元数据供页面清单引用，不重新解析设计树。
        metadata["resolvedDesignProfile"] = mapping["resolvedDesignProfile"];
        metadata["resolvedProfileAlias"] = mapping["resolvedProfileAlias"];
        const auto collect = [this](const QJsonObject& entries) {
            for (auto it = entries.begin(); it != entries.end(); ++it) {
                const auto entry = it.value().toObject();
                const auto key = entry.contains("assetId")
                    ? entry["assetId"].toString() : entry["source"].toString();
                if (definitions.contains(key) && definitions[key] != entry)
                    fail("RESOURCE_DECLARATION_CONFLICT", key);
                definitions[key] = entry;
            }
        };
        collect(mapping["publicAssetMappings"].toObject()["entries"].toObject());
        for (const auto& value : mapping["rules"].toArray()) {
            collect(value.toObject()["resourceMappings"].toObject()["entries"].toObject());
        }
    }

    QString use(const QString& source) {
        if (resolved.contains(source)) return resolved[source];
        const auto entry = definitions.value(source);
        const auto ownership = entry["ownership"].toString();
        if (ownership != "shared" && ownership != "page") fail("RESOURCE_OWNERSHIP_REQUIRED", source);
        if (ownership == "shared") {
            const auto path = resolveShared(source);
            resolved[source] = source;
            records.append(QJsonObject{{"ownership", ownership}, {"assetId", source},
                                       {"source", path}, {"copied", false}});
            return source;
        }
        // 命名空间由资源声明提供，而不是从设计 URL 或节点编号拼接。
        const auto prefix = entry["qrcPrefix"].toString();
        const auto alias = entry["alias"].toString();
        if (!QRegularExpression("^/[A-Za-z][A-Za-z0-9_-]*(/[A-Za-z][A-Za-z0-9_-]*)+$").match(prefix).hasMatch() ||
            !QRegularExpression("^[A-Za-z][A-Za-z0-9_-]*\\.(svg|png)$").match(alias).hasMatch())
            fail("INVALID_PAGE_RESOURCE_PATH", source);
        if (entry["provenance"].toString().trimmed().isEmpty() || entry["license"].toString().trimmed().isEmpty())
            fail("PAGE_RESOURCE_PROVENANCE_REQUIRED", source);
        const auto target = ":" + prefix + "/" + alias;
        if (QFile::exists(target) || targets.contains(target)) fail("RESOURCE_PATH_COLLISION", target);
        // 公共目录即使当前无同名文件也不能被页面占用。
        for (const auto& declaration : definitions) {
            if (declaration["ownership"] != "shared") continue;
            const auto shared = resolveShared(declaration["assetId"].toString());
            const auto sharedFolder = shared.left(shared.lastIndexOf('/'));
            if (target.startsWith(sharedFolder + "/")) fail("RESOURCE_SHARED_NAMESPACE_COLLISION", target);
        }
        const QFileInfo info(source);
        if (!info.isAbsolute() || source.startsWith(":/") || info.suffix() != QFileInfo(alias).suffix())
            fail("PAGE_RESOURCE_LOCAL_PATH_REQUIRED", source);
        QFile file(source);
        if (!file.open(QIODevice::ReadOnly) || file.size() <= 0 || file.size() > 8 * 1024 * 1024)
            fail("PAGE_RESOURCE_MISSING_OR_INVALID", source);
        const auto bytes = file.readAll();
        const auto digest = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
        // 同名文件来自不同命名空间时仍使用内容哈希区分包内路径，不暴露源路径。
        const auto local = "images/" + digest + "." + info.suffix();
        targets[target] = {prefix, alias, local, bytes};
        resolved[source] = target;
        records.append(QJsonObject{{"ownership", ownership}, {"source", source}, {"qrc", target},
            {"sha256", digest}, {"license", entry["license"]}, {"provenance", entry["provenance"]}, {"copied", true}});
        return target;
    }

    // 页面 QSS 与页面图片共用一个可编译的 QRC，确保运行时不依赖当前工作目录。
    bool hasPageResources() const { return true; }
    QJsonArray trace() const { return records; }

    // 页面清单引用采集解析元数据；键由页面生成器预先写入映射。
    QJsonValue operator[](const QString& key) const { return metadata.value(key); }

    void write(const QString& directory, const QString& pageClass,
               const QByteArray& pageQss) const {
        // 页面 QSS 始终存在；图片目录只在确有页面图片时创建，避免空资源目录进入交付包。
        if (!QDir().mkpath(directory + "/view/res/qss") ||
            (!targets.isEmpty() && !QDir().mkpath(directory + "/view/res/images")))
            fail("RESOURCE_OUTPUT_FAILED", directory);
        QByteArray bytes;
        QXmlStreamWriter xml(&bytes);
        xml.setAutoFormatting(true);
        xml.writeStartDocument(); xml.writeStartElement("RCC");
        // 页面样式使用独立命名空间，避免覆盖 SDK 的公共资源路径。
        xml.writeStartElement("qresource");
        xml.writeAttribute("prefix", "/d2c/" + pageClass + "/qss");
        xml.writeStartElement("file");
        xml.writeAttribute("alias", pageClass + ".qss");
        xml.writeCharacters("qss/" + pageClass + ".qss");
        xml.writeEndElement();
        xml.writeEndElement();
        save(directory + "/view/res/qss/" + pageClass + ".qss", pageQss);
        for (const auto& asset : targets) {
            save(directory + "/view/res/" + asset.local, asset.bytes);
            xml.writeStartElement("qresource"); xml.writeAttribute("prefix", asset.prefix);
            xml.writeStartElement("file"); xml.writeAttribute("alias", asset.alias);
            xml.writeCharacters(asset.local); xml.writeEndElement(); xml.writeEndElement();
        }
        xml.writeEndElement(); xml.writeEndDocument();
        save(directory + "/view/res/" + pageClass + ".qrc", bytes);
    }
private:
    struct Asset { QString prefix, alias, local; QByteArray bytes; };
    QMap<QString, QJsonObject> definitions;
    QMap<QString, QString> resolved;
    QMap<QString, Asset> targets;
    QJsonArray records;
    QJsonObject metadata;
    static QString resolveShared(const QString& id) {
        if (commonWidget::IconAssetCatalog::isSemanticId(id) &&
            commonWidget::IconAssetCatalog::contains(id))
            return commonWidget::IconAssetCatalog::resolve(id);
        if (commonWidget::VisualAssetCatalog::isSemanticId(id) &&
            commonWidget::VisualAssetCatalog::contains(id))
            return commonWidget::VisualAssetCatalog::resolve(id);
        fail("UNMAPPED_RESOURCE", id);
    }
    [[noreturn]] static void fail(const QString& code, const QString& detail) {
        throw std::runtime_error((code + ": " + detail).toStdString());
    }
    static void save(const QString& path, const QByteArray& bytes) {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
            fail("RESOURCE_OUTPUT_FAILED", path);
    }
};
