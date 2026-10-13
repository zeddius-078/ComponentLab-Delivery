#include "ButtonGallery.h"
#include "ButtonAppearanceAdapter.h"
#include "ComponentFactory.h"
#include "FigmaGalleryView.h"
#include "Z_PushButton.h"
#include <QFile>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <stdexcept>

namespace native_gallery {
namespace {
QRect bounds(const QJsonArray& node) {
    return QRect(qRound(node[3].toDouble()), qRound(node[4].toDouble()), qRound(node[5].toDouble()),
                 qRound(node[6].toDouble()));
}
void text(const QJsonArray& node, QWidget* parent, bool key = false) {
    auto* label = new QLabel(node[7].toString(), parent);
    label->setProperty("figmaNodeId", node[0].toString());
    label->setGeometry(bounds(node).adjusted(0, 0, key ? 0 : 8, 0));
    label->setTextFormat(Qt::PlainText);
    label->setStyleSheet(
        key ? "background:transparent;border:0;color:#505868;font:600 12px 'Inter';"
            : "background:transparent;border:0;color:#202020;font:700 22px 'Noto Sans SC';");
}
} // namespace
QWidget* createButtonGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                             QWidget* parent) {
    QFile file(":/figma/gallery-layouts.json");
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Figma gallery evidence missing");
    QJsonArray gallery;
    for (const auto& value : QJsonDocument::fromJson(file.readAll()).object()["pages"].toArray()) {
        const auto page = value.toObject();
        if (page["id"] == "172:2710")
            gallery = page["galleries"].toArray()[0].toArray();
    }
    if (gallery.isEmpty())
        throw std::runtime_error("Button gallery evidence missing");
    QFontDatabase::addApplicationFont(":/figma/fonts/Inter-SemiBold.otf");
    QFontDatabase::addApplicationFont(":/figma/fonts/NotoSansSC-Bold.otf");
    auto* canvas = new QWidget;
    canvas->setObjectName("figmaGalleryCanvas");
    canvas->setFixedSize(bounds(gallery).size());
    canvas->setProperty("galleryNodeId", gallery[0].toString());
    canvas->setProperty("variantSpecimenCount", 132);
    canvas->setProperty("componentSetCount", 7);
    canvas->setProperty("visualReviewStatus", "captured-light-runtime-review");
    canvas->setStyleSheet("QWidget#figmaGalleryCanvas {background:white;border-radius:16px;}");
    for (const auto& child : gallery[7].toArray()) {
        const auto node = child.toArray();
        auto* group = new QWidget(canvas);
        group->setGeometry(bounds(node));
        if (node[1] == "Variant key") {
            group->setObjectName("variantKey");
            group->setStyleSheet("QWidget#variantKey {background:#f7f9fc;border-radius:10px;}");
            for (const auto& key : node[7].toArray())
                text(key.toArray(), group, true);
            continue;
        }
        group->setProperty("figmaNodeId", node[0].toString());
        for (const auto& entry : node[7].toArray()) {
            const auto set = entry.toArray();
            if (set[2] == "TEXT") {
                text(set, group);
                continue;
            }
            auto* setHost = new QWidget(group);
            setHost->setGeometry(bounds(set));
            setHost->setProperty("figmaComponentSetId", set[0].toString());
            setHost->setObjectName("figmaSet_" + set[0].toString().replace(':', '_'));
            const auto family = set[1].toString().section('/', -1);
            for (const auto& specimen : set[7].toArray()) {
                const auto variant = specimen.toArray();
                const auto name = variant[1].toString();
                const auto size = name.section(',', 0, 0).section('=', 1).trimmed();
                const auto state = name.section("State=", 1).trimmed();
                auto* b = qobject_cast<commonWidget::Z_PushButton*>(
                    factory->create("commonWidget::Z_PushButton", setHost));
                if (!b)
                    throw std::runtime_error("Z_PushButton unavailable");
                QString error;
                if (!factory->apply(b, "ButtonAppearanceAdapter.setAppearance", "figma-light",
                                    &error))
                    throw std::runtime_error(error.toStdString());
                auto* adapter = b->findChild<ButtonAppearanceAdapter*>();
                if (!adapter)
                    throw std::runtime_error("SDK Button appearance adapter unavailable");
                b->setText("Button");
                b->setAccessibleName("Button");
                b->setType(family == "Primary" || family == "Danger"
                               ? commonWidget::ButtonType::Primary
                           : family == "Dashed" ? commonWidget::ButtonType::Dashed
                           : family == "Text"   ? commonWidget::ButtonType::Text
                           : family == "Link"   ? commonWidget::ButtonType::Link
                                                : commonWidget::ButtonType::Default);
                b->setDanger(family == "Danger");
                b->setSize(size == "large"    ? commonWidget::ButtonSize::L
                           : size == "medium" ? commonWidget::ButtonSize::M
                           : size == "xs"     ? commonWidget::ButtonSize::XS
                                              : commonWidget::ButtonSize::S);
                adapter->setAppearance("figma-light");
                if (family == "IconOnly") {
                    b->setLeadingIconSource(":/figma/button/plus-default.svg");
                    b->setIconOnly(true);
                }
                b->setProperty("figmaNodeId", variant[0].toString());
                b->setProperty("figmaVariant", name);
                b->setProperty("figmaPreviewState", state);
                b->setProperty("figmaDesignBounds", bounds(variant));
                b->setFixedSize(bounds(variant).size());
                b->move(bounds(variant).topLeft());
                b->setEnabled(state != "disabled");
                b->setLoading(state == "loading");
                b->setAttribute(Qt::WA_TransparentForMouseEvents);
                b->setFocusPolicy(Qt::NoFocus);
                adapter->refresh();
            }
        }
    }
    return new FigmaGalleryView(canvas, parent);
}
} // namespace native_gallery
