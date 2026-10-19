#include "QtMapping.h"
#include "ComponentTheme.h"
#include "FieldChrome.h"
#include "IconAssetCatalog.h"
#include "Z_CheckBox.h"
#include "Z_InputField.h"
#include "Z_Select.h"
#include <QBuffer>
#include <QBoxLayout>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QUrl>
#include <QUiLoader>
#include <QXmlStreamWriter>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace design_mapping {
namespace {
void require(bool ok, const QString& message) {
    if (!ok)
        throw std::invalid_argument(message.toStdString());
}
QJsonObject rule(const QJsonObject& map, const QString& name) {
    auto value = map["components"].toObject()[name].toObject();
    if (value.contains("extends")) {
        auto base = map["components"].toObject()[value["extends"].toString()].toObject();
        base.insert("iconOnly", value["iconOnly"]);
        return base;
    }
    return value;
}
QJsonObject properties(const QJsonObject& node, const QJsonObject& definition) {
    QJsonObject result;
    const auto specs = definition["properties"].toObject();
    for (auto it = specs.begin(); it != specs.end(); ++it)
        result.insert(it.key(), it.value().toObject()["default"]);
    const auto supplied = node["properties"].toObject();
    for (auto it = supplied.begin(); it != supplied.end(); ++it)
        result.insert(it.key(), it.value());
    return result;
}
bool container(const QJsonObject& node) {
    return node["component"] == "Column" || node["component"] == "Row";
}
QString asset(const QString& key, const QJsonObject& map) {
    return key.isEmpty() ? QString() : map["assets"].toObject()[key].toString();
}
void keys(const QJsonObject& object, const QStringList& allowed, const QString& where) {
    for (auto it = object.begin(); it != object.end(); ++it)
        require(allowed.contains(it.key()), where + ": unknown field " + it.key());
}
void visit(const QJsonObject& node, const std::function<void(const QJsonObject&)>& action) {
    action(node);
    for (const auto& child : node["children"].toArray())
        visit(child.toObject(), action);
}
void writeNumber(QXmlStreamWriter& xml, const QString& name, int value) {
    xml.writeStartElement("property");
    xml.writeAttribute("name", name);
    xml.writeTextElement("number", QString::number(value));
    xml.writeEndElement();
}
QByteArray uiXml(const QJsonObject& contract, const field::Tokens& tokens, const QJsonObject& map) {
    QByteArray bytes;
    QXmlStreamWriter xml(&bytes);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("ui");
    xml.writeAttribute("version", "4.0");
    xml.writeTextElement("class", "MappingGenerated");
    std::function<void(const QJsonObject&, bool)> nodeXml = [&](const QJsonObject& node,
                                                                bool root) {
        xml.writeStartElement("widget");
        xml.writeAttribute("class", "QWidget");
        xml.writeAttribute("name",
                           root ? "MappingPageRoot"
                                : (container(node) ? "group_" : "host_") + node["id"].toString());
        xml.writeStartElement("layout");
        xml.writeAttribute("class", node["component"] == "Row" ? "QHBoxLayout" : "QVBoxLayout");
        xml.writeAttribute("name", "layout_" + node["id"].toString());
        const auto metrics = map["layoutMetrics"].toObject();
        const auto gap = container(node)
            ? tokens.metric(metrics["spacing"].toObject()["runtimeMetric"].toString()) : 0;
        const auto margin = container(node)
            ? tokens.metric(metrics["margin"].toObject()["runtimeMetric"].toString()) : 0;
        writeNumber(xml, "spacing", gap);
        for (const auto& side : {"leftMargin", "topMargin", "rightMargin", "bottomMargin"})
            writeNumber(xml, side, margin);
        for (const auto& child : node["children"].toArray()) {
            xml.writeStartElement("item");
            nodeXml(child.toObject(), false);
            xml.writeEndElement();
        }
        xml.writeEndElement();
        xml.writeEndElement();
    };
    nodeXml(contract["page"].toObject(), true);
    xml.writeEmptyElement("resources");
    xml.writeEmptyElement("connections");
    xml.writeEndElement();
    xml.writeEndDocument();
    return bytes;
}
} // namespace

