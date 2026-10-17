#include "SliderAppearance.h"
#include <QEvent>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QPainter>
#include <QProxyStyle>
#include <QSlider>
#include <QStyleFactory>
#include <QStyleOptionSlider>
#include <QSvgRenderer>
#include <QVariant>
#include <QtMath>
#include <stdexcept>

namespace commonWidget {
namespace {
bool booleanProperty(const QMap<QString, QString>& properties, const QString& name,
                     bool defaultValue) {
    if (!properties.contains(name))
        return defaultValue;
    const auto value = properties.value(name);
    if (value == "True")
        return true;
    if (value == "False")
        return false;
    throw std::invalid_argument(("Invalid Slider boolean property: " + name).toStdString());
}

QString visualState(const QSlider* slider) {
    if (!slider->isEnabled())
        return "Disabled";
    const auto snapshot = slider->property("figmaSliderSnapshotState").toString();
    if (!snapshot.isEmpty())
        return snapshot;
    return slider->isSliderDown() ? "Dragging" : slider->hasFocus() ? "Focus" : "Default";
}

QFont valueFont() {
    QFont font("HarmonyOS Sans");
    font.setPixelSize(14);
    font.setWeight(QFont::Medium);
    return font;
}

struct Geometry {
    QRectF rail, active, handle, ring, bubble;
    QRect nativeGroove, nativeHandle;
    QString label;
};

Geometry geometry(const QSlider* slider, const QStyleOptionSlider* option) {
    Geometry g;
    const bool vertical = slider->orientation() == Qt::Vertical;
    const bool snapshot = slider->property("figmaSliderSnapshotGeometry").toBool();
    const int start = vertical ? 30 : 10,
              span = qMax(0, (vertical ? slider->height() : slider->width()) - 2 * start);
    const int position = QStyle::sliderPositionFromValue(
        option->minimum, option->maximum, option->sliderPosition, span, option->upsideDown);
    const int center = start + position;
    // Native QSlider derives pointer/value conversion from groove extent minus
    // handle length. Visual rail excludes the half-handle on each end.
    g.nativeGroove =
        vertical ? QRect(41, start - 9, 18, span + 18) : QRect(start - 9, 28, span + 18, 18);
    g.nativeHandle = vertical ? QRect(41, center - 9, 18, 18) : QRect(center - 9, 28, 18, 18);
    g.rail = vertical ? QRectF(47, start, 6, span) : QRectF(start, 34, span, 6);
    const int minimumPosition = option->upsideDown ? start + span : start;
    g.active = vertical
                   ? QRectF(47, qMin(center, minimumPosition), 6, qAbs(center - minimumPosition))
                   : QRectF(qMin(center, minimumPosition), 34, qAbs(center - minimumPosition), 6);
    const int diameter = visualState(slider) == "Dragging" ? 20 : 18;
    g.handle = QRectF(g.nativeHandle.topLeft(), QSizeF(diameter, diameter));
    if (snapshot) {
        g.handle =
            vertical ? QRectF(41, 144, diameter, diameter) : QRectF(108, 28, diameter, diameter);
        g.active = vertical ? QRectF(47, 150, 6, 80) : QRectF(10, 34, 104, 6);
    }
    g.ring = QRectF(g.handle.topLeft() - QPointF(3, 3), QSizeF(24, 24));
    const auto label = slider->property("figmaSliderValueLabel");
    g.label = slider->property("figmaSliderAutomaticLabel").toBool()
                  ? QString::number(slider->sliderPosition())
                  : label.toString();
    const int labelWidth = qCeil(QFontMetricsF(valueFont()).horizontalAdvance(g.label));
    const int width = snapshot && g.label == "40" ? 19
                      : snapshot                  ? labelWidth
                                 : qMin(labelWidth, vertical ? 33 : slider->width());
    g.bubble = vertical ? QRectF(0, snapshot ? 141 : center - 12, width, 24)
                        : QRectF(snapshot ? 95 : center - 22, 0, width, 24);
    if (!snapshot && !vertical)
        g.bubble.moveLeft(qBound(0., g.bubble.x(), qreal(slider->width() - width)));
    return g;
}

class SliderStyle final : public QProxyStyle {
public:
    explicit SliderStyle(QObject* owner) : QProxyStyle(QStyleFactory::create("Fusion")) {
        setParent(owner);
    }

