#include "ToolBoxGallery.h"
#include "ToolBoxAppearance.h"
#include "ComponentFactory.h"
#include "FigmaGalleryView.h"
#include <QAbstractButton>
#include <QFile>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLayout>
#include <QPainterPath>
#include <QRegion>
#include <QScrollArea>
#include <QToolBox>
#include <functional>
#include <stdexcept>

namespace native_gallery {
namespace {
QRect bounds(const QJsonArray& n) {
    return QRect(qRound(n[3].toDouble()), qRound(n[4].toDouble()), qRound(n[5].toDouble()),
                 qRound(n[6].toDouble()));
}
QLabel* label(QWidget* parent, const QString& text, const QRect& rect, int size, int weight,
              const QString& color, bool wrap = false) {
    auto* result = new QLabel(text, parent);
    result->setTextFormat(Qt::PlainText);
    result->setGeometry(rect);
    result->setWordWrap(wrap);
    result->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    QFont font("Inter");
    font.setPixelSize(size);
    font.setWeight(weight);
    result->setFont(font);
    result->setStyleSheet(
        QString("background:transparent;border:0;color:%1;font-family:'Inter';font-size:%2px;")
            .arg(color)
            .arg(size));
    return result;
}
QScrollArea* pageArea(QToolBox* box, int index) {
    return qobject_cast<QScrollArea*>(box->widget(index)->parentWidget()->parentWidget());
}
} // namespace
QWidget* createToolBoxGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                              QWidget* parent) {
    QFile file(":/figma/gallery-layouts.json");
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Gallery evidence missing");
    QJsonArray gallery;
    for (const auto& item : QJsonDocument::fromJson(file.readAll()).object()["pages"].toArray())
        if (item.toObject()["id"] == "505:2")
            gallery = item.toObject()["galleries"].toArray()[0].toArray();
    for (const auto* name : {"Regular", "Medium", "SemiBold"})
        QFontDatabase::addApplicationFont(QString(":/figma/fonts/Inter-%1.otf").arg(name));
    auto* canvas = new QWidget;
    canvas->setObjectName("figmaGalleryCanvas");
    canvas->setFixedSize(bounds(gallery).size());
    canvas->setProperty("galleryNodeId", "505:29");
    canvas->setProperty("variantSpecimenCount", 5);
    canvas->setProperty("componentSetCount", 1);
    canvas->setStyleSheet("QWidget#figmaGalleryCanvas {background:white;border-radius:16px;}");
    std::function<void(const QJsonArray&, QWidget*)> build = [&](const QJsonArray& node,
                                                                 QWidget* host) {
        const auto id = node[0].toString(), name = node[1].toString();
        if (node[2] == "COMPONENT") {
            auto* box = qobject_cast<QToolBox*>(factory->create("commonWidget::Z_ToolBox", host));
            if (!box)
                throw std::runtime_error("Native ToolBox unavailable");
            const bool anatomy = id != "505:555";
            box->setProperty("figmaToolBoxSourceSnapshot", true);
            box->setProperty("figmaToolBoxAnatomyOnly", anatomy);
            if (anatomy) {
                box->addItem(new QWidget, "General");
                box->setItemEnabled(0, name != "State=Disabled");
            } else
                commonWidget::populateFigmaToolBox(box, true);
            commonWidget::applyFigmaToolBoxAppearance(
                box, anatomy ? name.section('=', 1) : QString(), anatomy ? 0 : -1, !anatomy);
            if (anatomy)
                box->setStyleSheet("QToolBox[figmaToolBoxAnatomyOnly=\"true\"] "
                                   "{background:transparent;border:0;}");
            box->setProperty("figmaNodeId", id);
            box->setProperty("figmaDesignBounds", bounds(node));
            box->setProperty("figmaVariant", name);
            box->setFixedSize(bounds(node).size());
            box->move(bounds(node).topLeft());
            box->ensurePolished();
            box->layout()->setEnabled(false);
            int index = 0;
            for (int i = 0; i < box->layout()->count(); ++i)
                if (auto* button =
                        qobject_cast<QAbstractButton*>(box->layout()->itemAt(i)->widget())) {
                    button->setGeometry(anatomy ? QRect(0, 0, 760, 48)
                                                : QRect(16,
                                                        index == 0   ? 16
                                                        : index == 1 ? 64
                                                                     : 247,
                                                        760, 48));
                    button->setFocusPolicy(Qt::NoFocus);
                    if (!anatomy)
                        button->setProperty("figmaNodeId", index == 0   ? "505:556"
                                                           : index == 1 ? "505:561"
                                                                        : "505:566");
                    ++index;
                }
            if (anatomy)
                pageArea(box, 0)->hide();
            else {
                pageArea(box, 1)->setGeometry(16, 112, 768, 135);
                pageArea(box, 1)->setProperty("figmaNodeId", "505:571");
            }
            box->setAttribute(Qt::WA_TransparentForMouseEvents);
            return;
        }
        if (node[2] == "TEXT") {
            const int size = id == "505:575"   ? 11
                             : id == "505:576" ? 16
                             : id == "505:578" ? 12
                                               : 13;
            auto* text = label(host, node[7].toString(), bounds(node), size,
                               id == "505:575" || id == "505:576"   ? QFont::DemiBold
                               : id == "505:463" || id == "505:578" ? QFont::Medium
                                                                    : QFont::Normal,
                               id == "505:575"   ? "#6235f7"
                               : id == "505:578" ? "#8a8a8a"
                                                 : "#191919",
                               id == "505:577" || id == "505:578");
            text->setProperty("figmaNodeId", id);
            return;
        }
        auto* group = new QWidget(host);
        group->setGeometry(bounds(node));
        group->setProperty("figmaNodeId", id);
        group->setObjectName("toolBoxGroup_" + QString(id).replace(':', '_'));
        if (node[2] == "COMPONENT_SET") {
            group->setProperty("figmaComponentSetId", id);
            QPainterPath clip;
            clip.addRoundedRect(QRectF(group->rect()), 5, 5);
            group->setMask(QRegion(clip.toFillPolygon().toPolygon()));
        } else if (id == "505:462" || id == "505:554")
            group->setStyleSheet(QString("QWidget#%1 {background:#f3f3f3;border-radius:%2px;}")
                                     .arg(group->objectName())
                                     .arg(id == "505:462" ? 10 : 12));
        else if (id == "505:574")
            group->setStyleSheet(
                QString(
                    "QWidget#%1 {background:white;border:1px solid #d0d0d0;border-radius:10px;}")
                    .arg(group->objectName()));
        for (const auto& child : node[7].toArray())
            build(child.toArray(), group);
    };
    for (const auto& child : gallery[7].toArray())
        build(child.toArray(), canvas);
    return new FigmaGalleryView(canvas, parent);
}
} // namespace native_gallery
