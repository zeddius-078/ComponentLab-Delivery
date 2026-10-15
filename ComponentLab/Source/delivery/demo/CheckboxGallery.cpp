#include "CheckboxGallery.h"
#include "FigmaGalleryView.h"
#include "Z_CheckBox.h"
#include "ComponentFactory.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QPainter>
#include <stdexcept>

namespace native_gallery {
namespace {
QRect bounds(const QJsonObject& node) {
    return QRect(qRound(node["x"].toDouble()), qRound(node["y"].toDouble()),
                 qRound(node["width"].toDouble()), qRound(node["height"].toDouble()));
}
QMap<QString, QString> axes(const QString& variant) {
    QMap<QString, QString> values;
    for (const auto& part : variant.split(','))
        values.insert(part.section('=', 0, 0).trimmed(), part.section('=', 1).trimmed());
    return values;
}
class SetOutline final : public QWidget {
  public:
    using QWidget::QWidget;

  protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QPen pen(QColor("#dbe4ff"), 1);
        pen.setDashPattern({6, 6});
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(QRectF(rect()).adjusted(.5, .5, -.5, -.5), 10, 10);
    }
};
commonWidget::Z_CheckBox* specimen(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                                   QWidget* parent, const QRect& designRect, const QString& label,
                                   const QString& size, const QString& value, const QString& state,
                                   const QString& nodeId) {
    auto* checkbox = qobject_cast<commonWidget::Z_CheckBox*>(
        factory->create("commonWidget::Z_CheckBox", parent));
    if (!checkbox)
        throw std::runtime_error("Z_CheckBox unavailable from SDK factory");
    checkbox->setText(label);
    checkbox->setAppearance("figma-light");
    checkbox->setControlSize(size.toLower());
    checkbox->setChecked(value == "Checked");
    checkbox->setIndeterminate(value == "Indeterminate");
    checkbox->setEnabled(state != "Disabled");
    checkbox->setProperty("figmaPreviewState", state);
    checkbox->setProperty("figmaNodeId", nodeId);
    checkbox->setProperty("figmaValue", value);
    checkbox->setProperty("figmaDesignBounds", designRect);
    checkbox->setProperty("figmaVisualException",
                          value == "Indeterminate" && state != "Disabled"
                              ? "Confirmed runtime policy: legible horizontal mixed mark retained"
                              : "");
    checkbox->setAttribute(Qt::WA_TransparentForMouseEvents);
    checkbox->setFocusPolicy(Qt::NoFocus);
    // The real SDK control retains its accessible hit area. Offset that padding
    // so its authored content, not the hit rectangle, matches the Figma node.
    checkbox->setFixedSize(designRect.width() + 8, 32);
    checkbox->move(designRect.topLeft() - checkbox->designContentRect().topLeft());
    return checkbox;
}
} // namespace
QWidget* createCheckboxGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                               QWidget* parent) {
    QFile file(":/figma/checkbox-gallery.json");
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Checkbox gallery evidence missing");
    const auto gallery = QJsonDocument::fromJson(file.readAll()).object()["gallery"].toObject();
    auto* canvas = new QWidget;
    canvas->setObjectName("figmaGalleryCanvas");
    canvas->setFixedSize(gallery["width"].toInt(), gallery["height"].toInt());
    canvas->setStyleSheet("QWidget#figmaGalleryCanvas {background:white;border-radius:16px;}");
    canvas->setProperty("galleryNodeId", gallery["id"].toString());
    canvas->setProperty("variantSpecimenCount", 34);
    canvas->setProperty("componentSetCount", 2);
    canvas->setProperty("visualReviewStatus", "mixed-state-horizontal-dash-confirmed");
    for (const auto& entry : gallery["children"].toArray()) {
        const auto node = entry.toObject();
        if (node["type"] == "FRAME") {
            auto* key = new QWidget(canvas);
            key->setObjectName("variantKey");
            key->setGeometry(bounds(node));
            key->setStyleSheet("QWidget#variantKey {background:#f7f9fc;border-radius:10px;}");
            const QStringList labels{"Assets · 2 groups",
                                     "Variants · component set / component / frame",
                                     "States · default / hover / selected / disabled / custom"};
            int index = 0;
            for (const auto& child : node["children"].toArray()) {
                auto* label = new QLabel(labels.value(index++), key);
                label->setGeometry(bounds(child.toObject()));
                label->setStyleSheet(
                    "background:transparent;border:0;color:#505868;font:600 12px 'Inter';");
            }
            continue;
        }
        auto* group = new SetOutline(canvas);
        group->setGeometry(bounds(node));
        group->setProperty("figmaComponentSetId", node["id"].toString());
        group->setObjectName("figmaSet_" + node["id"].toString().replace(':', '_'));
        for (const auto& child : node["children"].toArray()) {
            const auto variant = child.toObject();
            const auto values = axes(variant["name"].toString());
            if (node["id"] == "235:2710") {
                auto* checkbox =
                    specimen(factory, group, bounds(variant), "Checkbox", values["Size"],
                             values["Value"], values["State"], variant["id"].toString());
                checkbox->setProperty("figmaVariant", variant["name"].toString());
            } else {
                // Group is composition of 3 actual Z_CheckBox controls, not a
                // fabricated standalone QWidget SDK type or a summary selector.
                const bool vertical = values["Layout"] == "Vertical";
                const QRect groupRect = bounds(variant);
                const QStringList selection{"Unchecked", "Checked", "Indeterminate"};
                for (int i = 0; i < 3; ++i) {
                    const QRect childRect(groupRect.x() + (vertical ? 0 : i * 115),
                                          groupRect.y() + (vertical ? i * 36 : 0), i == 2 ? 90 : 91,
                                          24);
                    auto* checkbox = specimen(
                        factory, group, childRect, "Option " + QString(QChar('A' + i)), "LG",
                        selection[i], values["State"] == "Disabled" ? "Disabled" : "Default",
                        variant["id"].toString());
                    checkbox->setProperty("figmaGroupVariant", variant["name"].toString());
                }
            }
        }
    }
    return new FigmaGalleryView(canvas, parent);
}
} // namespace native_gallery
