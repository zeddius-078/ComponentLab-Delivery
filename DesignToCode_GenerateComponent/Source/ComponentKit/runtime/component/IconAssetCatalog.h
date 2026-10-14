#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace commonWidget {

// Public, non-QWidget icon resource contract. Consumers depend on stable IDs;
// only this catalog owns the concrete QRC paths.
struct IconAssetDescriptor {
    QString id;
    QString qrcPath;
    QString accessibleName;
    QString format;
    QString source;
    QString license;
    QList<int> supportedSizes;
    QStringList supportedTones;
    bool tintable = false;
};

class IconAssetCatalog final {
public:
    static QList<IconAssetDescriptor> descriptors();
    static bool isSemanticId(const QString& value);
    static bool contains(const QString& id);
    // Unknown or unavailable resources fail explicitly with UNMAPPED_RESOURCE.
    static const IconAssetDescriptor& descriptor(const QString& id);
    static QString resolve(const QString& id);
};

}
