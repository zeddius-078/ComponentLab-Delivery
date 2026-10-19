#include "SelectionGallery.h"
#include "ComponentFactory.h"
#include "RadioAppearance.h"
#include "FigmaGalleryView.h"
#include <QBoxLayout>
#include <QButtonGroup>
#include <QFile>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QRadioButton>
#include <stdexcept>

namespace native_gallery {
namespace {
QRect bounds(const QJsonArray& n) {
    return QRect(qRound(n[3].toDouble()), qRound(n[4].toDouble()), qRound(n[5].toDouble()),
                 qRound(n[6].toDouble()));
}
QRadioButton* radio(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                    QWidget* parent) {
    auto* button =
        qobject_cast<QRadioButton*>(factory->create("commonWidget::Z_RadioButton", parent));
    if (!button)
        throw std::runtime_error("RadioButton SDK identity missing");
    return button;
}
} // namespace

QWidget* createRadioGroupPreview(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                                 bool vertical, bool enabled, bool snapshot, QWidget* parent) {
    auto* host = new QWidget(parent);
    host->setObjectName("radioGroupComposition");
    host->setProperty("designIdentity", "ComponentLab/RadioGroup");
    host->setProperty("runtimeComposition", "QButtonGroup + layout + commonWidget::Z_RadioButton");
    host->setProperty("publicAdapterImplemented", false);
    auto* layout =
        new QBoxLayout(vertical ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight, host);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(12);
    layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    auto* group = new QButtonGroup(host);
    group->setObjectName("radioExclusiveGroup");
    group->setExclusive(true);
    for (int id = 1; id <= 3; ++id) {
        auto* button = radio(factory, host);
        button->setObjectName(QString("radioOption%1").arg(id));
        button->setText(QString("Option %1").arg(id));
        commonWidget::applyFigmaRadioAppearance(button, snapshot ? "Default" : "");
        button->setFixedSize(180, 32);
        layout->addWidget(button);
        group->addButton(button, id);
    }
    group->button(1)->setChecked(true);
    host->setProperty("checkedId", group->checkedId());
    QObject::connect(group, &QButtonGroup::idToggled, host, [host, group](int, bool) {
        host->setProperty("checkedId", group->checkedId());
    });
    host->setEnabled(enabled);
    host->setMinimumSize(vertical ? QSize(196, 136) : QSize(580, 48));
    return host;
}

QWidget* createRadioGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                            QWidget* parent) {
    QFile file(":/figma/gallery-layouts.json");
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Gallery evidence missing");
    QJsonArray gallery;
    for (const auto& value : QJsonDocument::fromJson(file.readAll()).object()["pages"].toArray())
        if (value.toObject()["id"] == "341:2")
            gallery = value.toObject()["galleries"].toArray()[0].toArray();
    if (gallery.isEmpty())
        throw std::runtime_error("Radio gallery evidence missing");
    QFile visuals(":/figma/radio-visuals.json");
    if (!visuals.open(QIODevice::ReadOnly))
        throw std::runtime_error("Radio composition evidence missing");
    const auto groupVariants = QJsonDocument::fromJson(visuals.readAll())
                                   .object()["sets"]
                                   .toArray()[1]
                                   .toObject()["variants"]
                                   .toArray();
    QFontDatabase::addApplicationFont(":/figma/fonts/Inter-SemiBold.otf");
    auto* canvas = new QWidget;
    canvas->setObjectName("figmaGalleryCanvas");
    canvas->setFixedSize(bounds(gallery).size());
    canvas->setProperty("galleryNodeId", gallery[0].toString());
    canvas->setProperty("variantSpecimenCount", 14);
    canvas->setProperty("componentSetCount", 2);
    canvas->setStyleSheet("QWidget#figmaGalleryCanvas {background:white;border-radius:16px;}");
    for (const auto& child : gallery[7].toArray()) {
        const auto node = child.toArray();
        auto* group = new QWidget(canvas);
        group->setGeometry(bounds(node));
        if (node[1] == "Variant key") {
            group->setObjectName("variantKey");
            group->setStyleSheet("QWidget#variantKey {background:#f7f9fc;border-radius:10px;}");
            for (const auto& entry : node[7].toArray()) {
                const auto key = entry.toArray();
                auto* label = new QLabel(key[7].toString(), group);
                QFont font("HarmonyOS Sans");
                font.setPixelSize(12);
                font.setWeight(QFont::DemiBold);
                label->setFont(font);
                label->setGeometry(bounds(key));
                label->setStyleSheet("background:transparent;border:0;color:#505868;"
                                     "font-family:'HarmonyOS Sans';font-size:12px;font-weight:600;");
            }
            continue;
        }
        group->setProperty("figmaComponentSetId", node[0].toString());
        for (const auto& entry : node[7].toArray()) {
            const auto variant = entry.toArray();
            const auto name = variant[1].toString();
            QWidget* widget = nullptr;
            if (node[0] == "341:53") {
                auto* button = radio(factory, group);
                button->setAutoExclusive(
                    false); // independent state specimens, not one selection group
                button->setText("Radio option");
                button->setChecked(name.startsWith("Value=Selected"));
                commonWidget::applyFigmaRadioAppearance(button, name.section("State=", 1));
                widget = button;
            } else {
                widget = createRadioGroupPreview(factory, name.contains("Vertical"),
                                                 !name.contains("Disabled"), true, group);
                // Keep nested instance IDs, not fabricated standalone public types.
                int option = 1;
                QJsonArray instances;
                for (const auto& captured : groupVariants)
                    if (captured.toObject()["id"] == variant[0])
                        instances = captured.toObject()["children"].toArray();
                for (const auto& nested : instances) {
                    const auto instance = nested.toObject();
                    auto* button =
                        widget->findChild<QRadioButton*>(QString("radioOption%1").arg(option++));
                    if (button) {
                        const auto rect = instance["rect"].toArray();
                        button->setProperty("figmaNodeId", instance["id"].toString());
                        button->setProperty("figmaDesignBounds",
                                            QRect(rect[0].toInt(), rect[1].toInt(), rect[2].toInt(),
                                                  rect[3].toInt()));
                    }
                }
            }
            widget->setProperty("figmaNodeId", variant[0].toString());
            widget->setProperty("figmaVariant", name);
            widget->setProperty("figmaDesignBounds", bounds(variant));
            widget->setFixedSize(bounds(variant).size());
            widget->move(bounds(variant).topLeft());
            widget->setEnabled(!name.contains("Disabled"));
        }
    }
    return new FigmaGalleryView(canvas, parent);
}
} // namespace native_gallery
