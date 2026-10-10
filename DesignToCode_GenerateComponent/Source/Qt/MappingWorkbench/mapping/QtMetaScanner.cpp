#include "ComponentFactory.h"
#include "ComponentTheme.h"

#include <QAbstractButton>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAbstractSlider>
#include <QAbstractSpinBox>
#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QCoreApplication>
#include <QFile>
#include <QFileSystemModel>
#include <QFormLayout>
#include <QGraphicsScene>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHash>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLayout>
#include <QMetaProperty>
#include <QObject>
#include <QSortFilterProxyModel>
#include <QStackedLayout>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QVBoxLayout>

#include <memory>
#include <stdexcept>

namespace {

const QMetaObject* supportMetaObject(const QString& name) {
#define D2C_META(TYPE) if (name == QStringLiteral(#TYPE)) return &TYPE::staticMetaObject
    D2C_META(QAbstractButton);
    D2C_META(QAction);
    D2C_META(QApplication);
    D2C_META(QCoreApplication);
    D2C_META(QGuiApplication);
    D2C_META(QObject);
    D2C_META(QGraphicsScene);
    D2C_META(QAbstractSlider);
    D2C_META(QAbstractSpinBox);
    D2C_META(QAbstractItemView);
    D2C_META(QBoxLayout);
    D2C_META(QFormLayout);
    D2C_META(QGridLayout);
    D2C_META(QHBoxLayout);
    D2C_META(QLayout);
    D2C_META(QStackedLayout);
    D2C_META(QVBoxLayout);
    D2C_META(QAbstractItemModel);
    D2C_META(QFileSystemModel);
    D2C_META(QSortFilterProxyModel);
    D2C_META(QStandardItemModel);
    D2C_META(QStringListModel);
#undef D2C_META
    return nullptr;
}

QJsonObject readObject(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("CATALOG_NOT_READABLE");
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        throw std::runtime_error("CATALOG_INVALID_JSON");
    return document.object();
}

void writeObject(const QString& path, const QJsonObject& object) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        throw std::runtime_error("REPORT_NOT_WRITABLE");
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    if (argc != 3)
        return 64;
    const QJsonObject catalog = readObject(QString::fromLocal8Bit(argv[1]));
    component_theme::install(application, QStringLiteral("light-comfortable"));
    commonWidget::ComponentFactory factory(QStringLiteral("light-comfortable"));
    QWidget owner;
    QJsonArray classes;
    QJsonArray issues;
    int visualCount = 0;
    int verifiedPropertyCount = 0;

    QHash<QString, QString> sdkByQt;
    for (const auto& descriptor : commonWidget::ComponentFactory::descriptors()) {
        if (descriptor.qtClass != QStringLiteral("semantic"))
            sdkByQt.insert(descriptor.qtClass, descriptor.sdkClass);
    }

    for (const auto& entry : catalog.value(QStringLiteral("components")).toArray()) {
        const QJsonObject component = entry.toObject();
        const QString qtClass = component.value(QStringLiteral("qtClass")).toString();
        const QString mappingKind = component.value(QStringLiteral("mappingKind")).toString();
        const QMetaObject* metaObject = nullptr;
        std::unique_ptr<QWidget> visual;
        if (mappingKind == QStringLiteral("visual-component")) {
            ++visualCount;
            const QString sdkClass = sdkByQt.value(qtClass);
            if (!sdkClass.isEmpty()) {
                visual.reset(factory.create(sdkClass));
                metaObject = visual->metaObject();
            }
        } else {
            metaObject = supportMetaObject(qtClass);
        }

        QJsonArray properties;
        for (const auto& propertyEntry : component.value(QStringLiteral("declaredProperties")).toArray()) {
            QJsonObject property = propertyEntry.toObject();
            const QString propertyName = property.value(QStringLiteral("name")).toString();
            QString status = QStringLiteral("catalog-only");
            if (metaObject) {
                const int index = metaObject->indexOfProperty(propertyName.toLatin1().constData());
                if (index >= 0) {
                    const QMetaProperty metaProperty = metaObject->property(index);
                    status = metaProperty.isWritable() ? QStringLiteral("qproperty-writable")
                                                       : QStringLiteral("qproperty-read-only");
                    property.insert(QStringLiteral("qtType"), QString::fromLatin1(metaProperty.typeName()));
                    property.insert(QStringLiteral("qtWritable"), metaProperty.isWritable());
                    property.insert(QStringLiteral("qtEnum"), metaProperty.isEnumType());
                    ++verifiedPropertyCount;
                } else {
                    status = QStringLiteral("documented-api-not-qproperty");
                    issues.append(QJsonObject{{QStringLiteral("severity"), QStringLiteral("info")},
                                              {QStringLiteral("qtClass"), qtClass},
                                              {QStringLiteral("property"), propertyName},
                                              {QStringLiteral("reason"), QStringLiteral("Reference field is not exposed through QMetaProperty")}});
                }
            }
            property.insert(QStringLiteral("metaVerification"), status);
            properties.append(property);
        }
        classes.append(QJsonObject{{QStringLiteral("qtClass"), qtClass},
                                   {QStringLiteral("mappingKind"), mappingKind},
                                   {QStringLiteral("metaObjectAvailable"), metaObject != nullptr},
                                   {QStringLiteral("properties"), properties}});
        if (mappingKind == QStringLiteral("visual-component") && !metaObject)
            issues.append(QJsonObject{{QStringLiteral("severity"), QStringLiteral("error")},
                                      {QStringLiteral("qtClass"), qtClass},
                                      {QStringLiteral("reason"), QStringLiteral("Visual component has no SDK factory or Qt meta-object")}});
    }

    int errors = 0;
    for (const auto& issue : issues) {
        if (issue.toObject().value(QStringLiteral("severity")) == QStringLiteral("error"))
            ++errors;
    }
    const QJsonObject report{
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("qtVersion"), QStringLiteral(QT_VERSION_STR)},
        {QStringLiteral("sourceCatalog"), QString::fromLocal8Bit(argv[1])},
        {QStringLiteral("summary"), QJsonObject{{QStringLiteral("classes"), classes.size()},
                                                 {QStringLiteral("visualComponents"), visualCount},
                                                 {QStringLiteral("verifiedQProperties"), verifiedPropertyCount},
                                                 {QStringLiteral("errors"), errors},
                                                 {QStringLiteral("informationalMismatches"), issues.size() - errors}}},
        {QStringLiteral("classes"), classes},
        {QStringLiteral("issues"), issues}};
    writeObject(QString::fromLocal8Bit(argv[2]), report);
    return errors == 0 ? 0 : 1;
}
