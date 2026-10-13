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
    int variantCount;
    bool instantiable;
    QString figmaStatus;
    QString approvalStatus;
    QString runtimeStatus;
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
QList<DesignComponentCategory> figmaComponentCategories();
QList<DesignComponentPage> figmaComponentPages();

} // namespace commonWidget
