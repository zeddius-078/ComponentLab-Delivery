#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace commonWidget {

// Public design inventory generated from the current Figma component ledger.
// instantiable=false means the design contract exists but no runtime class is claimed.
struct DesignComponentDescriptor {
    QString designComponent;
    QString sdkClass;
    QString qtClass;
    QString mappingKind;
    QString category;
    QString pageId;
    QString componentSetId;
    QString publicDesignNodeId;
    QString variantSourceComponentSetId;
    int variantCount;
    bool instantiable;
    QString figmaStatus;
    QString approvalStatus;
    QString runtimeStatus;
};

// Full canonical library inventory. Widget/Composite entries may also appear in
// DesignComponentDescriptor; LayoutHelper and Asset entries deliberately do not.
struct DesignLibraryEntry {
    QString canonicalId;
    QString name;
    QString kind;
    QString category;
    QString pageId;
    QString componentSetId;
    QString publicDesignNodeId;
    QString variantSourceComponentSetId;
    QString engineeringStatus;
    QString approvalStatus;
    bool instantiable;
};

struct DesignComponentCategory {
    QString id;
    QString label;
};

struct DesignVariantAxis {
    QString name;
    QStringList values;
};

struct DesignComponentSet {
    QString id;
    QString name;
    QList<DesignVariantAxis> variantAxes;
    QStringList variants;
};

struct DesignComponentPage {
    QString id;
    QString name;
    QString categoryId;
    QList<DesignComponentSet> componentSets;
};

QList<DesignComponentCategory> designComponentCategories();
QList<DesignComponentDescriptor> designComponentDescriptors();
QList<DesignLibraryEntry> designLibraryEntries();
QList<DesignComponentCategory> figmaComponentCategories();
QList<DesignComponentPage> figmaComponentPages();

} // namespace commonWidget
