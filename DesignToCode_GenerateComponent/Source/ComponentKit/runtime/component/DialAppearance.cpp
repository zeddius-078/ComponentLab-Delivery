#include "DialAppearance.h"
#include "../surface/Surface.h"
#include <QDial>
#include <QEvent>
#include <QFile>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOptionSlider>
#include <QSvgRenderer>
#include <QVariant>
#include <QtMath>
#include <stdexcept>

namespace commonWidget {
namespace {
QString visualState(const QDial* dial) {
    if (!dial->isEnabled())
        return "Disabled";
    const auto snapshot = dial->property("figmaDialSnapshotState").toString();
    if (!snapshot.isEmpty())
        return snapshot;
    return dial->isSliderDown() ? "Dragging" : dial->hasFocus() ? "Focus" : "Default";
}

QSizeF asset(QPainter* painter, const QString& name, const QPointF& origin) {
    // viewBoxF preserves fractional export dimensions; defaultSize rounds them.
    QSvgRenderer svg(":/figma/dial/" + name + ".svg");
    const QSizeF size = svg.viewBoxF().size();
    svg.render(painter, QRectF(origin, size));
    return size;
}

const QJsonObject& sourceGeometry() {
    static const auto data = [] {
        QFile file(":/figma/dial-visuals.json");
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("Dial source evidence missing");
        return QJsonDocument::fromJson(file.readAll()).object()["sourceGeometry"].toObject();
    }();
    return data;
}

void capturedNotches(QPainter* painter, bool wrapping) {
    const auto children =
        sourceGeometry()[wrapping ? "on" : "off"].toObject()["children"].toArray();
    for (const auto& child : children) {
        const auto node = child.toObject();
        if (node["name"] != "Notch")
            continue;
        const auto matrix = node["transform"].toArray();
        const auto a = matrix[0].toArray(), b = matrix[1].toArray();
        painter->save();
        painter->setWorldTransform(QTransform(a[0].toDouble(), b[0].toDouble(), a[1].toDouble(),
                                              b[1].toDouble(), a[2].toDouble(), b[2].toDouble()),
                                   true);
        painter->drawRoundedRect(QRectF(0, 0, 2, 7), 1, 1);
        painter->restore();
    }
}

class DialStyle final : public QProxyStyle {
public:
    explicit DialStyle(QDial* dial) : QProxyStyle(QStyleFactory::create("Fusion")) {
        setParent(dial);
    }