    QRect subControlRect(ComplexControl control, const QStyleOptionComplex* option, SubControl sub,
                         const QWidget* widget) const override {
        const auto* slider = qobject_cast<const QSlider*>(widget);
        const auto* opt = qstyleoption_cast<const QStyleOptionSlider*>(option);
        if (control != CC_Slider || !slider || !opt)
            return QProxyStyle::subControlRect(control, option, sub, widget);
        const auto g = geometry(slider, opt);
        if (sub == SC_SliderHandle)
            return g.nativeHandle;
        if (sub == SC_SliderGroove)
            return g.nativeGroove;
        return slider->rect();
    }

    SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex* option,
                                     const QPoint& point, const QWidget* widget) const override {
        if (control != CC_Slider || !qobject_cast<const QSlider*>(widget))
            return QProxyStyle::hitTestComplexControl(control, option, point, widget);
        if (subControlRect(control, option, SC_SliderHandle, widget).contains(point))
            return SC_SliderHandle;
        if (subControlRect(control, option, SC_SliderGroove, widget).contains(point))
            return SC_SliderGroove;
        return SC_None;
    }

    int pixelMetric(PixelMetric metric, const QStyleOption* option,
                    const QWidget* widget) const override {
        if (qobject_cast<const QSlider*>(widget)) {
            if (metric == PM_SliderLength)
                return 18;
            if (metric == PM_SliderThickness)
                return 18;
        }
        return QProxyStyle::pixelMetric(metric, option, widget);
    }

    void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option,
                            QPainter* painter, const QWidget* widget) const override {
        const auto* slider = qobject_cast<const QSlider*>(widget);
        const auto* opt = qstyleoption_cast<const QStyleOptionSlider*>(option);
        if (control != CC_Slider || !slider || !opt) {
            QProxyStyle::drawComplexControl(control, option, painter, widget);
            return;
        }
        const auto g = geometry(slider, opt);
        const auto state = visualState(slider);
        const bool disabled = !slider->isEnabled(),
                   vertical = slider->orientation() == Qt::Vertical;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor("#d0d0d0"));
        painter->drawRoundedRect(g.rail, 3, 3);
        painter->setBrush(QColor(disabled ? "#8a8a8a" : "#6235f7"));
        if (!g.active.isEmpty())
            painter->drawRoundedRect(g.active, 3, 3);
        if (state == "Focus" || state == "Dragging") {
            QSvgRenderer ring(QStringLiteral(":/figma/slider/FocusRing.svg"));
            ring.render(painter, g.ring);
        }
        QSvgRenderer handle(QString(":/figma/slider/%1.svg")
                                .arg(disabled              ? "HandleDisabled"
                                     : state == "Dragging" ? "HandleDragging"
                                                           : "Handle"));
        handle.render(painter, g.handle);
        if (slider->property("figmaSliderShowValue").toBool() && !g.label.isEmpty()) {
            painter->setBrush(QColor(disabled ? "#f3f3f3" : "#191919"));
            painter->drawRoundedRect(g.bubble, 6, 6);
            painter->setFont(valueFont());
            painter->setPen(QColor(disabled ? "#444444" : "#ffffff"));
            painter->drawText(
                g.bubble, Qt::AlignCenter | Qt::TextSingleLine,
                QFontMetricsF(valueFont()).elidedText(g.label, Qt::ElideRight, g.bubble.width()));
            painter->setPen(Qt::NoPen);
        }
        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->setBrush(QColor(disabled ? "#8a8a8a" : "#a7a7a7"));
        if (slider->tickPosition() != QSlider::NoTicks) {
            const bool snapshot = slider->property("figmaSliderSnapshotGeometry").toBool();
            const qint64 minimum = opt->minimum, maximum = opt->maximum;
            qint64 interval =
                slider->tickInterval() > 0 ? slider->tickInterval() : qMax(1, slider->singleStep());
            // A dense integer range must not turn painting into billions of steps.
            // Thin only by whole interval multiples; never reinterpret interval as pixels.
            if ((maximum - minimum) / interval > 2048)
                interval *= ((maximum - minimum) / interval + 2047) / 2048;
            const int span = vertical ? qRound(g.rail.height()) : qRound(g.rail.width());
            const int count = snapshot ? 10 : int((maximum - minimum) / interval);
            for (int i = 0; i <= count; ++i) {
                const int position =
                    snapshot ? qRound(span * i / 10.0)
                             : QStyle::sliderPositionFromValue(opt->minimum, opt->maximum,
                                                               int(minimum + i * interval), span,
                                                               opt->upsideDown);
                if (slider->tickPosition() & QSlider::TicksBelow)
                    painter->drawRect(vertical ? QRectF(62, 30 + position, 5, 1)
                                               : QRectF(10 + position, 48, 1, 5));
                if (slider->tickPosition() & QSlider::TicksAbove)
                    painter->drawRect(vertical ? QRectF(33, 30 + position, 5, 1)
                                               : QRectF(10 + position, 21, 1, 5));
            }
        }
        auto* mutableSlider = const_cast<QSlider*>(slider);
        mutableSlider->setProperty("figmaSliderHandleAssetBounds", g.handle);
        mutableSlider->setProperty("figmaSliderFocusAssetBounds", g.ring);
        mutableSlider->setProperty("figmaSliderActiveTrackBounds", g.active);
        mutableSlider->setProperty("figmaSliderBubbleBounds", g.bubble);
        painter->restore();
    }
};

