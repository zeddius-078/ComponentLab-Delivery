#include "EvidenceGallery.h"
#include "FigmaGalleryView.h"
#include "FigmaLayoutEvidence.h"
#include "MyDesignSystem.h"

#include <QPainter>
#include <QPainterPath>
#include <QSet>
#include <stdexcept>

namespace native_gallery {
namespace {

const QSet<QString>& evidencePages() {
    static const QSet<QString> pages{
        QStringLiteral("397:2"),   QStringLiteral("400:2"), QStringLiteral("402:2"),
        QStringLiteral("433:7"),   QStringLiteral("436:8"), QStringLiteral("242:2720"),
        QStringLiteral("346:2"),   QStringLiteral("351:2"), QStringLiteral("359:2"),
        QStringLiteral("363:2"),   QStringLiteral("405:2"), QStringLiteral("542:2"),
        QStringLiteral("553:15"),
        QStringLiteral("567:2"),   QStringLiteral("579:33"), QStringLiteral("727:14"),
        QStringLiteral("735:2"),   QStringLiteral("737:2")};
    return pages;
}

QColor stateColor(const QString& value) {
    const QString lower = value.toLower();
    if (lower.contains("error") || lower.contains("critical") || lower.contains("danger"))
        return QColor("#c83c3c");
    if (lower.contains("warning"))
        return QColor("#c17d00");
    if (lower.contains("success"))
        return QColor("#2f7d4a");
    if (lower.contains("disabled"))
        return QColor("#9a9a9a");
    return QColor("#6235f7");
}

class EvidenceSpecimen final : public QWidget {
public:
    EvidenceSpecimen(QString pageId, QString setName, QString variant, QWidget* parent)
        : QWidget(parent), m_pageId(std::move(pageId)), m_setName(std::move(setName)),
          m_variant(std::move(variant)) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setFocusPolicy(Qt::NoFocus);
        setProperty("figmaVariant", m_variant);
        setProperty("figmaComponentSet", m_setName);
        setProperty("contractOnly", true);
        setProperty("evidenceSource", "/figma/gallery-layouts.json");
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QString state = figmaAxisValue(m_variant, QStringLiteral("State"));
        const bool disabled = state.compare(QStringLiteral("Disabled"), Qt::CaseInsensitive) == 0;
        const bool focus = state.compare(QStringLiteral("Focus"), Qt::CaseInsensitive) == 0;
        const QColor accent = stateColor(figmaAxisValue(m_variant, QStringLiteral("Type")) + state);
        const QColor ink = disabled ? QColor("#999999") : QColor("#202124");
        const QColor surface = disabled ? QColor("#f3f3f3") : QColor("#ffffff");
        const QRectF outer = rect().adjusted(1, 1, -1, -1);

        p.setPen(QPen(focus ? QColor("#6d4aff") : QColor("#d7dbe3"), focus ? 2 : 1));
        p.setBrush(surface);
        p.drawRoundedRect(outer, 8, 8);

        const QString family = m_setName.section('/', -1);
        if (m_setName.contains("Chart")) {
            drawChart(p, outer, ink, accent);
        } else if (m_setName.contains("MetricCard") || m_setName.contains("Statistic")) {
            drawMetric(p, outer, ink, accent);
        } else if (m_setName.contains("ImagePanel")) {
            drawImage(p, outer, ink);
        } else if (m_setName.contains("Empty") || m_setName.contains("Result")) {
            drawEmptyOrResult(p, outer, ink, accent);
        } else if (m_setName.contains("Select")) {
            drawSelect(p, outer, ink, accent);
        } else if (m_setName.contains("Switch")) {
            drawSwitch(p, outer, ink, accent);
        } else if (m_setName.contains("Chip") || m_setName.contains("MenuItem")) {
            drawChip(p, outer, ink, accent);
        } else if (m_setName.contains("Card")) {
            drawCard(p, outer, ink, accent);
        } else if (m_setName.contains("Form") || m_setName.contains("Descriptions")) {
            drawForm(p, outer, ink, accent);
        } else if (m_setName.contains("Divider") || m_setName.contains("LayoutPrimitive")) {
            drawLayout(p, outer, ink, accent);
        } else if (m_setName.contains("Collapse")) {
            drawCollapse(p, outer, ink, accent);
        } else if (m_setName.contains("Drawer")) {
            drawDrawer(p, outer, ink, accent);
        } else if (m_setName.contains("Alert") || m_setName.contains("Message") ||
                   m_setName.contains("Notification")) {
            drawFeedback(p, outer, ink, accent);
        } else if (m_setName.contains("Spin")) {
            drawSpin(p, outer, ink, accent);
        } else if (m_setName.contains("Skeleton")) {
            drawSkeleton(p, outer);
        } else {
            drawGeneric(p, outer, family, ink, accent);
        }

