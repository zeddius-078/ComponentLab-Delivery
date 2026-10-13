#include "ScrollBarAppearance.h"
#include "FocusIndicator.h"
#include "Surface.h"
#include <QEvent>
#include <QPainter>
#include <QProxyStyle>
#include <QScrollBar>
#include <QStyleFactory>
#include <QStyleOptionSlider>
#include <QSvgRenderer>
#include <QVariant>
#include <stdexcept>

namespace commonWidget {
namespace {
struct Geometry {
    QRect rail, groove, handle, subLine, addLine, subPage, addPage;
    bool vertical = false, reversed = false;
};

Geometry geometry(const QScrollBar* bar, const QStyleOptionSlider* option) {
    Geometry g;
    g.vertical = bar->orientation() == Qt::Vertical;
    g.reversed = option->upsideDown != (!g.vertical && option->direction == Qt::RightToLeft);
    const int length = g.vertical ? bar->height() : bar->width();
    const int center = (g.vertical ? bar->width() : bar->height()) / 2;
    const bool buttons = bar->property("figmaScrollBarShowLineButtons").toBool();
    const bool snapshot = bar->property("figmaScrollBarSnapshotGeometry").toBool();
    // A small viewport keeps a usable native groove instead of imposing the
    // gallery's 280x56 / 100x260 specimen frame on the host layout.
    const int edge = qMin(8, length / 4);
    const int start = buttons ? qMin(32, length / 2) : edge;
    const int span = qMax(0, length - 2 * start);
    const auto axisRect = [=](int pos, int extent, int cross, int thickness) {
        return g.vertical ? QRect(cross, pos, thickness, extent)
                          : QRect(pos, cross, extent, thickness);
    };
    g.rail = axisRect(edge, qMax(0, length - 2 * edge), center - 6, 12);
    g.groove = axisRect(start, span, center - 9, 18);
    const qint64 range = qint64(option->maximum) - option->minimum;
    const qint64 total = range + qMax(0, option->pageStep);
    int handleLength = total > 0 ? int(qint64(span) * option->pageStep / total) : span;
    handleLength = qBound(qMin(18, span), handleLength, span);
    if (range == 0)
        handleLength = span;
    int handleStart = start + QStyle::sliderPositionFromValue(option->minimum, option->maximum,
                                                              option->sliderPosition,
                                                              span - handleLength, g.reversed);
    if (snapshot) {
        handleLength = 72;
        handleStart = 86;
    }
    g.handle = axisRect(handleStart, handleLength, center - 8, 16);
    const QRect before = axisRect(start, qMax(0, handleStart - start), center - 9, 18);
    const QRect after =
        axisRect(handleStart + handleLength, qMax(0, start + span - handleStart - handleLength),
                 center - 9, 18);
    g.subPage = g.reversed ? after : before;
    g.addPage = g.reversed ? before : after;
    if (buttons && length >= 64) {
        const QRect first = axisRect(8, 18, center - 10, 18);
        const QRect last = axisRect(length - 28, 18, center - 10, 18);
        g.subLine = g.reversed ? last : first;
        g.addLine = g.reversed ? first : last;
    }
    return g;
}

QString state(const QScrollBar* bar, const QStyleOptionSlider* option) {
    if (!bar->isEnabled())
        return "Disabled";
    const auto fixed = bar->property("figmaScrollBarSnapshotState").toString();
    if (!fixed.isEmpty())
        return fixed;
    if (bar->isSliderDown())
        return "Dragging";
    return (option->state & QStyle::State_MouseOver) &&
                   (option->activeSubControls & QStyle::SC_ScrollBarSlider)
               ? "Hover"
               : "Default";
}

class ScrollBarStyle final : public QProxyStyle {
public:
    explicit ScrollBarStyle(QObject* owner) : QProxyStyle(QStyleFactory::create("Fusion")) {
        setParent(owner);
    }

    QRect subControlRect(ComplexControl control, const QStyleOptionComplex* option, SubControl sub,
                         const QWidget* widget) const override {
        const auto* bar = qobject_cast<const QScrollBar*>(widget);
        const auto* opt = qstyleoption_cast<const QStyleOptionSlider*>(option);
        if (control != CC_ScrollBar || !bar || !opt)
            return QProxyStyle::subControlRect(control, option, sub, widget);
        const auto g = geometry(bar, opt);
        switch (sub) {
        case SC_ScrollBarGroove:
            return g.groove;
        case SC_ScrollBarSlider:
            return g.handle;
        case SC_ScrollBarSubLine:
            return g.subLine;
        case SC_ScrollBarAddLine:
            return g.addLine;
        case SC_ScrollBarSubPage:
            return g.subPage;
        case SC_ScrollBarAddPage:
            return g.addPage;
        default:
            return {};
        }
    }

    SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex* option,
                                     const QPoint& point, const QWidget* widget) const override {
        if (control != CC_ScrollBar || !qobject_cast<const QScrollBar*>(widget))
            return QProxyStyle::hitTestComplexControl(control, option, point, widget);
        for (auto sub : {SC_ScrollBarSubLine, SC_ScrollBarAddLine, SC_ScrollBarSlider,
                         SC_ScrollBarSubPage, SC_ScrollBarAddPage})
            if (subControlRect(control, option, sub, widget).contains(point))
                return sub;
        return SC_None;
    }