QByteArray read(const QString& path) {
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "Cannot read " + path);
    require(file.size() <= 1024 * 1024, "Mapping input exceeds 1 MiB");
    return file.readAll();
}
QJsonObject parse(const QByteArray& bytes) {
    require(bytes.size() <= 1024 * 1024, "Mapping input exceeds 1 MiB");
    QJsonParseError error;
    const auto json = QJsonDocument::fromJson(bytes, &error);
    require(error.error == QJsonParseError::NoError && json.isObject(),
            QString("JSON at byte %1: %2 (object required)")
                .arg(error.offset)
                .arg(error.errorString()));
    return json.object();
}
QJsonObject componentMap() {
    return parse(read(":/mapping/qt-component-map.json"));
}
QJsonObject example() {
    return parse(read(":/mapping/example.ui-contract.json"));
}
QString pageStyle() {
    return QString::fromUtf8(read(":/mapping/page.qss"));
}

QStringList validate(const QJsonObject& contract, const QString& profile) {
    try {
        const auto map = componentMap();
        const field::Tokens tokens(profile);
        const surface::Catalog surfaces(profile);
        keys(contract, {"schemaVersion", "mappingId", "source", "page"}, "contract");
        require(contract["schemaVersion"] == 1, "schemaVersion must be 1");
        require(contract["mappingId"] == map["id"],
                "mappingId does not match the installed Qt map");
        require(contract["source"].isObject(), "source object required");
        auto source = contract["source"].toObject();
        const QString hashField = "rawSha256";
        keys(source, {"kind", "documentUrl", "nodeId", "rawSha256", "reviewed",
                      "snapshotBase64", "reviewedSha256"}, "source");
        require(source["reviewed"].isBool(), "source.reviewed must be boolean");
        for (const auto& key : QStringList{"documentUrl", "nodeId", hashField})
            require(source[key].isString(), QString("source.%1 must be string").arg(key));
        require(source["kind"] == "localFixture" || source["kind"] == "figmaNormalized",
                "Unsupported source.kind; use localFixture or figmaNormalized");
        const bool designSource = source["kind"] != "localFixture";
        QJsonObject designSnapshot;
        if (designSource) {
            const QUrl url(source["documentUrl"].toString());
            const bool allowedHost = url.host() == "figma.com" || url.host() == "www.figma.com";
            require(url.isValid() && url.scheme() == "https" && allowedHost &&
                        url.userInfo().isEmpty() && (url.port(-1) == -1 || url.port() == 443),
                    "Design URL must use Figma's HTTPS origin");
            require(!source["nodeId"].toString().isEmpty(), "Design nodeId required");
            require(QRegularExpression("^[a-fA-F0-9]{64}$").match(source[hashField].toString()).hasMatch(),
                    "Design evidence SHA256 required");
            require(source["reviewed"].toBool(), "Normalized design contract requires review");
            const auto encoded = source["snapshotBase64"].toString().toLatin1();
            const auto raw = QByteArray::fromBase64(encoded);
            require(!raw.isEmpty() && raw.toBase64() == encoded, "Exact evidence snapshot required");
            const auto digest = QString::fromLatin1(QCryptographicHash::hash(raw, QCryptographicHash::Sha256).toHex());
            require(source[hashField].toString().toLower() == digest &&
                        source["reviewedSha256"].toString().toLower() == digest,
                    "Source bytes or review hash changed");
            designSnapshot = parse(raw);
            require(designSnapshot["provider"] == "figma", "Snapshot provider mismatch");
            const auto fileKey = designSnapshot["fileKey"].toString();
            require(!fileKey.isEmpty() && url.path().split('/').contains(fileKey), "Snapshot file mismatch");
            const auto rootId = designSnapshot["root"].toObject()["id"].toString();
            require(rootId == source["nodeId"].toString(), "Snapshot root mismatch");
        } else {
            require(!source["reviewed"].toBool() && source["documentUrl"].toString().isEmpty() &&
                        source["nodeId"].toString().isEmpty() && source[hashField].toString().isEmpty(),
                    "Local fixture must not claim live design evidence");
        }
        require(contract["page"].isObject() && container(contract["page"].toObject()),
                "page must be Column or Row");
        QSet<QString> identifiers, designInstances;
        int count = 0;
        const auto limits = map["limits"].toObject();
        std::function<void(const QJsonObject&, int)> check = [&](const QJsonObject& node,
                                                                 int depth) {
            const auto id = node["id"].toString();
            require(QRegularExpression("^[A-Za-z][A-Za-z0-9_]{0,63}$").match(id).hasMatch(),
                    "Invalid id: " + id);
            require(!identifiers.contains(id), "Duplicate id: " + id);
            identifiers.insert(id);
            require(++count <= limits["nodes"].toInt() && depth <= limits["depth"].toInt(),
                    "Node/depth limit exceeded");
            const auto kind = node["component"].toString();
            if (container(node)) {
                keys(node, {"id", "component", "layout", "children"}, id);
                require(node["layout"].isObject(), id + ": layout required");
                const auto layout = node["layout"].toObject();
                keys(layout, {"spacing", "margin"}, id + ".layout");
                const auto metricNames = map["layoutMetrics"].toObject();
                for (const auto& key : {"spacing", "margin"}) {
                    const auto mapping = metricNames[key].toObject();
                    const auto metric = mapping["runtimeMetric"].toString();
                    const auto expected = mapping["token"].toString();
                    require(layout[key].isString() && layout[key].toString() == expected,
                            id + ".layout." + key + ": expected Token " + expected +
                                ", got " + layout[key].toVariant().toString());
                    require(tokens.metric(metric) >= 0, id + ": invalid resolved layout metric");
                }
                require(node["children"].isArray() && !node["children"].toArray().isEmpty(),
                        id + ": children required");
                for (const auto& child : node["children"].toArray()) {
                    require(child.isObject(), id + ": child must be object");
                    check(child.toObject(), depth + 1);
                }
                return;
            }
            keys(node, {"id", "component", "properties", "tokenRefs", "design"}, id);
            if (designSource) {
                require(node["design"].isObject(), id + ": design identity required");
                const auto design = node["design"].toObject();
                keys(design, {"instanceNodeId", "componentKey", "libraryVersion", "alias"},
                     id + ".design");
                for (auto key : {"instanceNodeId", "componentKey", "libraryVersion", "alias"})
                    require(design[key].isString() && !design[key].toString().trimmed().isEmpty() &&
                                design[key].toString().size() <= 256,
                            id + ".design." + key + ": identity required");
                require(design["alias"] == kind,
                        id + ": design alias does not match mapped family");
                require(!designInstances.contains(design["instanceNodeId"].toString()),
                        id + ": duplicate design instance");
                designInstances.insert(design["instanceNodeId"].toString());
            } else {
                require(!node.contains("design"),
                        id + ": local fixture must not claim design identity");
            }
            auto definition = rule(map, kind);
            require(!definition.isEmpty(), id + ": unmapped component " + kind);
            require(node["properties"].isObject(), id + ": properties object required");
            const auto specs = definition["properties"].toObject();
            const auto supplied = node["properties"].toObject();
            for (auto it = supplied.begin(); it != supplied.end(); ++it) {
                const auto where = id + "." + it.key();
                require(specs.contains(it.key()), where + ": unmapped property");
                const auto spec = specs[it.key()].toObject();
                const auto type = spec["type"].toString();
                const auto value = it.value();
                if (type == "bool")
                    require(value.isBool(), where + ": boolean required");
                else if (type == "options") {
                    require(value.isArray() && value.toArray().size() <= limits["options"].toInt(),
                            where + ": options array required");
                    QSet<QString> values;
                    for (const auto& entry : value.toArray()) {
                        require(entry.isObject(), where + ": option object required");
                        auto option = entry.toObject();
                        keys(option, {"value", "label", "disabled"}, where);
                        require(option["value"].isString() &&
                                    !option["value"].toString().isEmpty() &&
                                    option["value"].toString().size() <= 2048 &&
                                    !values.contains(option["value"].toString()),
                                where + ": invalid/duplicate option value");
                        require(option["label"].isString() &&
                                    option["label"].toString().size() <= 2048 &&
                                    option["disabled"].isBool(),
                                where + ": label and disabled required");
                        values.insert(option["value"].toString());
                    }
                } else if (type == "strings") {
                    require(value.isArray() && value.toArray().size() <= 64,
                            where + ": string array required");
                    QSet<QString> unique;
                    for (const auto& item : value.toArray()) {
                        require(item.isString() && item.toString().size() <= 2048 &&
                                    !unique.contains(item.toString()),
                                where + ": unique strings required");
                        unique.insert(item.toString());
                    }
                } else {
                    require(value.isString() &&
                                value.toString().size() <= limits["textLength"].toInt(),
                            where + ": bounded string required");
                    if (type == "enum")
                        require(spec["values"].toArray().contains(value),
                                where + ": unsupported value " + value.toString());
                    if (type == "asset" && !value.toString().isEmpty()) {
                        const auto assetId = asset(value.toString(), map);
                        require(commonWidget::IconAssetCatalog::isSemanticId(assetId) &&
                                    commonWidget::IconAssetCatalog::contains(assetId),
                                where + ": register asset in qt-component-map.json first");
                    }
                    if (type == "material")
                        require(surfaces.presetIds().contains(value.toString()) ||
                                    (definition["factory"] == "checkbox" && value == "figma-light"),
                                where + ": unsupported material");
                }
            }
            const auto p = properties(node, definition);
            if (definition["factory"] == "button") {
                if (definition["iconOnly"].toBool())
                    require(!p["accessibleName"].toString().trimmed().isEmpty() &&
                                (!p["leadingIcon"].toString().isEmpty() ||
                                 !p["trailingIcon"].toString().isEmpty()),
                            id + ": IconButton requires icon and accessibleName");
            }
            if (definition["factory"] == "select") {
                QSet<QString> options;
                for (const auto& entry : p["options"].toArray())
                    options.insert(entry.toObject()["value"].toString());
                require(p["mode"] != "single" || p["values"].toArray().size() <= 1,
                        id + ": single mode accepts one value");
                for (const auto& value : p["values"].toArray())
                    require(p["mode"] == "tags" || options.contains(value.toString()),
                            id + ": unknown selected option");
            }
            require(node["tokenRefs"].isArray(), id + ": tokenRefs array required");
            QJsonArray allowedRefs;
            const auto size = p["controlSize"].toString();
            const auto sizeAliases = specs["controlSize"].toObject()["tokenValues"].toObject();
            const auto tokenSize = sizeAliases.contains(size) ? sizeAliases[size].toString() : size;
            for (const auto& ref : definition["tokenRefs"].toArray()) {
                auto path = ref.toString();
                const auto overrides = definition["tokenRefOverrides"].toObject()[size].toObject();
                if (overrides.contains(path))
                    path = overrides[path].toString();
                else {
                    path.replace(".size.medium.", ".size." + tokenSize + '.');
                    path.replace(".size.small.", ".size." + tokenSize + '.');
                }
                allowedRefs.append(path);
            }
            for (const auto& ref : node["tokenRefs"].toArray())
                require(ref.isString() && allowedRefs.contains(ref),
                        id + ": unregistered Token binding " + ref.toString());
        };
        check(contract["page"].toObject(), 0);
        return {};
    } catch (const std::exception& error) {
        return {QString::fromUtf8(error.what())};
    }
}