        p.setPen(disabled ? QColor("#999999") : QColor("#596273"));
        QFont caption = font();
        caption.setPixelSize(qBound(9, height() / 18, 12));
        caption.setWeight(QFont::Medium);
        p.setFont(caption);
        p.drawText(QRectF(10, height() - 24, width() - 20, 17),
                   Qt::AlignLeft | Qt::AlignVCenter, m_variant);
    }

private:
    void title(QPainter& p, const QRectF& r, const QString& text, const QColor& ink) const {
        QFont f = font();
        f.setPixelSize(qBound(11, height() / 13, 16));
        f.setWeight(QFont::DemiBold);
        p.setFont(f);
        p.setPen(ink);
        p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, text);
    }

    void drawChart(QPainter& p, const QRectF& r, const QColor& ink, const QColor& accent) const {
        title(p, QRectF(22, 14, r.width() - 44, 24), QStringLiteral("Chart title"), ink);
        p.setPen(QColor("#777e8c"));
        p.drawText(QRectF(22, 38, r.width() - 44, 18), QStringLiteral("Last 7 days"));
        const QRectF plot(48, 76, r.width() - 76, r.height() - 116);
        p.setPen(QColor("#e4e7ec"));
        for (int i = 0; i < 4; ++i)
            p.drawLine(plot.left(), plot.top() + i * plot.height() / 3.0, plot.right(),
                       plot.top() + i * plot.height() / 3.0);
        if (figmaAxisValue(m_variant, QStringLiteral("State")) == QStringLiteral("Empty")) {
            p.setPen(QPen(QColor("#c9ced7"), 1, Qt::DashLine));
            p.setBrush(QColor("#f7f8fa"));
            p.drawRoundedRect(plot.adjusted(42, 18, -42, -18), 8, 8);
            p.setPen(QColor("#8a909d"));
            p.drawText(plot, Qt::AlignCenter, QStringLiteral("No data"));
            return;
        }
        const QString kind = figmaAxisValue(m_variant, QStringLiteral("Kind"));
        if (kind == QStringLiteral("Line")) {
            QPainterPath path;
            for (int i = 0; i < 7; ++i) {
                const QPointF point(plot.left() + i * plot.width() / 6.0,
                                    plot.bottom() - (0.25 + ((i * 37) % 55) / 100.0) * plot.height());
                i == 0 ? path.moveTo(point) : path.lineTo(point);
            }
            p.setPen(QPen(accent, 3));
            p.setBrush(Qt::NoBrush);
            p.drawPath(path);
            for (int i = 0; i < 7; ++i) {
                const QPointF point(plot.left() + i * plot.width() / 6.0,
                                    plot.bottom() - (0.25 + ((i * 37) % 55) / 100.0) * plot.height());
                p.setBrush(accent);
                p.drawEllipse(point, 4, 4);
            }
        } else if (kind == QStringLiteral("Status")) {
            const QColor colors[]{QColor("#2f7d4a"), QColor("#c17d00"), QColor("#c83c3c")};
            for (int i = 0; i < 3; ++i) {
                p.setPen(Qt::NoPen);
                p.setBrush(colors[i]);
                p.drawRoundedRect(QRectF(plot.left(), plot.top() + 12 + i * plot.height() / 3.2,
                                         plot.width() * (0.78 - i * .12), 18), 4, 4);
            }
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#315e4b"));
            for (int i = 0; i < 7; ++i) {
                const qreal h = plot.height() * (0.28 + ((i * 31) % 55) / 100.0);
                p.drawRoundedRect(QRectF(plot.left() + i * plot.width() / 7.0 + 7,
                                         plot.bottom() - h, plot.width() / 10.0, h), 3, 3);
            }
        }
    }

