#include "V2ComponentMapPage.h"
#include "TokenResources.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSplitter>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <stdexcept>

namespace component_lab_v2 {
namespace {

QString g_lastMapSource;
QString g_lastMapSha;

QJsonObject readObject(const QString& path) {
    // Prefer the map shipped beside the demo executable.  The SDK path is a
    // build-time fallback only; otherwise an old installed SDK can silently
    // make the gallery display a stale V2 token map.
    QStringList candidates;
    const auto fileName = QFileInfo(path).fileName();
    const auto packaged = QCoreApplication::applicationDirPath() +
                          QStringLiteral("/resources/mappings/") + fileName;
    candidates << packaged << path;
    for (const auto& candidate : candidates) {
        QFile file(candidate);
        if (!file.open(QIODevice::ReadOnly))
            continue;
        const auto bytes = file.readAll();
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(bytes, &error);
        if (error.error != QJsonParseError::NoError || !document.isObject())
            continue;
        if (document.isObject() && document.object()["schemaVersion"].toString() == "2.0" &&
            document.object()["components"].isArray()) {
            g_lastMapSource = candidate;
            g_lastMapSha = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
            return QJsonDocument::fromJson(bytes).object();
        }
    }
    {
        // SDK resource fallback keeps the page alive when the mapping files are
        // embedded instead of installed on disk.
        const auto bytes = token_resources::read(":/mappings/" + fileName);
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(bytes, &error);
        if (error.error != QJsonParseError::NoError || !document.isObject())
            throw std::runtime_error(("V2 component map unavailable: " + path).toStdString());
        if (document.object()["schemaVersion"].toString() != "2.0" ||
            !document.object()["components"].isArray())
            throw std::runtime_error("V2 component map has an invalid schema");
        g_lastMapSource = QStringLiteral(":/mappings/") + fileName;
        g_lastMapSha = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
        return document.object();
    }
}

QString valueText(const QJsonValue& value) {
    if (value.isString())
        return value.toString();
    if (value.isDouble())
        return QString::number(value.toDouble(), 'g', 12);
    if (value.isBool())
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    return QString();
}

QString statusText(const QString& status) {
    if (status == "specified")
        return QString::fromUtf8("V2规范");
    if (status == "aligned")
        return QString::fromUtf8("一致");
    if (status == "delta")
        return QString::fromUtf8("待重绑定");
    if (status == "unmapped")
        return QString::fromUtf8("待裁决");
    if (status == "isolated")
        return QString::fromUtf8("隔离（antd 遗留）");
    return status;
}

} // namespace

V2ComponentMapPage::V2ComponentMapPage(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    auto* header = new QLabel(this);
    header->setText(QString::fromUtf8("V2 组件地图 — 每个组件的外观绑定与状态表由 V2 规范产物驱动："
                                      "figma-component-map-v2.json（Figma 变量组 ↔ 组件全量映射）、"
                                      "component-registry.json（状态矩阵）。\n"
                                      "V2 引用显示 Recipe 的语义 Token；解析值显示编译器解析后的实际值，"
                                      "不把语义引用误标为 Figma 设计值。"));
    header->setWordWrap(true);
    layout->addWidget(header);
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    layout->addWidget(splitter, 1);

    auto* left = new QWidget(splitter);
    auto* leftLayout = new QVBoxLayout(left);
    filter = new QLineEdit(left);
    filter->setPlaceholderText(QString::fromUtf8("过滤组件…"));
    leftLayout->addWidget(filter);
    componentList = new QListWidget(left);
    leftLayout->addWidget(componentList, 1);
    left->setMinimumWidth(220);
    splitter->addWidget(left);

    detailTabs = new QTabWidget(splitter);
    splitter->addWidget(detailTabs);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);

    appearanceTable = new QTableWidget(detailTabs);
    appearanceTable->setColumnCount(6);
    appearanceTable->setHorizontalHeaderLabels(
        {QString::fromUtf8("Figma 变量"), QString::fromUtf8("族"),
         QString::fromUtf8("V2 引用"), QString::fromUtf8("Canonical 路径"),
         QString::fromUtf8("解析值"), QString::fromUtf8("状态")});
    appearanceTable->horizontalHeader()->setStretchLastSection(true);
    appearanceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    detailTabs->addTab(appearanceTable, QString::fromUtf8("外观"));

    stateTable = new QTableWidget(detailTabs);
    stateTable->setColumnCount(3);
    stateTable->setHorizontalHeaderLabels(
        {QString::fromUtf8("变体轴"), QString::fromUtf8("取值"), QString::fromUtf8("状态矩阵")});
    stateTable->horizontalHeader()->setStretchLastSection(true);
    stateTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    detailTabs->addTab(stateTable, QString::fromUtf8("状态表"));

