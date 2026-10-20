#pragma once
// Bounded static preview: design identity/geometry and initial appearance are
// independent inputs. No SDK class inference, component-name heuristics or
// document-specific branches. The formal SceneBuilder remains unchanged.
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QLabel>
#include <QPixmap>
#include <QXmlStreamWriter>
#include <QCryptographicHash>
#include <QSet>
#include <QFontMetrics>
#include <QRegularExpression>
#include <cmath>
#include <memory>
#include <stdexcept>

namespace v2 {
struct StructureResult {
    QByteArray ui, qss, report, png;
};

class StructurePreview {
    QSet<QString> ids;
    QJsonArray nodes, warnings;
    QString css, font;
    QJsonObject style;
    QXmlStreamWriter xml;
    QByteArray ui;
    int count = 0;

    static double number(const QJsonObject& node, const char* key, bool size = false) {
        const auto v = node[key];
        if (!v.isDouble() || !std::isfinite(v.toDouble()) ||
            std::abs(v.toDouble()) > 32768 || (size && v.toDouble() < 0))
            throw std::runtime_error("STRUCTURE_INVALID_GEOMETRY");
        return v.toDouble();
    }
    void warning(const QString& id, const QString& code) {
        warnings.append(QJsonObject{{"id", id}, {"code", code}});
    }
    void property(const QString& key, const QString& type, const QString& value) {
        xml.writeStartElement("property"); xml.writeAttribute("name", key);
        xml.writeTextElement(type, value); xml.writeEndElement();
    }
    QString color(const char* key) const { return style[key].toString(); }