    void drawMetric(QPainter& p, const QRectF& r, const QColor& ink, const QColor& accent) const {
        title(p, QRectF(18, 14, r.width() - 36, 22), QStringLiteral("Process yield"), ink);
        const QString state = figmaAxisValue(m_variant, QStringLiteral("State"));
        if (state == QStringLiteral("Loading")) {
            p.setPen(Qt::NoPen); p.setBrush(QColor("#e7e9ee"));
            p.drawRoundedRect(QRectF(18, 52, r.width() * .55, 22), 5, 5);
        } else if (state == QStringLiteral("Empty")) {
            p.setPen(QColor("#8a909d")); p.drawText(QRectF(18, 48, r.width() - 36, 34), QStringLiteral("—"));
        } else if (state == QStringLiteral("Error")) {
            p.setPen(QColor("#c83c3c")); p.drawText(QRectF(18, 48, r.width() - 36, 34), QStringLiteral("Data unavailable"));
        } else {
            QFont f = font(); f.setPixelSize(qBound(22, height() / 4, 38)); f.setBold(true); p.setFont(f);
            p.setPen(ink); p.drawText(QRectF(18, 42, r.width() - 36, 48), QStringLiteral("98.6%"));
            p.setPen(accent); p.drawText(QRectF(18, 88, r.width() - 36, 18), QStringLiteral("+1.2%"));
        }
    }

    void drawImage(QPainter& p, const QRectF& r, const QColor& ink) const {
        const QRectF image = r.adjusted(14, 14, -14, -52);
        QLinearGradient gradient(image.topLeft(), image.bottomRight());
        gradient.setColorAt(0, QColor("#dfe6f5")); gradient.setColorAt(1, QColor("#b5c5b9"));
        p.setPen(Qt::NoPen); p.setBrush(gradient); p.drawRoundedRect(image, 7, 7);
        p.setPen(QPen(QColor("#ffffff"), 4)); p.setBrush(Qt::NoBrush);
        p.drawEllipse(image.center(), image.width() / 7, image.width() / 7);
        title(p, QRectF(16, r.bottom() - 48, r.width() - 32, 22), QStringLiteral("Wafer inspection"), ink);
    }

    void drawEmptyOrResult(QPainter& p, const QRectF& r, const QColor& ink, const QColor& accent) const {
        const QPointF center(r.center().x(), r.top() + r.height() * .33);
        p.setPen(QPen(accent, 3)); p.setBrush(QColor("#f1edff"));
        p.drawEllipse(center, qMin(32.0, r.width() / 10), qMin(32.0, r.width() / 10));
        title(p, QRectF(20, center.y() + 42, r.width() - 40, 28),
              m_setName.contains("Result") ? QStringLiteral("Operation complete") : QStringLiteral("No content"), ink);
        p.setPen(QColor("#747b88"));
        p.drawText(QRectF(20, center.y() + 72, r.width() - 40, 36), Qt::AlignLeft | Qt::TextWordWrap,
                   QStringLiteral("There is currently nothing to display."));
        if (figmaAxisValue(m_variant, QStringLiteral("Action")) == QStringLiteral("Primary")) {
            p.setPen(Qt::NoPen); p.setBrush(accent);
            p.drawRoundedRect(QRectF(20, r.bottom() - 64, 112, 32), 6, 6);
            p.setPen(Qt::white); p.drawText(QRectF(20, r.bottom() - 64, 112, 32), Qt::AlignCenter, QStringLiteral("Continue"));
        }
    }