    void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option,
                            QPainter* painter, const QWidget* widget) const override {
        const auto* dial = qobject_cast<const QDial*>(widget);
        const auto* opt = qstyleoption_cast<const QStyleOptionSlider*>(option);
        if (control != CC_Dial || !dial || !opt) {
            QProxyStyle::drawComplexControl(control, option, painter, widget);
            return;
        }
        const auto state = visualState(dial);
        const bool disabled = !dial->isEnabled(), dragging = state == "Dragging";
        const bool snapshot = dial->property("figmaDialSnapshotGeometry").toBool();
        const bool wrapping = dial->wrapping();
        const QString mode = wrapping ? "On" : "Off", tone = disabled ? "Disabled" : "";
        const QPointF center =
            snapshot ? QPointF(90, 86) : QPointF(dial->width() / 2.0, dial->height() / 2.0);
        const qreal range = qreal(opt->maximum) - opt->minimum;
        qreal fraction = range > 0 ? (qreal(opt->sliderPosition) - opt->minimum) / range : 0;
        fraction = qBound<qreal>(0, fraction, 1);
        const qreal start = wrapping ? -270 : -240, sweep = wrapping ? 360 : 300;
        const qreal angle =
            snapshot ? (wrapping ? 54 : -27)
                     : start + sweep * (dial->invertedAppearance() ? 1 - fraction : fraction);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        if (snapshot) {
            // Exported SVGs are tightly cropped to their authored arc geometry.
            asset(painter, "Range" + mode + tone,
                  wrapping ? QPointF(30, 26) : QPointF(47.5736, 26));
            asset(painter, "Active" + mode + tone,
                  wrapping ? QPointF(90, 26) : QPointF(47.5736, 26));
        } else {
            // This arc is value-driven, not a replacement for a static Figma asset.
            const QRectF arc(center.x() - 54.6, center.y() - 54.6, 109.2, 109.2);
            painter->setPen(QPen(QColor("#d0d0d0"), 10.8, Qt::SolidLine, Qt::FlatCap));
            painter->drawArc(arc, qRound(-start * 16), qRound(-sweep * 16));
            painter->setPen(
                QPen(QColor(disabled ? "#8a8a8a" : "#6235f7"), 10.8, Qt::SolidLine, Qt::FlatCap));
            const qreal activeStart = dial->invertedAppearance() ? start + sweep : start;
            const qreal activeSweep = sweep * fraction * (dial->invertedAppearance() ? -1 : 1);
            if (fraction > 0)
                painter->drawArc(arc, qRound(-activeStart * 16), qRound(-activeSweep * 16));
            painter->setPen(Qt::NoPen);
        }
        painter->setBrush(QColor(disabled ? "#8a8a8a" : "#a7a7a7"));
        int notchCount = 0;
        if (dial->notchesVisible()) {
            if (snapshot) {
                capturedNotches(painter, wrapping);
                notchCount = 12;
            } else if (opt->tickInterval > 0 && range > 0) {
                qint64 interval = opt->tickInterval;
                const qint64 span = qint64(opt->maximum) - opt->minimum;
                if (span / interval > 2048)
                    interval *= (span / interval + 2047) / 2048;
                // Wrapping shares both endpoints; do not draw the same mark twice.
                const qint64 last =
                    wrapping && span % interval == 0 ? span / interval - 1 : span / interval;
                for (qint64 i = 0; i <= last; ++i) {
                    qreal position = qreal(i * interval) / range;
                    if (dial->invertedAppearance())
                        position = 1 - position;
                    painter->save();
                    painter->translate(center);
                    painter->rotate(start + sweep * position + 90);
                    painter->drawRoundedRect(QRectF(-1, -70.5, 2, 7), 1, 1);
                    painter->restore();
                    ++notchCount;
                }
            }
        }
        if (state == "Focus")
            asset(painter, "FocusRing", center - QPointF(70, 70));
        painter->save();
        painter->translate(center);
        painter->rotate(angle);
        asset(painter,
              "Pointer" + mode +
                  (disabled   ? "Disabled"
                   : dragging ? "Dragging"
                              : ""),
              QPointF(0, dragging ? -5 : -4));
        painter->restore();
        if (dragging) {
            // QtSvg 5 does not render SVG filter effects. Keep the original hub
            // asset and render its independently measured Figma effect behind it.
            int padding = 0;
            const QJsonObject shadow{
                {"x", 0},       {"y", 2}, {"blur", 6}, {"spread", 0}, {"color", "#3d1fcc"},
                {"alpha", 0.22}};
            const auto image = surface::shadowImage(QSize(16, 16), dial->devicePixelRatioF(), 8, 0,
                                                    shadow, false, &padding);
            painter->drawImage(center - QPointF(8 + padding, 8 + padding), image);
        }
        asset(painter,
              disabled   ? "HubDisabled"
              : dragging ? "HubDragging"
                         : "Hub",
              center - (dragging ? QPointF(14, 12) : QPointF(8, 8)));
        const QString label = dial->property("figmaDialAutomaticLabel").toBool()
                                  ? QString::number(dial->sliderPosition())
                                  : dial->property("figmaDialValueLabel").toString();
        if (dial->property("figmaDialShowValue").toBool()) {
            QFont font("Inter");
            font.setPixelSize(14);
            font.setWeight(QFont::Medium);
            painter->setFont(font);
            painter->setPen(QColor(disabled ? "#8a8a8a" : "#191919"));
            const QRectF box = snapshot ? QRectF(80.5, 154, 19, 14)
                                        : QRectF(center.x() - 80, center.y() + 68, 160, 14);
            painter->drawText(box, Qt::AlignCenter | Qt::TextSingleLine,
                              QFontMetricsF(font).elidedText(label, Qt::ElideRight, box.width()));
        }
        auto* mutableDial = const_cast<QDial*>(dial);
        mutableDial->setProperty("figmaDialCenter", center);
        mutableDial->setProperty("figmaDialPointerAngle", angle);
        mutableDial->setProperty("figmaDialNotchCount", notchCount);
        mutableDial->setProperty("figmaDialVisualState", state);
        mutableDial->setProperty("figmaDialGeometryAuthority",
                                 snapshot ? "Figma captured geometry" : "Qt native value / angle");
        painter->restore();
    }
};

class DialBinding final : public QObject {
public:
    explicit DialBinding(QDial* dial) : QObject(dial), dial_(dial) {
        setObjectName("figmaDialBinding");
        dial_->installEventFilter(this);
        for (const auto signal : {&QAbstractSlider::valueChanged, &QAbstractSlider::sliderMoved})
            QObject::connect(dial_, signal, this, [this] {
                dial_->update();
            });
        QObject::connect(dial_, &QAbstractSlider::sliderPressed, this, [this] {
            dial_->update();
        });
        QObject::connect(dial_, &QAbstractSlider::sliderReleased, this, [this] {
            dial_->update();
        });
    }

protected:
    bool eventFilter(QObject*, QEvent* event) override {
        if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut ||
            event->type() == QEvent::EnabledChange)
            dial_->update();
        return false;
    }

private:
    QDial* dial_;
};
}

void applyFigmaDialAppearance(QDial* dial, const QString& snapshotState, bool snapshotGeometry,
                              bool showValue, const QString& valueLabel) {
    if (!dial || !QStringList{"", "Default", "Focus", "Dragging"}.contains(snapshotState))
        throw std::invalid_argument(
            "Captured Dial requires QDial and a source-backed snapshot state");
    static const int font = QFontDatabase::addApplicationFont(":/figma/fonts/Inter-Medium.otf");
    Q_UNUSED(font);
    dial->setProperty("figmaAppearance", "figma-light");
    dial->setProperty("designSystemStyleOwner", "figma-dial-captured");
    dial->setProperty("figmaDialSnapshotState", snapshotState);
    dial->setProperty("figmaDialSnapshotGeometry", snapshotGeometry);
    dial->setProperty("figmaDialShowValue", showValue);
    dial->setProperty("figmaDialAutomaticLabel", valueLabel.isNull());
    dial->setProperty("figmaDialValueLabel", valueLabel);
    if (!dial->findChild<QObject*>("figmaDialBinding", Qt::FindDirectChildrenOnly)) {
        dial->setStyle(new DialStyle(dial));
        new DialBinding(dial);
        dial->ensurePolished();
    }
    dial->update();
}
}