QMap<QString, QByteArray> generate(const QJsonObject& contract, const QString& profile) {
    const auto errors = validate(contract, profile);
    require(errors.isEmpty(), errors.join('\n'));
    const auto json = QJsonDocument(contract).toJson();
    QByteArray delimiter;
    int suffix = 0;
    do {
        delimiter = "QTM" + QByteArray::number(suffix++);
    } while (json.contains(')' + delimiter + '"'));
    const field::Tokens tokens(profile);
    QMap<QString, QByteArray> files;
    files["MappingGenerated.ui"] = uiXml(contract, tokens, componentMap());
    files["MappingGenerated.h"] =
        "#pragma once\n#include <QWidget>\nclass MappingGenerated : public QWidget {\n"
        "public:\n    explicit MappingGenerated(const QString& profile, QWidget* parent = "
        "nullptr);\n"
        "    QWidget* mappedControl(const QString& id) const;\n};\n";
    files["MappingGenerated.cpp"] =
        "#include \"MappingGenerated.h\"\n#include \"ui_MappingGenerated.h\"\n#include "
        "\"QtMapping.h\"\n"
        "MappingGenerated::MappingGenerated(const QString& profile, QWidget* parent) : "
        "QWidget(parent) {\n"
        "    Ui::MappingGenerated ui;\n    ui.setupUi(this);\n"
        "    const auto contract = design_mapping::parse(QByteArray(R\"" +
        delimiter + "(\n" + json + ")" + delimiter +
        "\"));\n"
        "    design_mapping::mount(this, contract, profile);\n    "
        "setStyleSheet(design_mapping::pageStyle());\n}\n"
        "QWidget* MappingGenerated::mappedControl(const QString& id) const { return "
        "findChild<QWidget*>(id); }\n";
    files["MappingGenerated.qss"] = pageStyle().toUtf8();
    files["ui-contract.json"] = QJsonDocument(contract).toJson();
    QJsonObject hashes;
    for (auto it = files.begin(); it != files.end(); ++it)
        hashes.insert(
            it.key(),
            QString::fromLatin1(
                QCryptographicHash::hash(it.value(), QCryptographicHash::Sha256).toHex()));
    QJsonObject report{
        {"status", "ADAPTED"},
        {"source", contract["source"]},
        {"profile", profile},
        {"mappingId", contract["mappingId"]},
        {"tokenPipelineModified", false},
        {"tokenSourceHash", tokens.sourceHash()},
        {"files", hashes},
        {"layout", QJsonObject{{"spacing", tokens.metric("labelGap")},
                               {"margin", tokens.metric("tagPadding")}}},
        {"dependencies", QJsonArray{"Qt5::UiTools", "d2c_theme", "d2c_appearance", "mapping_core", "mapping-resources.qrc", "component-resources.qrc"}},
        {"designParity", "NOT_VERIFIED_WITH_LIVE_DESIGN"}};
    files["mapping-report.json"] = QJsonDocument(report).toJson();
    files["README.md"] =
        QString::fromUtf8(
            "# Qt 映射产物\n\n.ui 保存结构，.h/.cpp 装配现有组件，.qss 读取当前运行时调色板。\n\n"
            "这些文件使用 DesignToCode 的 mapping_core、私有 ComponentKit 和两份 QRC。\n"
            "CMake 开启 AUTOMOC、AUTOUIC；把 MappingGenerated.cpp 和 MappingGenerated.ui "
            "加入目标，链接 mapping_core。\n"
            "创建 QApplication 后先调用 component_theme::install(app, profile)，再创建 "
            "MappingGenerated(profile)。\n\n"
            "MappingGenerated.qss 是 mapping/page.qss 的可读副本；编译后的 .cpp 从 QRC "
            "读取同一份页面样式。\n"
            "组件视觉由 SDK 内嵌的 V2 tokens_qt.json 运行时解析；改变导出副本不会更改内嵌资源。\n"
            "Qt Designer 展示结构占位；真实组件在装配后出现。\n\n"
            "ui-contract.json 是本次输入，mapping-report.json 给出来源、依赖及文件哈希。\n"
            "完整接入步骤见 docs/mainSkill/generate-component-pipeline/references/设计组件映射指南.md。\n")
            .toUtf8();
    return files;
}