    int pixelMetric(PixelMetric metric, const QStyleOption* option,
                    const QWidget* widget) const override {
        if (qobject_cast<const QScrollBar*>(widget))
            if (metric == PM_ScrollBarExtent || metric == PM_ScrollBarSliderMin)
                return metric == PM_ScrollBarExtent ? 20 : 18;
        return QProxyStyle::pixelMetric(metric, option, widget);
    }

    void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option,
                            QPainter* painter, const QWidget* widget) const override {
        const auto* bar = qobject_cast<const QScrollBar*>(widget);
        const auto* opt = qstyleoption_cast<const QStyleOptionSlider*>(option);
        if (control != CC_ScrollBar || !bar || !opt) {
            QProxyStyle::drawComplexControl(control, option, painter, widget);
            return;
        }
        const auto g = geometry(bar, opt);
        const auto current = state(bar, opt);
        const bool disabled = !bar->isEnabled();
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor("#d0d0d0"));
        painter->drawRoundedRect(g.rail, 6, 6);
        if (current == "Dragging") {
            int padding = 0;
            const QJsonObject shadow{{"x", g.vertical ? 2 : 0},
                                     {"y", g.vertical ? 0 : 2},
                                     {"blur", 5},
                                     {"spread", 0},
                                     {"color", "#3d1fcc"},
                                     {"alpha", 0.22}};
            const auto image = surface::shadowImage(g.handle.size(), bar->devicePixelRatioF(), 8, 0,
                                                    shadow, false, &padding);
            painter->drawImage(g.handle.topLeft() - QPoint(padding, padding), image);
        }
        painter->setBrush(QColor(disabled                ? "#f3f3f3"
                                 : current == "Dragging" ? "#6235f7"
                                 : current == "Hover"    ? "#a7a7a7"
                                                         : "#444444"));
        painter->drawRoundedRect(g.handle, 8, 8);
        for (const auto& rect : {g.subLine, g.addLine}) {
            if (rect.isEmpty())
                continue;
            painter->setBrush(QColor(disabled ? "#d0d0d0" : "#ffffff"));
            painter->drawRoundedRect(rect, 6, 6);
            const bool first =
                g.vertical ? rect.y() < bar->height() / 2 : rect.x() < bar->width() / 2;
            const QString direction =
                g.vertical ? (first ? "Up" : "Down") : (first ? "Left" : "Right");
            QSvgRenderer icon(QString(":/figma/scrollbar/%1%2.svg")
                                  .arg(direction, disabled ? "Disabled" : "Primary"));
            icon.render(painter, QRectF(rect.topLeft() + QPoint(2, 2), QSizeF(16, 16)));
        }
        // No Focus variant is authored. Opt-in native keyboard focus uses the
        // existing application focus policy, separately from captured states.
        if (FocusIndicator::instance().visibleFor(bar)) {
            painter->setBrush(Qt::NoBrush);
            painter->setPen(
                QPen(FocusIndicator::instance().color(QColor("#6235f7")), 1, Qt::DashLine));
            painter->drawRoundedRect(QRectF(g.groove).adjusted(-2, -2, 2, 2), 6, 6);
        }
        auto* mutableBar = const_cast<QScrollBar*>(bar);
        mutableBar->setProperty("figmaScrollBarHandleBounds", g.handle);
        mutableBar->setProperty("figmaScrollBarSubIconBounds",
                                g.subLine.isEmpty()
                                    ? QRect()
                                    : QRect(g.subLine.topLeft() + QPoint(2, 2), QSize(16, 16)));
        mutableBar->setProperty("figmaScrollBarAddIconBounds",
                                g.addLine.isEmpty()
                                    ? QRect()
                                    : QRect(g.addLine.topLeft() + QPoint(2, 2), QSize(16, 16)));
        mutableBar->setProperty("figmaScrollBarVisualState", current);
        painter->restore();
    }
};
}

void applyFigmaScrollBarAppearance(QScrollBar* bar, const QString& snapshotState,
                                   bool snapshotGeometry, bool showLineButtons) {
    if (!bar || !QStringList{"", "Default", "Hover", "Dragging"}.contains(snapshotState))
        throw std::invalid_argument(
            "Captured ScrollBar requires QScrollBar and a source-backed snapshot state");
    bar->setProperty("figmaAppearance", "figma-light");
    bar->setProperty("designSystemStyleOwner", "figma-scrollbar-captured");
    bar->setProperty("figmaScrollBarSnapshotState", snapshotState);
    bar->setProperty("figmaScrollBarSnapshotGeometry", snapshotGeometry);
    bar->setProperty("figmaScrollBarShowLineButtons", showLineButtons);
    if (!bar->property("figmaScrollBarStyleInstalled").toBool()) {
        bar->setStyle(new ScrollBarStyle(bar));
        bar->setProperty("figmaScrollBarStyleInstalled", true);
        FocusIndicator::instance().watch(bar);
        bar->ensurePolished();
    }
    // Native QScrollBar assumes an opaque style; this one has authored empty margins.
    bar->setAttribute(Qt::WA_OpaquePaintEvent, false);
    bar->setAttribute(Qt::WA_Hover, true);
    bar->updateGeometry();
    bar->update();
}
}