    void drawSelect(QPainter& p, const QRectF& r, const QColor& ink, const QColor& accent) const {
        const bool selected = m_variant.contains("Selected") || m_variant.contains("Value") ||
                              m_variant.contains("Multiple") || m_variant.contains("Tags");
        p.setPen(QPen(m_variant.contains("Error") ? QColor("#c83c3c") : accent,
                      m_variant.contains("Focus") ? 2 : 1));
        p.setBrush(QColor("#ffffff"));
        p.drawRoundedRect(r.adjusted(12, 16, -12, -42), 6, 6);
        p.setPen(ink);
        p.drawText(r.adjusted(24, 20, -42, -46), Qt::AlignVCenter,
                   selected ? QStringLiteral("ETCH-01") : QStringLiteral("Select an option"));
        p.drawText(QRectF(r.right() - 34, r.top() + 20, 16, 18), Qt::AlignCenter, QStringLiteral("⌄"));
    }

    void drawSwitch(QPainter& p, const QRectF& r, const QColor& ink, const QColor& accent) const {
        const bool on = m_variant.contains("Checked") || m_variant.contains("On") ||
                        m_variant.contains("Selected");
        const QRectF track(r.left() + 20, r.top() + r.height() * .30, qMin(68.0, r.width() * .36), 32);
        p.setPen(Qt::NoPen); p.setBrush(on ? accent : QColor("#c8cdd6")); p.drawRoundedRect(track, 16, 16);
        p.setBrush(Qt::white); const qreal cx = on ? track.right() - 16 : track.left() + 16;
        p.drawEllipse(QPointF(cx, track.center().y()), 12, 12);
        title(p, QRectF(track.right() + 14, track.top(), r.right() - track.right() - 26, 32), QStringLiteral("Enable"), ink);
    }

    void drawChip(QPainter& p, const QRectF& r, const QColor& ink, const QColor& accent) const {
        const QRectF chip = r.adjusted(12, 14, -12, qMin(-42.0, -r.height() * .35));
        p.setPen(QPen(accent, 1)); p.setBrush(QColor("#f1edff")); p.drawRoundedRect(chip, chip.height() / 2, chip.height() / 2);
        p.setPen(ink); p.drawText(chip.adjusted(14, 0, -14, 0), Qt::AlignVCenter, QStringLiteral("Process tag"));
    }

    void drawCard(QPainter& p, const QRectF& r, const QColor& ink, const QColor& accent) const {
        p.setPen(Qt::NoPen); p.setBrush(QColor("#eef1f6")); p.drawRoundedRect(r.adjusted(14, 14, -14, -r.height() * .52), 6, 6);
        title(p, QRectF(18, r.height() * .51, r.width() - 36, 24), QStringLiteral("Equipment card"), ink);
        p.setPen(QColor("#747b88")); p.drawText(QRectF(18, r.height() * .63, r.width() - 36, 38), Qt::TextWordWrap, QStringLiteral("Status and summary content"));
        p.setPen(Qt::NoPen); p.setBrush(accent); p.drawRoundedRect(QRectF(18, r.bottom() - 52, 92, 28), 5, 5);
    }

    void drawForm(QPainter& p, const QRectF& r, const QColor& ink, const QColor& accent) const {
        const int rows = qBound(2, int(r.height() / 72), 6);
        for (int i = 0; i < rows; ++i) {
            const qreal y = 18 + i * (r.height() - 48) / rows;
            p.setPen(ink); p.drawText(QRectF(16, y, r.width() * .28, 24), Qt::AlignVCenter, QStringLiteral("Field %1").arg(i + 1));
            p.setPen(QPen(i == 0 && m_variant.contains("Focus") ? accent : QColor("#d2d6dd")));
            p.setBrush(QColor("#ffffff")); p.drawRoundedRect(QRectF(r.width() * .32, y, r.width() * .62, 30), 5, 5);
        }
    }

