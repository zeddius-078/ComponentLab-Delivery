#pragma once
// Data-only target library contracts. No component implementation is generated here.
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <stdexcept>

namespace d2c { namespace library {
inline void require(bool ok, const QString& message) {
    if (!ok) throw std::runtime_error(message.toStdString());
}

inline QJsonObject load(QJsonObject profile, const QString& directory) {
    const auto reference = profile.value("componentLibrary").toString();
    if (reference.isEmpty()) return profile; // Existing profiles are unchanged.
    const auto base = QDir(directory).canonicalPath();
    const auto path = QFileInfo(QDir(directory).filePath(reference)).canonicalFilePath();
    require(!base.isEmpty() && path.startsWith(base + '/', Qt::CaseInsensitive),
            "INVALID_COMPONENT_LIBRARY_PATH");
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "COMPONENT_LIBRARY_NOT_FOUND");
    const auto bytes = file.readAll();
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(bytes, &error);
    require(error.error == QJsonParseError::NoError && document.isObject(), "INVALID_COMPONENT_LIBRARY_JSON");
    const auto catalog = document.object();
    require(catalog["schemaVersion"].toInt() == 1 && !catalog["id"].toString().isEmpty(),
            "INVALID_COMPONENT_LIBRARY_SCHEMA");
    require(catalog["classPrefix"].isString(), "MISSING_LIBRARY_CLASS_PREFIX");
    require(catalog["status"] == "contract-only" && catalog["implementationStatus"] == "planned",
            "UNSUPPORTED_COMPONENT_LIBRARY_STATUS");
    require(profile["apiPolicy"] == "public-qt" && profile["widgetSource"] == "qt-builtin-map" &&
            profile["headerPolicy"] == "component-library", "INVALID_COMPONENT_LIBRARY_POLICY");
    QJsonObject entries, byClass;
    const QRegularExpression classPattern("^[A-Za-z_][A-Za-z0-9_]*(::[A-Za-z_][A-Za-z0-9_]*)*$");
    const QRegularExpression headerPattern("^[A-Za-z0-9_/-]+\\.h$");
    for (const auto& value : catalog["components"].toArray()) {
        auto entry = value.toObject();
        const auto id = entry["id"].toString();
        const auto suffix = entry["classSuffix"].toString();
        const auto cls = catalog["classPrefix"].toString() + suffix;
        const auto qt = entry["qtBase"].toString();
        const auto kind = entry["kind"].toString();
        const auto header = entry["header"].toString(catalog["headerPrefix"].toString() + cls + ".h");
        require(!id.isEmpty() && !entries.contains(id) && !byClass.contains(cls), "DUPLICATE_LIBRARY_ID_OR_CLASS");
        require(!suffix.isEmpty() && classPattern.match(suffix).hasMatch() &&
                classPattern.match(cls).hasMatch() && classPattern.match(qt).hasMatch() &&
                qt.startsWith('Q'), "INVALID_LIBRARY_CLASS");
        require(kind == "widget" || kind == "layout" || kind == "spacer", "INVALID_LIBRARY_KIND");
        require(cls != qt, "LIBRARY_TARGET_COLLIDES_WITH_QT_BASE");
        require(headerPattern.match(header).hasMatch() && !header.startsWith('/') && !header.contains(".."),
                "INVALID_LIBRARY_HEADER");
        entry["className"] = cls;
        entry["header"] = header;
        entry["implementationStatus"] = "planned";
        entries[id] = entry;
        byClass[cls] = entry;
    }
    require(!entries.isEmpty(), "EMPTY_COMPONENT_LIBRARY");
    for (const auto& rule : catalog["sourceClassRules"].toArray())
        require(QRegularExpression(rule.toObject()["pattern"].toString()).isValid(), "INVALID_LIBRARY_CLASS_RULE");
    const auto bindings = catalog["designBindings"].toObject();
    for (auto it = bindings.begin(); it != bindings.end(); ++it)
        require(entries.contains(it.value().toString()), "UNMAPPED_LIBRARY_BINDING: " + it.key());
    profile["libraryId"] = catalog["id"];
    profile["libraryStatus"] = catalog["status"];
    profile["componentLibrarySha256"] = QString::fromLatin1(
        QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
    profile["libraryEntries"] = entries;
    profile["libraryClasses"] = byClass;
    profile["sourceClassRules"] = catalog["sourceClassRules"];
    profile["designBindings"] = bindings;
    return profile;
}

inline QJsonObject resolve(const QJsonObject& rule, const QJsonObject& profile) {
    if (!profile.contains("componentLibrary")) return {};
    auto id = profile["designBindings"].toObject()[rule["designComponentName"].toString()].toString();
    if (id.isEmpty()) {
        id = rule["qtClass"].toString();
        for (const auto& value : profile["sourceClassRules"].toArray()) {
            const auto rename = value.toObject();
            id.replace(QRegularExpression(rename["pattern"].toString()), rename["replacement"].toString());
        }
    }
    const auto entry = profile["libraryEntries"].toObject()[id].toObject();
    require(!entry.isEmpty(), "UNMAPPED_LIBRARY_COMPONENT: " + id);
    require(entry["kind"] == "widget", "LIBRARY_COMPONENT_IS_NOT_WIDGET: " + id);
    return entry;
}
} }
