#pragma once
#include <MyDesignSystem/Z_InputField.h>
#include <QAbstractItemModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTableView>

// Validation-only input. The generated page never reads this JSON or creates
// widgets from it. Inspect exactly the objects declared by the generated .ui.
static QJsonObject generatedPageSnapshot(QWidget& page, const QString& contractPath) {
    QFile contractFile(contractPath);
    if (!contractFile.open(QIODevice::ReadOnly))
        throw std::runtime_error("PAGE_SNAPSHOT_CONTRACT_MISSING");
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(contractFile.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        throw std::runtime_error("PAGE_SNAPSHOT_CONTRACT_INVALID");
    const auto contract = document.object();
    const int width = contract.value("width").toInt();
    const int height = contract.value("height").toInt();
    if (width <= 0 || height <= 0 || width > 8192 || height > 8192)
        throw std::runtime_error("PAGE_SNAPSHOT_VIEWPORT_INVALID");
    page.resize(width, height);
    page.show();
    QApplication::processEvents();
    if (page.layout()) page.layout()->activate();
    QJsonArray objects;
    QSet<QString> names;
    const auto definitions = contract.value("objects").toArray();
    if (definitions.isEmpty()) throw std::runtime_error("PAGE_SNAPSHOT_OBJECTS_REQUIRED");
    for (const auto& value : definitions) {
        const auto spec = value.toObject();
        const QString name = spec.value("name").toString();
        const QString type = spec.value("class").toString();
        if (name.isEmpty() || names.contains(name))
            throw std::runtime_error("PAGE_SNAPSHOT_NAME_NOT_UNIQUE");
        names.insert(name);
        auto matches = page.findChildren<QWidget*>(name);
        if (page.objectName() == name) matches.prepend(&page);
        if (matches.size() != 1)
            throw std::runtime_error("GENERATED_WIDGET_NOT_UNIQUE");
        auto* widget = matches.first();
        if (!widget->inherits(type.toLatin1().constData()))
            throw std::runtime_error("GENERATED_WIDGET_CLASS_MISMATCH");
        QJsonObject properties;
        for (const char* key : {"enabled", "text", "accessibleName", "visualState", "iconOnly",
                "leadingIconSource", "trailingIconSource", "density", "profileName",
                "searchText", "collapsed", "activeRoute", "routeIds", "routeLabels",
                "primaryItems", "secondaryItems", "primaryIconSources", "currentIndex",
                "secondaryIndex", "iconPlacement"}) {
            if (widget->metaObject()->indexOfProperty(key) >= 0)
                properties.insert(key, QJsonValue::fromVariant(widget->property(key)));
        }
        if (auto navbar = qobject_cast<commonWidget::Z_Navbar*>(widget)) {
            auto* search = navbar->searchInput();
            if (!search || !search->lineEdit())
                throw std::runtime_error("GENERATED_NAVBAR_SEARCH_MISSING");
            properties.insert("searchPlaceholder", search->lineEdit()->placeholderText());
        }
        if (auto table = qobject_cast<QTableView*>(widget)) {
            auto* model = table->model();
            if (!model) throw std::runtime_error("GENERATED_TABLE_MODEL_MISSING");
            QJsonArray headers, rows;
            bool editable = false;
            for (int column = 0; column < model->columnCount(); ++column)
                headers.append(model->headerData(column, Qt::Horizontal).toString());
            for (int row = 0; row < model->rowCount(); ++row) {
                QJsonArray cells;
                for (int column = 0; column < model->columnCount(); ++column) {
                    const auto index = model->index(row, column);
                    cells.append(model->data(index).toString());
                    editable |= model->flags(index).testFlag(Qt::ItemIsEditable);
                }
                rows.append(cells);
            }
            properties.insert("sampleData", QJsonObject{{"headers", headers}, {"rows", rows}});
            properties.insert("modelEditable", editable);
        }
        QJsonArray resources;
        for (const auto& value : properties.value("primaryIconSources").toArray()) {
            const auto path = value.toString();
            if (!path.startsWith(QStringLiteral(":/")) || !QFile::exists(path))
                throw std::runtime_error("GENERATED_ICON_RESOURCE_MISSING");
            QSvgRenderer renderer(path);
            if (!renderer.isValid()) throw std::runtime_error("GENERATED_ICON_RESOURCE_INVALID");
            resources.append(path);
        }
        objects.append(QJsonObject{{"name", name}, {"class", type},
            {"properties", properties}, {"checkedIconResources", resources},
            {"width", widget->width()}, {"height", widget->height()},
            {"visible", widget->isVisible()}});
    }
    return QJsonObject{{"status", "PASS"}, {"scope", "generated page snapshot only; no synthetic controls"},
        {"qtVersion", qVersion()}, {"objects", objects},
        {"width", page.width()}, {"height", page.height()},
        {"pageStylesheetLoaded", !page.styleSheet().isEmpty()}, {"visualParity", "NOT_RUN"}};
}