    void drawLayout(QPainter& p, const QRectF& r, const QColor& ink, const QColor& accent) const {
        const bool vertical = m_variant.contains("Vertical");
        p.setPen(QPen(accent, 2));
        if (m_setName.contains("Divider")) {
            vertical ? p.drawLine(r.center().x(), r.top() + 18, r.center().x(), r.bottom() - 42)
                     : p.drawLine(r.left() + 18, r.center().y(), r.right() - 18, r.center().y());
        } else {
            p.setBrush(QColor("#f3f5f8"));
            for (int i = 0; i < 3; ++i) {
                const QRectF box = vertical ? QRectF(18, 18 + i * (r.height() - 54) / 3.0, r.width() - 36, (r.height() - 70) / 3.0)
                                            : QRectF(18 + i * (r.width() - 36) / 3.0, 18, (r.width() - 54) / 3.0, r.height() - 60);
                p.drawRoundedRect(box, 5, 5);
            }
        }
        Q_UNUSED(ink);
    }

    void drawCollapse(QPainter& p, const QRectF& r, const QColor& ink, const QColor& accent) const {
        for (int i = 0; i < 3; ++i) {
            const QRectF row(14, 14 + i * (r.height() - 48) / 3.0, r.width() - 28, (r.height() - 66) / 3.0);
            p.setPen(QPen(i == 0 && m_variant.contains("Focus") ? accent : QColor("#d7dbe3")));
            p.setBrush(i == 0 ? QColor("#faf9ff") : Qt::NoBrush); p.drawRoundedRect(row, 5, 5);
            p.setPen(ink); p.drawText(row.adjusted(12, 0, -12, 0), Qt::AlignVCenter, QStringLiteral("Section %1   ›").arg(i + 1));
        }
    }

    void drawDrawer(QPainter& p, const QRectF& r, const QColor& ink, const QColor& accent) const {
        const QString placement = figmaAxisValue(m_variant, QStringLiteral("Placement"));
        QRectF drawer = r.adjusted(12, 12, -12, -40);
        if (placement == "Left") drawer.setWidth(drawer.width() * .45);
        else if (placement == "Right") { drawer.setLeft(drawer.left() + drawer.width() * .55); }
        else if (placement == "Top") drawer.setHeight(drawer.height() * .48);
        else { drawer.setTop(drawer.top() + drawer.height() * .52); }
        p.setPen(QPen(accent, m_variant.contains("Focus") ? 2 : 1)); p.setBrush(QColor("#faf9ff")); p.drawRoundedRect(drawer, 6, 6);
        title(p, drawer.adjusted(14, 8, -14, -drawer.height() + 36), QStringLiteral("Drawer"), ink);
    }

    void drawFeedback(QPainter& p, const QRectF& r, const QColor& ink, const QColor& accent) const {
        const QRectF banner = r.adjusted(14, 16, -14, -44);
        p.setPen(QPen(accent, 1)); p.setBrush(accent.lighter(190)); p.drawRoundedRect(banner, 7, 7);
        p.setBrush(accent); p.setPen(Qt::NoPen); p.drawEllipse(QPointF(banner.left() + 22, banner.top() + 22), 10, 10);
        title(p, QRectF(banner.left() + 42, banner.top() + 9, banner.width() - 54, 26), m_setName.section('/', -1), ink);
        p.setPen(QColor("#5d6470")); p.drawText(QRectF(banner.left() + 42, banner.top() + 38, banner.width() - 54, banner.height() - 48), Qt::TextWordWrap, QStringLiteral("Operation status message"));
    }

    void drawSpin(QPainter& p, const QRectF& r, const QColor& ink, const QColor& accent) const {
        const qreal radius = qMin(r.width(), r.height()) * .16;
        p.setPen(QPen(QColor("#e0e3e8"), qMax(4.0, radius / 5))); p.setBrush(Qt::NoBrush); p.drawEllipse(r.center(), radius, radius);
        p.setPen(QPen(accent, qMax(4.0, radius / 5), Qt::SolidLine, Qt::RoundCap)); p.drawArc(QRectF(r.center().x()-radius,r.center().y()-radius,radius*2,radius*2), 25*16, 220*16);
        title(p, QRectF(14, r.center().y() + radius + 12, r.width() - 28, 24), QStringLiteral("Loading"), ink);
    }