    QWidget* node(const QJsonObject& source, QWidget* parent, const QString& parentId,
                  bool ancestorVisible, int depth) {
        if (++count > 10000 || depth > 96) throw std::runtime_error("STRUCTURE_LIMIT");
        const auto id = source["id"].toString();
        const auto type = source["type"].toString();
        if (id.isEmpty() || type.isEmpty() || ids.contains(id))
            throw std::runtime_error("STRUCTURE_ID_INVALID_OR_DUPLICATE");
        if (type == "INSTANCE" && source["mainComponent"].toObject()["id"].toString().isEmpty())
            throw std::runtime_error("STRUCTURE_INSTANCE_IDENTITY_MISSING");
        ids.insert(id);
        const int x = parent ? qRound(number(source, "x")) : 0;
        const int y = parent ? qRound(number(source, "y")) : 0;
        const int w = qRound(number(source, "width", true));
        const int h = qRound(number(source, "height", true));
        if (!parent && (w <= 0 || h <= 0 || w > 8192 || h > 8192))
            throw std::runtime_error("STRUCTURE_ROOT_SIZE");
        if (source.contains("visible") && !source["visible"].isBool())
            throw std::runtime_error("STRUCTURE_VISIBLE_INVALID");
        const bool visible = ancestorVisible && source["visible"].toBool(true);
        const bool text = type == "TEXT";
        if (text && !source["characters"].isString())
            throw std::runtime_error("STRUCTURE_TEXT_MISSING");
        if (source.contains("children") && !source["children"].isArray())
            throw std::runtime_error("STRUCTURE_CHILDREN_INVALID");
        const auto children = source["children"].toArray();
        const bool shape = type == "RECTANGLE" || type == "ELLIPSE" || type == "LINE";
        const bool container = type == "FRAME" || type == "GROUP" || type == "INSTANCE" ||
            type == "COMPONENT" || type == "COMPONENT_SET";
        bool placeholder = !text && !shape && !container;
        for (const auto& fill : source["fills"].toArray())
            if (fill.toObject()["visible"].toBool(true) && fill.toObject()["type"] == "IMAGE")
                placeholder = true;
        // Preview object names use the original Pixso name, not a guessed
        // development class. A stable suffix distinguishes repeated instances.
        QString readableName = source["name"].toString().normalized(QString::NormalizationForm_C);
        readableName.replace(QRegularExpression("[^\\p{L}\\p{N}_]"), "_");
        readableName = readableName.left(80);
        if (readableName.isEmpty()) readableName = "unnamed";
        const QString name = "node_" + readableName + "_" +
            QString::fromLatin1(QCryptographicHash::hash(id.toUtf8(),
                QCryptographicHash::Sha256).toHex().left(16));
        std::unique_ptr<QWidget> owned(text ? static_cast<QWidget*>(new QLabel(parent)) : new QWidget(parent));
        QWidget* widget = owned.get();
        widget->setObjectName(name);
        widget->setAttribute(Qt::WA_StyledBackground, true);
        widget->setGeometry(x, y, w, h);
        xml.writeStartElement("widget");
        xml.writeAttribute("class", text ? "QLabel" : "QWidget");
        xml.writeAttribute("name", name);
        xml.writeStartElement("property"); xml.writeAttribute("name", "geometry");
        xml.writeStartElement("rect");
        for (const auto& pair : {qMakePair(QString("x"), x), qMakePair(QString("y"), y),
                                 qMakePair(QString("width"), w), qMakePair(QString("height"), h)})
            xml.writeTextElement(pair.first, QString::number(pair.second));
        xml.writeEndElement(); xml.writeEndElement();
        property("visible", "bool", visible ? "true" : "false");
        property("toolTip", "string", source["name"].toString() + " [" + id + "]");
        xml.writeStartElement("property");
        xml.writeAttribute("name", "designNodeName");
        xml.writeAttribute("stdset", "0");
        xml.writeTextElement("string", source["name"].toString());
        xml.writeEndElement();
        QString rule = "background: transparent; border: none;";
        if (!parent) rule = "background: " + color("canvas") + "; border: none;";
        else if (placeholder) {
            rule = "background: transparent; border: 1px dashed " + color("placeholder") + ";";
            warning(id, "ASSET_PLACEHOLDER");
        } else if (!text && (!source["fills"].toArray().isEmpty() || shape)) {
            const int radius = type == "ELLIPSE" ? qMin(w, h) / 2 : style["radius"].toInt();
            rule = "background: " + color("surface") + "; border: 1px solid " +
                color("border") + "; border-radius: " + QString::number(radius) + "px;";
        }
        if (text) {
            auto* label = static_cast<QLabel*>(widget);
            label->setTextFormat(Qt::PlainText);
            label->setText(source["characters"].toString());
            const double sourceSize = source["fontSize"].toDouble(14);
            // Preserve compact one-line labels instead of wrapping them into
            // clipped second lines solely because the initial font differs.
            const bool wrap = h > sourceSize * 1.8 || label->text().contains('\n');
            label->setWordWrap(wrap);
            label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            property("textFormat", "enum", "Qt::PlainText");
            property("text", "string", label->text());
            property("wordWrap", "bool", wrap ? "true" : "false");
            property("alignment", "set", "Qt::AlignLeft|Qt::AlignTop");
            double size = source["fontSize"].toDouble(14);
            if (!std::isfinite(size) || size < 1 || size > 512)
                throw std::runtime_error("STRUCTURE_FONT_SIZE_INVALID");
            rule += " color: " + color("text") + "; font-size: " + QString::number(size) +
                "px; font-family: \"" + font + "\";";
            warning(id, "INITIAL_FONT_AND_ALIGNMENT_NOT_SOURCE_TYPOGRAPHY");
        }
        css += "#" + name + " { " + rule + " }\n";
        if (source["rotation"].toDouble() != 0) warning(id, "ROTATION_NOT_RENDERED");
        if (number(source, "width", true) != w || number(source, "height", true) != h ||
            (parent && (number(source, "x") != x || number(source, "y") != y)))
            warning(id, "GEOMETRY_ROUNDED_TO_QT_INTEGER");
        if (source["opacity"].toDouble(1) != 1) warning(id, "OPACITY_NOT_RENDERED");
        if (!source["effects"].toArray().isEmpty()) warning(id, "EFFECTS_NOT_RENDERED");
        if (source["clipsContent"] == false && !children.isEmpty())
            warning(id, "QT_CHILDREN_CLIP_TO_PARENT");
        if (type == "INSTANCE" || type == "COMPONENT")
            warning(id, "PRESENTATION_ONLY_NO_COMPONENT_CONTRACT");
        QJsonObject record{{"id", id}, {"parentId", parentId}, {"objectName", name},
            {"sourceName", source["name"]}, {"sourceType", type},
            {"sourceKey", source["key"]}, {"mainComponent", source["mainComponent"]},
            {"componentProperties", source["componentProperties"]},
            {"visible", visible}, {"sourceLayoutMode", source["layoutMode"]},
            {"sourceLayout", QJsonObject{{"itemSpacing", source["itemSpacing"]},
                {"paddingLeft", source["paddingLeft"]}, {"paddingRight", source["paddingRight"]},
                {"paddingTop", source["paddingTop"]}, {"paddingBottom", source["paddingBottom"]}}},
            {"geometryPolicy", "observed-static"},
            {"geometry", QJsonArray{x,y,w,h}}, {"placeholder", placeholder},
            {"text", source["characters"]}};
        nodes.append(record);
        for (const auto& child : children) {
            if (!child.isObject()) throw std::runtime_error("STRUCTURE_NODE_INVALID");
            node(child.toObject(), widget, id, visible, depth + 1);
        }
        if (!parent) property("styleSheet", "string", css);
        widget->setVisible(visible);
        xml.writeEndElement();
        return owned.release();
    }
public:
    StructureResult run(const QJsonObject& input, const QJsonObject& initialStyle,
                        const QString& fontFamily, const QString& inputHash,
                        const QString& styleHash, const QString& fontHash) {
        if (input["schema"] != "design-structure/v1" ||
            input["fileKey"].toString().isEmpty() || !input["root"].isObject() ||
            input["provenance"].toString().isEmpty())
            throw std::runtime_error("STRUCTURE_SCHEMA_INVALID");
        if (initialStyle["schema"] != "initial-visual/v1" ||
            initialStyle["name"].toString().isEmpty() || initialStyle["version"] != 1)
            throw std::runtime_error("INITIAL_STYLE_SCHEMA_INVALID");
        style = initialStyle;
        for (const auto* key : {"canvas", "surface", "text", "border", "placeholder"}) {
            const auto value = color(key);
            if (!QRegularExpression("^#[0-9a-fA-F]{6}$").match(value).hasMatch())
                throw std::runtime_error("INITIAL_STYLE_COLOR_INVALID");
        }
        if (!style["radius"].isDouble() || style["radius"].toDouble() < 0 ||
            style["radius"].toDouble() > 64)
            throw std::runtime_error("INITIAL_STYLE_RADIUS_INVALID");
        font = fontFamily;
        if (font.contains('"') || font.contains('\\') || font.contains('\n'))
            throw std::runtime_error("INITIAL_FONT_NAME_INVALID");
        QBuffer buffer(&ui); buffer.open(QIODevice::WriteOnly);
        xml.setDevice(&buffer); xml.setAutoFormatting(true);
        xml.writeStartDocument(); xml.writeStartElement("ui");
        xml.writeAttribute("version", "4.0"); xml.writeTextElement("class", "StructurePreview");
        std::unique_ptr<QWidget> page(node(input["root"].toObject(), nullptr, "", true, 0));
        xml.writeEmptyElement("resources"); xml.writeEmptyElement("connections");
        xml.writeEndElement(); xml.writeEndDocument();
        page->setStyleSheet(css);
        page->ensurePolished();
        page->show();
        QApplication::processEvents();
        for (const auto* label : page->findChildren<QLabel*>()) {
            if (!label->isVisible()) continue;
            const QFontMetrics metrics(label->font());
            const QRect needed = metrics.boundingRect(QRect(0, 0, label->width(), 32768),
                Qt::AlignLeft | Qt::AlignTop | (label->wordWrap() ? Qt::TextWordWrap : 0),
                label->text());
            if (needed.height() > label->height() ||
                (!label->wordWrap() && metrics.horizontalAdvance(label->text()) > label->width())) {
                for (const auto& entry : nodes)
                    if (entry.toObject()["objectName"] == label->objectName())
                        warning(entry.toObject()["id"].toString(), "TEXT_METRICS_OVERFLOW");
            }
        }
        QByteArray png;
        QBuffer imageBuffer(&png); imageBuffer.open(QIODevice::WriteOnly);
        if (!page->grab().save(&imageBuffer, "PNG")) throw std::runtime_error("PREVIEW_PNG_FAILED");
        QJsonObject report{{"schema", "structure-preview-report/v1"},
            {"status", "PREVIEW_WITH_LIMITATIONS"}, {"formal", false},
            {"fileKey", input["fileKey"]}, {"rootId", input["root"].toObject()["id"]},
            {"provenance", input["provenance"]}, {"capturedAt", input["capturedAt"]},
            {"inputSha256", inputHash}, {"styleSha256", styleHash}, {"fontSha256", fontHash},
            {"styleName", style["name"]}, {"fontFamily", font},
            {"layoutPolicy", "observed-static-not-responsive"}, {"nodes", nodes},
            {"warnings", warnings}, {"componentMapping", "NOT_RUN"},
            {"interaction", "NOT_IMPLEMENTED"}, {"visualParity", "NOT_CLAIMED"}};
        return {ui, css.toUtf8(), QJsonDocument(report).toJson(), png};
    }
};
}
