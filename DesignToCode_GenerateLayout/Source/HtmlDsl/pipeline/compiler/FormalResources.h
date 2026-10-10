#pragma once
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
        for (const auto& value : mapping["rules"].toArray()) {
            const auto entries = value.toObject()["resourceMappings"].toObject()["entries"].toObject();
            for (auto it = entries.begin(); it != entries.end(); ++it) {
                const auto entry = it.value().toObject();
                const auto source = entry["source"].toString();
                if (definitions.contains(source) && definitions[source] != entry)
                    fail("RESOURCE_DECLARATION_CONFLICT", source);
                definitions[source] = entry;
            }
        }
    }

    QString use(const QString& source) {
        if (resolved.contains(source)) return resolved[source];
        const auto entry = definitions.value(source);
        const auto ownership = entry["ownership"].toString();
        if (ownership != "shared" && ownership != "page") fail("RESOURCE_OWNERSHIP_REQUIRED", source);
        if (ownership == "shared") {
            if (!source.startsWith(":/") || !QFile::exists(source)) fail("SHARED_RESOURCE_MISSING", source);
            resolved[source] = source;
            records.append(QJsonObject{{"ownership", ownership}, {"source", source}, {"copied", false}});
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
            const auto shared = declaration["source"].toString();
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

    bool hasPageAssets() const { return !targets.isEmpty(); }
    QJsonArray trace() const { return records; }

    void write(const QString& directory, const QString& pageClass) const {
        if (!hasPageAssets()) return;
        if (!QDir().mkpath(directory + "/view/res/images")) fail("RESOURCE_OUTPUT_FAILED", directory);
        QByteArray bytes;
        QXmlStreamWriter xml(&bytes);
        xml.setAutoFormatting(true);
        xml.writeStartDocument(); xml.writeStartElement("RCC");
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
    [[noreturn]] static void fail(const QString& code, const QString& detail) {
        throw std::runtime_error((code + ": " + detail).toStdString());
    }
    static void save(const QString& path, const QByteArray& bytes) {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
            fail("RESOURCE_OUTPUT_FAILED", path);
    }
};