    void drawSkeleton(QPainter& p, const QRectF& r) const {
        p.setPen(Qt::NoPen); p.setBrush(QColor("#e7e9ed"));
        p.drawEllipse(QPointF(34, 38), 18, 18);
        p.drawRoundedRect(QRectF(64, 22, r.width() * .45, 13), 6, 6);
        p.drawRoundedRect(QRectF(64, 45, r.width() * .30, 11), 5, 5);
        for (int i = 0; i < 3; ++i)
            p.drawRoundedRect(QRectF(18, 78 + i * 27, r.width() - 36 - i * 22, 11), 5, 5);
    }

    void drawGeneric(QPainter& p, const QRectF& r, const QString& family, const QColor& ink, const QColor& accent) const {
        title(p, QRectF(16, 18, r.width() - 32, 28), family, ink);
        p.setPen(Qt::NoPen); p.setBrush(accent.lighter(190)); p.drawRoundedRect(r.adjusted(16, 58, -16, -48), 6, 6);
    }

    QString m_pageId;
    QString m_setName;
    QString m_variant;
};

} // namespace

bool hasEvidenceGallery(const QString& pageId) { return evidencePages().contains(pageId); }

QWidget* createEvidenceGallery(
    const commonWidget::DesignComponentPage& page,
    const std::shared_ptr<commonWidget::ComponentFactory>& factory,
    QWidget* parent) {
    const auto layout = figmaGalleryLayout(page.id);
    auto* canvas = new QWidget;
    canvas->setObjectName(QStringLiteral("figmaGalleryCanvas"));
    canvas->setFixedSize(layout.size);
    canvas->setStyleSheet(
        QStringLiteral("QWidget#figmaGalleryCanvas {background:white;border-radius:16px;}"));
    canvas->setProperty("galleryNodeId", layout.galleryId);
    canvas->setProperty("evidenceBacked", true);
    canvas->setProperty("contractOnly", false);
    canvas->setProperty("runtimeBacked", true);
    canvas->setProperty("figmaPageId", page.id);
    QMap<QString, commonWidget::DesignComponentDescriptor> descriptorBySet;
    for (const auto& descriptor : commonWidget::designComponentDescriptors()) {
        for (const auto& id : descriptor.componentSetId.split(',', Qt::SkipEmptyParts))
            descriptorBySet.insert(id, descriptor);
        for (const auto& id : descriptor.variantSourceComponentSetId.split(',', Qt::SkipEmptyParts))
            descriptorBySet.insert(id, descriptor);
    }
    int runtimeSpecimenCount = 0;
    for (const auto& item : layout.variants) {
        QWidget* specimen = nullptr;
        const auto descriptor = descriptorBySet.value(item.componentSetId);
        if (factory && descriptor.instantiable &&
            commonWidget::ComponentFactory::isRegistered(descriptor.sdkClass)) {
            auto* candidate = factory->create(descriptor.sdkClass, canvas);
            if (candidate->metaObject()->indexOfProperty("variantTuple") >= 0) {
                QString error;
                if (!factory->apply(candidate, QStringLiteral("setVariantTuple"), item.variant,
                                    &error))
                    throw std::runtime_error(error.toStdString());
                specimen = candidate;
                specimen->setObjectName(QStringLiteral("figmaRuntimeSpecimen"));
                specimen->setProperty("contractOnly", false);
                specimen->setProperty("runtimeBacked", true);
                ++runtimeSpecimenCount;
            } else {
                delete candidate;
            }
        }
        if (!specimen) {
            specimen = new EvidenceSpecimen(page.id, item.componentSetName, item.variant, canvas);
            specimen->setObjectName(QStringLiteral("figmaEvidenceSpecimen"));
            specimen->setProperty("runtimeBacked", false);
        }
        specimen->setGeometry(item.rect);
        specimen->setProperty("figmaComponentSetId", item.componentSetId);
        specimen->setProperty("figmaNodeId", item.nodeId);
    }
    canvas->setProperty("componentSetCount", layout.componentSetCount);
    canvas->setProperty("variantSpecimenCount", layout.variants.size());
    canvas->setProperty("runtimeSpecimenCount", runtimeSpecimenCount);
    return new FigmaGalleryView(canvas, parent);
}

} // namespace native_gallery
