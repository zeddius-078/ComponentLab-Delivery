#include "InputGallery.h"
#include "ComponentFactory.h"
#include "FigmaGalleryView.h"
#include "Z_InputField.h"
#include <QFile>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <stdexcept>

namespace native_gallery {
namespace {
QRect bounds(const QJsonArray& n) {
    return QRect(qRound(n[3].toDouble()), qRound(n[4].toDouble()), qRound(n[5].toDouble()),
                 qRound(n[6].toDouble()));
}
QString dimension(const QString& name, const QString& key) {
    for (const auto& part : name.split(','))
        if (part.trimmed().startsWith(key + '='))
            return part.section('=', 1).trimmed();
    return {};
}
} // namespace
QWidget* createInputGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                            QWidget* parent) {
    QFile file(":/figma/gallery-layouts.json");
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Input gallery evidence missing");
    QJsonArray gallery;
    for (const auto& page : QJsonDocument::fromJson(file.readAll()).object()["pages"].toArray())
        if (page.toObject()["id"] == "339:2")
            gallery = page.toObject()["galleries"].toArray()[0].toArray();
    if (gallery.isEmpty())
        throw std::runtime_error("Input gallery missing");
    QFontDatabase::addApplicationFont(":/figma/fonts/Inter-SemiBold.otf");
    auto* canvas = new QWidget;
    canvas->setObjectName("figmaGalleryCanvas");
    canvas->setFixedSize(bounds(gallery).size());
    canvas->setProperty("galleryNodeId", gallery[0].toString());
    canvas->setProperty("variantSpecimenCount", 108);
    canvas->setProperty("componentSetCount", 7);
    canvas->setStyleSheet("QWidget#figmaGalleryCanvas {background:white;border-radius:16px;}");
    const auto visuals = field::inputVisuals();
    auto actionTokens = std::make_shared<field::Tokens>("light-comfortable");
    for (const auto& child : gallery[7].toArray()) {
        const auto node = child.toArray();
        auto* group = new QWidget(canvas);
        group->setGeometry(bounds(node));
        if (node[1] == "Variant key") {
            group->setObjectName("variantKey");
            group->setStyleSheet("QWidget#variantKey {background:#f7f9fc;border-radius:10px;}");
            for (const auto& item : node[7].toArray()) {
                const auto key = item.toArray();
                auto* label = new QLabel(key[7].toString(), group);
                label->setGeometry(bounds(key));
                label->setStyleSheet("background:transparent;border:0;color:#505868;font-family:'"
                                     "Inter';font-size:12px;");
                QFont font("Inter");
                font.setPixelSize(12);
                font.setWeight(QFont::DemiBold);
                label->setFont(font);
            }
            continue;
        }
        group->setProperty("figmaComponentSetId", node[0].toString());
        group->setObjectName("inputSet_" + node[0].toString().replace(':', '_'));
        for (const auto& set : visuals["sets"].toArray())
            if (set.toObject()["id"] == node[0])
                group->setStyleSheet(
                    QString("QWidget#%1 {background:%2;border-radius:%3px;}")
                        .arg(group->objectName(), set.toObject()["background"].toString())
                        .arg(set.toObject()["radius"].toInt()));
        for (const auto& entry : node[7].toArray()) {
            const auto variant = entry.toArray();
            const auto name = variant[1].toString();
            QWidget* specimen = nullptr;
            if (node[0] == "1179:666") {
                auto* action = new field::Action(actionTokens, group);
                const auto glyph = dimension(name, "Glyph");
                action->setFigmaGlyph(glyph);
                action->setAccessibleName(glyph);
                action->setToolTip(glyph);
                action->setProperty("internalAnatomy", true);
                action->setProperty("publicAdapterImplemented", false);
                action->setEnabled(!glyph.endsWith("Disabled"));
                specimen = action;
            } else {
                auto* input = qobject_cast<commonWidget::Z_InputField*>(
                    factory->create("commonWidget::Z_InputField", group));
                if (!input)
                    throw std::runtime_error("Input SDK identity missing");
                using Kind = commonWidget::Z_InputField::Kind;
                const auto setName = node[1].toString();
                const bool password = setName.contains("Password"),
                           search = setName.contains("Search"), area = setName.contains("TextArea");
                input->setKind(password ? Kind::Password
                               : search ? Kind::Search
                               : area   ? Kind::TextArea
                                        : Kind::Text);
                input->setAppearance("figma-light");
                const auto size = dimension(name, "Size");
                input->setControlSize(size == "Small" ? "sm" : size == "Large" ? "lg" : "md");
                input->setAutoSize(false, 2, 5);
                input->setPlaceholder(password ? "Password"
                                      : search ? "Search"
                                      : area   ? QString::fromUtf8("Describe the context…")
                                               : "Input text");
                const auto state = dimension(name, "State");
                const bool filled = state == "Filled" || dimension(name, "Content") == "Filled";
                input->setValue(node[0] == "1161:16084" ? "secret42"
                                : !filled               ? ""
                                : search                ? "ComponentLab"
                                : area                  ? "First line\nSecond line"
                                                        : "Entered value");
                input->setPasswordVisible(dimension(name, "Visibility") == "Revealed");
                input->setAllowClear(search);
                input->setLoading(setName.contains("SearchLoading"));
                input->setStatus(state == "Error" ? "error" : "");
                input->setSnapshotState(state.isEmpty() ? "Default" : state);
                input->setEnabled(state != "Disabled");
                specimen = input;
            }
            specimen->setProperty("figmaNodeId", variant[0].toString());
            specimen->setProperty("figmaVariant", name);
            specimen->setProperty("figmaDesignBounds", bounds(variant));
            specimen->setFixedSize(bounds(variant).size());
            specimen->move(bounds(variant).topLeft());
        }
    }
    return new FigmaGalleryView(canvas, parent);
}
} // namespace native_gallery