class SliderBinding final : public QObject {
public:
    explicit SliderBinding(QSlider* slider) : QObject(slider), slider_(slider) {
        setObjectName("figmaSliderBinding");
        slider_->installEventFilter(this);
        for (const auto signal : {&QAbstractSlider::valueChanged, &QAbstractSlider::sliderMoved})
            QObject::connect(slider_, signal, this, [this] {
                slider_->update();
            });
        QObject::connect(slider_, &QAbstractSlider::sliderPressed, this, [this] {
            slider_->update();
        });
        QObject::connect(slider_, &QAbstractSlider::sliderReleased, this, [this] {
            slider_->update();
        });
    }

protected:
    bool eventFilter(QObject*, QEvent* event) override {
        if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut ||
            event->type() == QEvent::EnabledChange)
            slider_->update();
        return false;
    }

private:
    QSlider* slider_;
};
}

void configureFigmaSlider(QSlider* slider, const QMap<QString, QString>& properties,
                          bool sourceSnapshot) {
    if (!slider)
        throw std::invalid_argument("Slider is required");
    const auto orientation = properties.value("Orientation");
    const auto visual = properties.value("State");
    if (!QStringList{"Horizontal", "Vertical"}.contains(orientation) ||
        !QStringList{"Default", "Focus", "Dragging", "Disabled"}.contains(visual))
        throw std::invalid_argument("Invalid Figma Slider variant");

    slider->setOrientation(orientation == "Vertical" ? Qt::Vertical : Qt::Horizontal);
    slider->setEnabled(visual != "Disabled");
    const bool ticks = booleanProperty(properties, "Show ticks", false);
    slider->setTickPosition(!ticks ? QSlider::NoTicks
                                  : slider->orientation() == Qt::Vertical ? QSlider::TicksRight
                                                                          : QSlider::TicksBelow);
    applyFigmaSliderAppearance(slider, visual == "Disabled" ? QString() : visual,
                               sourceSnapshot,
                               booleanProperty(properties, "Show value", true),
                               properties.value("Value label"));
}

void applyFigmaSliderAppearance(QSlider* slider, const QString& snapshotState,
                                bool snapshotGeometry, bool showValue, const QString& valueLabel) {
    if (!slider || !QStringList{"", "Default", "Focus", "Dragging"}.contains(snapshotState))
        throw std::invalid_argument(
            "Captured Slider requires QSlider and a source-backed snapshot state");
    static const int font = QFontDatabase::addApplicationFont(":/fonts/HarmonyOS_Sans_Regular.ttf");
    Q_UNUSED(font);
    slider->setProperty("figmaAppearance", "figma-light");
    slider->setProperty("designSystemStyleOwner", "figma-slider-captured");
    slider->setProperty("figmaSliderSnapshotState", snapshotState);
    slider->setProperty("figmaSliderSnapshotGeometry", snapshotGeometry);
    slider->setProperty("figmaSliderShowValue", showValue);
    slider->setProperty("figmaSliderAutomaticLabel", valueLabel.isNull());
    slider->setProperty("figmaSliderValueLabel", valueLabel);
    if (!slider->findChild<QObject*>("figmaSliderBinding", Qt::FindDirectChildrenOnly)) {
        slider->setStyle(new SliderStyle(slider));
        new SliderBinding(slider);
        slider->ensurePolished();
    }
    if (slider->orientation() == Qt::Horizontal) {
        slider->setMinimumWidth(280);
        slider->setMaximumWidth(QWIDGETSIZE_MAX);
        slider->setFixedHeight(80);
        slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    } else {
        slider->setMinimumHeight(260);
        slider->setMaximumHeight(QWIDGETSIZE_MAX);
        slider->setFixedWidth(100);
        slider->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }
    slider->update();
}
}