void mount(QWidget* root, const QJsonObject& contract, const QString& profile) {
    const auto errors = validate(contract, profile);
    require(errors.isEmpty(), errors.join('\n'));
    const auto map = componentMap();
    const auto fields = std::make_shared<field::Tokens>(profile);
    const auto surfaces = std::make_shared<surface::Catalog>(profile);
    using namespace commonWidget;
    visit(contract["page"].toObject(), [&](const QJsonObject& node) {
        const auto id = node["id"].toString();
        auto* layout = root->findChild<QBoxLayout*>("layout_" + id);
        require(layout != nullptr, "Generated .ui is missing layout_" + id);
        if (container(node)) {
            const int margin = fields->metric("tagPadding");
            layout->setContentsMargins(margin, margin, margin, margin);
            layout->setSpacing(fields->metric("labelGap"));
            layout->setAlignment(node["component"] == "Row" ? Qt::AlignTop | Qt::AlignLeft
                                                            : Qt::AlignTop);
            return;
        }
        require(layout->count() == 0, "Generated host already mounted: " + id);
        const auto definition = rule(map, node["component"].toString());
        const auto p = properties(node, definition);
        QWidget* widget = nullptr;
        const auto kind = definition["factory"].toString();
        if (kind == "button") {
            auto* b = new Z_PushButton(layout->parentWidget());
            widget = b;
            const QMap<QString, ButtonIntent> intents{{"primary", ButtonIntent::Primary},
                                                       {"secondary", ButtonIntent::Secondary},
                                                       {"operation", ButtonIntent::Operation},
                                                       {"danger", ButtonIntent::Danger}};
            const QMap<QString, ButtonAppearance> appearances{
                {"filled", ButtonAppearance::Filled},
                {"outlined", ButtonAppearance::Outlined},
                {"dashed", ButtonAppearance::Dashed},
                {"text", ButtonAppearance::Text},
                {"link", ButtonAppearance::Link}};
            const QMap<QString, ButtonSize> sizes{{"xs", ButtonSize::XS},
                                                  {"sm", ButtonSize::S},
                                                  {"md", ButtonSize::M},
                                                  {"lg", ButtonSize::L}};
            b->setIntent(intents.value(p["intent"].toString()));
            b->setAppearance(appearances.value(p["appearance"].toString()));
            b->setSize(sizes.value(p["controlSize"].toString()));
            b->setText(p["text"].toString());
            b->setAccessibleName(p["accessibleName"].toString());
            b->setLeadingIconStyle(IconContainerStyle::Bare);
            b->setTrailingIconStyle(IconContainerStyle::Bare);
            b->setLeadingIconSource(asset(p["leadingIcon"].toString(), map));
            b->setTrailingIconSource(asset(p["trailingIcon"].toString(), map));
            b->setLeadingIconVisible(!p["leadingIcon"].toString().isEmpty());
            b->setTrailingIconVisible(!p["trailingIcon"].toString().isEmpty());
            b->setIconOnly(definition["iconOnly"].toBool());
            b->setLoading(p["loading"].toBool());
        } else if (kind == "checkbox") {
            auto* c = new Z_CheckBox(surfaces, p["text"].toString(), layout->parentWidget());
            widget = c;
            c->setControlSize(p["controlSize"].toString());
            c->setAppearance(p["appearance"].toString());
            c->setChecked(p["checked"].toBool());
            c->setIndeterminate(p["indeterminate"].toBool());
        } else if (kind == "input") {
            auto* input = new Z_InputField(fields, layout->parentWidget());
            widget = input;
            const QMap<QString, Z_InputField::Kind> kinds{
                {"text", Z_InputField::Kind::Text},
                {"password", Z_InputField::Kind::Password},
                {"search", Z_InputField::Kind::Search},
                {"textarea", Z_InputField::Kind::TextArea}};
            input->setKind(kinds.value(p["kind"].toString()));
            input->setPasswordVisible(p["passwordVisible"].toBool());
            input->setControlSize(p["controlSize"].toString());
            input->setStatus(p["status"] == "none" ? "" : p["status"].toString());
            input->setAppearance(p["appearance"].toString());
            input->setLabel(p["label"].toString());
            input->setHint(p["hint"].toString());
            input->setPlaceholder(p["placeholder"].toString());
            input->setAllowClear(p["allowClear"].toBool());
            input->setReadOnly(p["readOnly"].toBool());
            input->setLoading(p["loading"].toBool());
            input->setValue(p["value"].toString());
        } else if (kind == "select") {
            auto* select = new Z_Select(fields, layout->parentWidget());
            widget = select;
            const QMap<QString, Z_Select::Mode> modes{{"single", Z_Select::Mode::Single},
                                                      {"multiple", Z_Select::Mode::Multiple},
                                                      {"tags", Z_Select::Mode::Tags}};
            select->setMode(modes.value(p["mode"].toString()));
            select->setControlSize(p["controlSize"].toString());
            select->setStatus(p["status"] == "none" ? "" : p["status"].toString());
            select->setAppearance(p["appearance"].toString());
            select->setLabel(p["label"].toString());
            select->setHint(p["hint"].toString());
            select->setPlaceholder(p["placeholder"].toString());
            select->setAllowClear(p["allowClear"].toBool());
            select->setSearchEnabled(p["searchable"].toBool());
            select->setReadOnly(p["readOnly"].toBool());
            select->setLoading(p["loading"].toBool());
            QVector<Z_Select::Option> options;
            for (const auto& entry : p["options"].toArray()) {
                auto option = entry.toObject();
                options.append({option["value"].toString(),
                                option["label"].toString(),
                                {},
                                option["disabled"].toBool()});
            }
            select->setOptions(options);
            QStringList values;
            for (const auto& value : p["values"].toArray())
                values.append(value.toString());
            select->setValues(values);
        }
        require(widget, "Unknown factory: " + kind);
        for (auto* label : widget->findChildren<QLabel*>())
            label->setTextFormat(Qt::PlainText);
        if (kind == "button")
            layout->parentWidget()->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        widget->setObjectName(id);
        widget->setProperty("designComponent", node["component"].toString());
        widget->setProperty("mappingId", contract["mappingId"].toString());
        if (node.contains("design")) {
            widget->setProperty("designNodeId",
                                node["design"].toObject().value("instanceNodeId").toString(
                                    node["design"].toObject()["variantNodeId"].toString()));
            widget->setProperty("designComponentKey",
                                node["design"].toObject().value("componentKey").toString(
                                    node["design"].toObject()["nativeComponentKey"].toString()));
        }
        if (auto* button = qobject_cast<Z_PushButton*>(widget))
            button->setEnabled(!p["disabled"].toBool());
        else
            widget->setEnabled(!p["disabled"].toBool());
        layout->addWidget(widget, 0, kind == "button" ? Qt::AlignLeft : Qt::Alignment{});
    });
}