    QJsonObject map;
    try {
        map = readObject(QString::fromLatin1(V2_COMPONENT_MAP_PATH));
        header->setToolTip(QString::fromUtf8("实际加载：") + g_lastMapSource +
                           QString::fromUtf8("\nSHA-256：") + g_lastMapSha);
    } catch (const std::exception& error) {
        new QListWidgetItem(QString::fromUtf8("映射文件不可用：") +
                                QString::fromUtf8(error.what()),
                            componentList);
        return;
    }
    const auto components = map["components"].toArray();
    for (const auto& entry : components) {
        const auto object = entry.toObject();
        auto* item = new QListWidgetItem(object["componentId"].toString(), componentList);
        item->setData(Qt::UserRole, entry);
    }
    componentList->sortItems();
    connect(filter, &QLineEdit::textChanged, this, [this](const QString& text) {
        for (int i = 0; i < componentList->count(); ++i) {
            auto* item = componentList->item(i);
            item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
        }
    });
    connect(componentList, &QListWidget::currentRowChanged, this,
            &V2ComponentMapPage::selectComponent);
    if (componentList->count())
        componentList->setCurrentRow(0);
}

void V2ComponentMapPage::selectComponent(int row) {
    appearanceTable->setRowCount(0);
    stateTable->setRowCount(0);
    if (row < 0)
        return;
    auto* item = componentList->item(row);
    const auto component = item->data(Qt::UserRole).toJsonObject();
    const QString componentId = component["componentId"].toString();

    const auto bindings = component["bindings"].toArray();
    appearanceTable->setRowCount(bindings.size());
    for (int i = 0; i < bindings.size(); ++i) {
        const auto binding = bindings[i].toObject();
        const auto canonicalValue = valueText(binding["canonicalValue"]);
        const auto resolvedDisplay = canonicalValue.isEmpty()
            ? QString::fromUtf8("复合引用：") + valueText(binding["value"])
            : canonicalValue;
        const QStringList columns{
            binding["variable"].toString(), binding["family"].toString(),
            valueText(binding["value"]), binding["canonicalPath"].toString(),
            resolvedDisplay, statusText(binding["status"].toString())};
        for (int column = 0; column < columns.size(); ++column) {
            auto* cell = new QTableWidgetItem(columns[column]);
            if (column == 5 && binding["status"].toString() == "delta") {
                cell->setToolTip(binding["note"].toString());
                cell->setText(cell->text() + QString::fromUtf8(" ⚠"));
            }
            appearanceTable->setItem(i, column, cell);
        }
    }

    QVector<QStringList> rows;
    const auto axes = component["variantAxes"].toObject();
    for (auto axis = axes.begin(); axis != axes.end(); ++axis) {
        QStringList values;
        for (const auto& value : axis.value().toArray())
            values << value.toString();
        rows.append({axis.key(), values.join(QString::fromUtf8(" | ")),
                     QString::fromUtf8("Figma 变体轴")});
    }
    QJsonObject registry;
    try {
        registry = readObject(QString::fromLatin1(V2_COMPONENT_REGISTRY_PATH));
    } catch (...) {
        registry = {};
    }
    const auto registryComponents = registry["components"].toObject();
    const auto registryEntry = registryComponents[componentId].toObject();
    if (!registryEntry.isEmpty()) {
        const auto states = registryEntry["states"].toObject();
        const auto supported = states["supported"].toArray();
        QStringList supportedList;
        for (const auto& state : supported)
            supportedList << state.toString();
        rows.append({QString::fromUtf8("状态"), supportedList.join(QString::fromUtf8(" | ")),
                     QString::fromUtf8("V2 组件注册表 states.supported")});
        const auto designOnly = states["designOnly"].toArray();
        if (!designOnly.isEmpty()) {
            QStringList designOnlyList;
            for (const auto& state : designOnly)
                designOnlyList << state.toString();
            rows.append({QString::fromUtf8("设计态（运行时未验证）"),
                         designOnlyList.join(QString::fromUtf8(" | ")),
                         QString::fromUtf8("V2 组件注册表 states.designOnly")});
        }
        const auto variants = registryEntry["variants"].toArray();
        if (!variants.isEmpty()) {
            QStringList variantList;
            for (const auto& variant : variants)
                variantList << variant.toString();
            rows.append({QString::fromUtf8("变体"), variantList.join(QString::fromUtf8(" | ")),
                         QString::fromUtf8("V2 组件注册表 variants")});
        }
        const auto source = registryEntry["source"].toString();
        if (!source.isEmpty())
            rows.append({QString::fromUtf8("来源"), source, QString::fromUtf8("V2 组件注册表")});
    }
    stateTable->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i)
        for (int column = 0; column < 3; ++column)
            stateTable->setItem(i, column, new QTableWidgetItem(rows[i][column]));
    stateTable->resizeColumnToContents(0);
    stateTable->resizeColumnToContents(1);
    appearanceTable->resizeColumnToContents(0);
    appearanceTable->resizeColumnToContents(1);
}

} // namespace component_lab_v2
