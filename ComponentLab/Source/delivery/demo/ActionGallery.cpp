#include "ActionGallery.h"
#include "ComponentFactory.h"
#include "CommandLinkAppearance.h"
#include "ToolButtonAppearance.h"
#include "DialogButtonBoxAppearance.h"
#include "Z_PushButton.h"
#include <QDialogButtonBox>
#include <QLayout>
#include "FigmaGalleryView.h"
#include <QCommandLinkButton>
#include <QToolButton>
#include <QFile>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <functional>
#include <stdexcept>

namespace native_gallery {
namespace {
QRect bounds(const QJsonArray& n) {
    return QRect(qRound(n[3].toDouble()), qRound(n[4].toDouble()), qRound(n[5].toDouble()),
                 qRound(n[6].toDouble()));
}
} // namespace
QWidget* createActionGallery(const QString& pageId,
                             const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                             QWidget* parent) {
    if (pageId != "450:2" && pageId != "443:2" && pageId != "447:2")
        throw std::invalid_argument("Unsupported captured action gallery");
    QFile file(":/figma/gallery-layouts.json");
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Figma gallery evidence missing");
    QJsonArray gallery;
    for (const auto& value : QJsonDocument::fromJson(file.readAll()).object()["pages"].toArray())
        if (value.toObject()["id"] == pageId)
            gallery = value.toObject()["galleries"].toArray()[0].toArray();
    if (gallery.isEmpty())
        throw std::runtime_error("Action gallery evidence missing");
    QFontDatabase::addApplicationFont(":/figma/fonts/Inter-SemiBold.otf");
    QFontDatabase::addApplicationFont(":/figma/fonts/Inter-Regular.otf");
    auto* canvas = new QWidget;
    canvas->setObjectName("figmaGalleryCanvas");
    canvas->setFixedSize(bounds(gallery).size());
    canvas->setProperty("galleryNodeId", gallery[0].toString());
    canvas->setProperty("variantSpecimenCount", pageId == "447:2"   ? 32
                                                : pageId == "443:2" ? 34
                                                                    : 16);
    canvas->setProperty("componentSetCount", 1);
    canvas->setStyleSheet("QWidget#figmaGalleryCanvas {background:white;border-radius:16px;}");
    QJsonArray dialogVariants;
    if (pageId == "447:2") {
        QFile evidence(":/figma/dialogactions-visuals.json");
        if (!evidence.open(QIODevice::ReadOnly))
            throw std::runtime_error("Dialog Actions evidence missing");
        dialogVariants = QJsonDocument::fromJson(evidence.readAll()).object()["variants"].toArray();
    }
    std::function<void(const QJsonArray&, QWidget*, bool)> build = [&](const QJsonArray& node,
                                                                       QWidget* host, bool key) {
        if (node[2] == "COMPONENT") {
            if (pageId == "447:2") {
                auto* box = qobject_cast<QDialogButtonBox*>(
                    factory->create("commonWidget::Z_DialogButtonBox", host));
                if (!box)
                    throw std::runtime_error("Native dialog button box unavailable");
                QJsonArray source;
                for (const auto& item : dialogVariants)
                    if (item.toArray()[0] == node[0])
                        source = item.toArray();
                if (source.isEmpty())
                    throw std::runtime_error("Dialog variant not captured");
                const auto name = node[1].toString();
                box->setOrientation(name.contains("Orientation=Vertical") ? Qt::Vertical
                                                                          : Qt::Horizontal);
                box->setCenterButtons(name.contains("Centered=True"));
                commonWidget::applyFigmaDialogButtonBoxAppearance(box);
                const auto row = source[3].toArray();
                for (const auto& value : source[4].toArray()) {
                    const auto child = value.toArray();
                    const auto roleName = child[1].toString().section(" / ", 0, 0);
                    const auto role = roleName == "AcceptRole"  ? QDialogButtonBox::AcceptRole
                                      : roleName == "ApplyRole" ? QDialogButtonBox::ApplyRole
                                      : roleName == "ResetRole" ? QDialogButtonBox::ResetRole
                                      : roleName == "HelpRole"  ? QDialogButtonBox::HelpRole
                                      : roleName == "DestructiveRole"
                                          ? QDialogButtonBox::DestructiveRole
                                          : QDialogButtonBox::RejectRole;
                    auto* b = qobject_cast<commonWidget::Z_PushButton*>(
                        factory->create("commonWidget::Z_PushButton", box));
                    if (!b)
                        throw std::runtime_error("Canonical Button unavailable");
                    b->setText(child[3].toString());
                    b->setAccessibleName(b->text());
                    b->setType(role == QDialogButtonBox::AcceptRole ||
                                       role == QDialogButtonBox::ApplyRole ||
                                       role == QDialogButtonBox::DestructiveRole
                                   ? commonWidget::ButtonType::Primary
                                   : commonWidget::ButtonType::Default);
                    b->setDanger(role == QDialogButtonBox::DestructiveRole);
                    b->setSize(commonWidget::ButtonSize::M);
                    b->setProperty("figmaPreviewState", "default");
                    b->setProperty("figmaNodeId", child[0].toString());
                    box->addButton(b, role);
                    const auto rect = child[2].toArray();
                    const QRect geometry(qRound(row[0].toDouble() + rect[0].toDouble()),
                                         qRound(row[1].toDouble() + rect[1].toDouble()),
                                         qRound(rect[2].toDouble()), qRound(rect[3].toDouble()));
                    b->setProperty("figmaDesignBounds", geometry);
                    b->setFixedSize(geometry.size());
                    b->setAutoDefault(false);
                    b->setFocusPolicy(Qt::NoFocus);
                    b->setAttribute(Qt::WA_TransparentForMouseEvents);
                }
                // Only the inert authored gallery suspends native layout. Live editing never does.
                box->layout()->setEnabled(false);
                box->setProperty("figmaDialogSourceSnapshot", true);
                box->setProperty("figmaNodeId", node[0].toString());
                box->setProperty("figmaVariant", name);
                box->setProperty("figmaDesignBounds", bounds(node));
                box->setEnabled(!name.contains("State=Disabled"));
                box->setFixedSize(bounds(node).size());
                box->move(bounds(node).topLeft());
                for (auto* b : box->buttons())
                    b->setGeometry(b->property("figmaDesignBounds").toRect());
                box->setAttribute(Qt::WA_TransparentForMouseEvents);
                return;
            }
            if (pageId == "443:2") {
                auto* button =
                    qobject_cast<QToolButton*>(factory->create("commonWidget::Z_ToolButton", host));
                if (!button)
                    throw std::runtime_error("Native tool button unavailable");
                const auto name = node[1].toString();
                const auto sourceState = name.section(", ", 2, 2).section('=', 1);
                button->setText("Format");
                button->setIcon(commonWidget::figmaToolButtonIcon());
                button->setIconSize(QSize(16, 16));
                commonWidget::configureFigmaToolButton(
                    button, {{"Surface", name.contains("Surface=AutoRaise") ? "AutoRaise" : "Framed"},
                             {"Menu", name.contains("Menu=Split") ? "Split" : "None"},
                             {"State", sourceState}, {"Show label", "True"}}, true);
                button->setProperty("figmaNodeId", node[0].toString());
                button->setProperty("figmaVariant", name);
                button->setProperty("figmaDesignBounds", bounds(node));
                button->setFixedSize(bounds(node).size());
                button->move(bounds(node).topLeft());
                button->setFocusPolicy(Qt::NoFocus);
                button->setAttribute(Qt::WA_TransparentForMouseEvents);
                return;
            }
            auto* button = qobject_cast<QCommandLinkButton*>(
                factory->create("commonWidget::Z_CommandLinkButton", host));
            if (!button)
                throw std::runtime_error("Native command link unavailable");
            const auto name = node[1].toString();
            const auto sourceState = name.section(", ", 1, 1).section('=', 1);
            button->setText("Create a new workspace");
            button->setDescription("Start with an empty workspace and configure it later.");
            button->setIcon(commonWidget::figmaCommandLinkIcon());
            button->setIconSize(QSize(16, 16));
            button->setAutoDefault(false);
            commonWidget::configureFigmaCommandLinkButton(
                button, {{"Surface", name.contains("Surface=Flat") ? "Flat" : "Raised"},
                         {"State", sourceState}, {"Show description", "True"},
                         {"Show icon", "True"}}, true);
            button->setProperty("figmaNodeId", node[0].toString());
            button->setProperty("figmaVariant", name);
            button->setProperty("figmaDesignBounds", bounds(node));
            button->setFixedSize(bounds(node).size());
            button->move(bounds(node).topLeft());
            button->setFocusPolicy(Qt::NoFocus);
            button->setAttribute(Qt::WA_TransparentForMouseEvents);
            return;
        }
        if (node[2] == "TEXT") {
            auto* label = new QLabel(node[7].toString(), host);
            label->setGeometry(bounds(node));
            label->setTextFormat(Qt::PlainText);
            const bool heading = node[1].toString().startsWith("Component set");
            QFont font("HarmonyOS Sans");
            font.setPixelSize(key ? 12 : heading ? 22 : 13);
            font.setWeight(key || heading ? QFont::DemiBold : QFont::Normal);
            label->setFont(font);
            label->setStyleSheet(
                QString(
                    "background:transparent;border:0;color:%1;font-family:'HarmonyOS Sans';font-size:%2px;")
                    .arg(key       ? "#505868"
                         : heading ? "#202124"
                                   : "#666666")
                    .arg(font.pixelSize()));
            return;
        }
        auto* group = new QWidget(host);
        group->setGeometry(bounds(node));
        group->setProperty("figmaNodeId", node[0].toString());
        if (node[1] == "Variant key") {
            group->setObjectName("variantKey");
            group->setStyleSheet("QWidget#variantKey {background:#f7f9fc;border-radius:10px;}");
            key = true;
        }
        if (node[1] == "Component variants") {
            group->setObjectName("actionVariants");
            group->setStyleSheet("QWidget#actionVariants {background:#fafafa;}");
        }
        if (node[2] == "COMPONENT_SET")
            group->setProperty("figmaComponentSetId", node[0].toString());
        for (const auto& child : node[7].toArray())
            build(child.toArray(), group, key);
    };
    for (const auto& child : gallery[7].toArray())
        build(child.toArray(), canvas, false);
    return new FigmaGalleryView(canvas, parent);
}
} // namespace native_gallery