QWidget* preview(const QJsonObject& contract, const QString& profile, QWidget* parent) {
    const auto files = generate(contract, profile);
    QBuffer buffer;
    buffer.setData(files["MappingGenerated.ui"]);
    buffer.open(QIODevice::ReadOnly);
    QUiLoader loader;
    // Only adapter-generated XML reaches QUiLoader; arbitrary input XML/code is never executed.
    auto* root = loader.load(&buffer, parent);
    require(root, "QUiLoader: " + loader.errorString());
    try {
        mount(root, contract, profile);
        root->setStyleSheet(pageStyle());
        return root;
    } catch (...) {
        delete root;
        throw;
    }
}

QString exportFiles(const QJsonObject& contract, const QString& profile,
                    const QString& parentDirectory) {
    const auto files = generate(contract, profile);
    QDir parent(parentDirectory);
    require(parent.exists(), "Select an existing output directory");
    QString name = "QtMapping-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss-zzz");
    require(!parent.exists(name) && parent.mkdir(name), "Cannot create a fresh export directory");
    const auto directory = parent.filePath(name);
    for (auto it = files.begin(); it != files.end(); ++it) {
        QSaveFile file(QDir(directory).filePath(it.key()));
        require(file.open(QIODevice::WriteOnly) && file.write(it.value()) == it.value().size() &&
                    file.commit(),
                "Cannot write export " + it.key());
    }
    return directory;
}
} // namespace design_mapping
