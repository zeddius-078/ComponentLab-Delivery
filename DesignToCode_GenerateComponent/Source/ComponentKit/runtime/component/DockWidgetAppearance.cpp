#include "DockWidgetAppearance.h"
#include <QAbstractButton>
#include <QDockWidget>
#include <QEvent>
#include <QFontDatabase>
#include <QLayout>
#include <QMainWindow>
#include <QPainter>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOptionDockWidget>
#include <QSvgRenderer>
#include <QTimer>
#include <QVariant>
#include <stdexcept>

namespace commonWidget {
namespace {
QFont titleFont() {
    QFont f("Inter");
    f.setPixelSize(14);
    f.setWeight(QFont::Medium);
    return f;
}

bool capturedTitle(const QDockWidget* d) {
    return !d->isFloating() && !d->titleBarWidget() &&
           !d->features().testFlag(QDockWidget::DockWidgetVerticalTitleBar);
}

class DockStyle final : public QProxyStyle {
public:
    explicit DockStyle(QDockWidget* d) : QProxyStyle(QStyleFactory::create("Fusion")), dock_(d) {
        setParent(d);
        setObjectName("figmaDockStyle");
    }

    int pixelMetric(PixelMetric m, const QStyleOption* o, const QWidget* w) const override {
        if (capturedTitle(dock_)) {
            if (m == PM_DockWidgetTitleMargin)
                return qMax(0, (38 - dock_->fontMetrics().height()) / 2);
            if (m == PM_SmallIconSize)
                return 16;
            if (m == PM_DockWidgetTitleBarButtonMargin)
                return 0;
        }
        return QProxyStyle::pixelMetric(m, o, w);
    }

    QRect subElementRect(SubElement e, const QStyleOption* o, const QWidget* w) const override {
        if (capturedTitle(dock_) &&
            (e == SE_DockWidgetCloseButton || e == SE_DockWidgetFloatButton)) {
            const QRect r(dock_->width() - (e == SE_DockWidgetCloseButton ? 27 : 51), 10, 16, 16);
            return QStyle::visualRect(dock_->layoutDirection(), dock_->rect(), r);
        }
        return QProxyStyle::subElementRect(e, o, w);
    }

    QIcon standardIcon(StandardPixmap p, const QStyleOption* o, const QWidget* w) const override {
        if (p == SP_TitleBarCloseButton)
            return QIcon(":/figma/dockwidget/Close.svg");
        if (p == SP_TitleBarNormalButton)
            return QIcon(":/figma/dockwidget/Undock.svg");
        return QProxyStyle::standardIcon(p, o, w);
    }

private:
    QDockWidget* dock_;
};

class Binding final : public QObject {
public:
    explicit Binding(QDockWidget* dock) : QObject(dock), dock_(dock) {
        setObjectName("figmaDockBinding");
        dock->installEventFilter(this);
        for (auto* b : dock->findChildren<QAbstractButton*>(QString(), Qt::FindDirectChildrenOnly))
            if (b->inherits("QDockWidgetTitleButton"))
                b->installEventFilter(this);
        QObject::connect(dock, &QDockWidget::dockLocationChanged, this, [this] {
            dock_->update();
        });
        QObject::connect(dock, &QDockWidget::topLevelChanged, this, [this] {
            dock_->update();
        });
    }

    bool eventFilter(QObject* watched, QEvent* e) override {
        if (e->type() != QEvent::Paint || !capturedTitle(dock_))
            return false;
        if (auto* b = qobject_cast<QAbstractButton*>(watched)) {
            const bool close = b->objectName() == "qt_dockwidget_closebutton";
            if (!dock_->property(close ? "figmaDockShowCloseGlyph" : "figmaDockShowFloatGlyph")
                     .toBool())
                return true;
            QPainter p(b);
            QStyleOption state;
            state.initFrom(b);
            state.state |= QStyle::State_AutoRaise;
            if (b->isDown()) state.state |= QStyle::State_Sunken;
            else if (b->isEnabled() && b->underMouse()) state.state |= QStyle::State_Raised;
            if (b->style()->styleHint(QStyle::SH_DockWidget_ButtonsHaveFrame, nullptr, b))
                b->style()->drawPrimitive(QStyle::PE_PanelButtonTool, &state, &p, b);
            QSvgRenderer icon(QString(":/figma/dockwidget/%1.svg").arg(close ? "Close" : "Undock"));
            if (!b->isEnabled())
                p.setOpacity(.5); // Runtime fallback, not an authored Disabled variant.
            icon.render(&p, QRectF((b->width() - 16) / 2.0, (b->height() - 16) / 2.0, 16, 16));
            b->setProperty("figmaDockGlyphBounds",
                           QRect((b->width() - 16) / 2, (b->height() - 16) / 2, 16, 16));
            return true;
        }
        if (watched != dock_)
            return false;
        const auto placement = dock_->property("figmaDockPlacementPreview").toString();
        const bool floating = placement == "Floating";
        const int border = dock_->property("figmaDockSourceBorderWidth").isValid()
                               ? dock_->property("figmaDockSourceBorderWidth").toInt()
                               : (floating ? 2 : 1);
        auto area = dock_->property("figmaDockAreaPreview").toString();
        if (area.isEmpty())
            if (auto* main = qobject_cast<QMainWindow*>(dock_->parentWidget())) {
                const auto a = main->dockWidgetArea(dock_);
                area = a == Qt::RightDockWidgetArea    ? "Right"
                       : a == Qt::TopDockWidgetArea    ? "Top"
                       : a == Qt::BottomDockWidgetArea ? "Bottom"
                       : a == Qt::LeftDockWidgetArea   ? "Left"
                                                       : "";
            }
        QPainter p(dock_);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor("#d0d0d0"), border));
        p.setBrush(Qt::white);
        p.drawRoundedRect(QRectF(dock_->rect())
                              .adjusted(border / 2.0, border / 2.0, -border / 2.0, -border / 2.0),
                          floating ? 10 : 4, floating ? 10 : 4);
        const QRect header(1, 1, qMax(0, dock_->width() - 2), 36);
        if (!dock_->property("figmaDockSourceTitleTransparent").toBool())
            p.fillRect(header, floating ? Qt::white : QColor("#f3f3f3"));
        p.setFont(titleFont());
        p.setPen(QColor("#191919"));
        const auto textRect = QStyle::visualRect(dock_->layoutDirection(), dock_->rect(),
                                                 QRect(14, 10, qMax(0, dock_->width() - 100), 22));
        p.drawText(
            textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
            p.fontMetrics().elidedText(dock_->windowTitle(), Qt::ElideRight, textRect.width()));
        dock_->setProperty("figmaDockTitleBounds", textRect);
        dock_->setProperty("figmaDockPaintFont", titleFont());
        QRect edge;
        if (!floating) {
            if (area == "Left")
                edge = QRect(0, 0, 3, dock_->height());
            if (area == "Right")
                edge = QRect(dock_->width() - 3, 0, 3, dock_->height());
            if (area == "Top")
                edge = QRect(0, 0, dock_->width(), 3);
            if (area == "Bottom")
                edge = QRect(0, dock_->height() - 3, dock_->width(), 3);
            p.fillRect(edge, QColor("#6235f7"));
        }
        dock_->setProperty("figmaDockEdgeBounds", edge);
        return true; // Paint only. Native drag, double-click, close, layout and signals pass through.
    }

private:
    QDockWidget* dock_;
};
}

void applyFigmaDockWidgetAppearance(QDockWidget* dock, const QString& area,
                                    const QString& placement, bool showCloseGlyph,
                                    bool showFloatGlyph) {
    if (!dock || !QStringList{"", "Left", "Right", "Top", "Bottom"}.contains(area) ||
        !QStringList{"", "Docked", "Floating"}.contains(placement))
        throw std::invalid_argument("Native DockWidget and captured Area/Placement required");
    static const int font = QFontDatabase::addApplicationFont(":/figma/fonts/Inter-Medium.otf");
    Q_UNUSED(font);
    dock->setProperty("designSystemStyleOwner", "figma-dockwidget-captured");
    dock->setProperty("figmaDockAreaPreview", area);
    dock->setProperty("figmaDockPlacementPreview", placement);
    dock->setProperty("figmaDockShowCloseGlyph", showCloseGlyph);
    dock->setProperty("figmaDockShowFloatGlyph", showFloatGlyph);
    dock->setFont(titleFont());
    dock->setStyleSheet(
        "QDockWidget[designSystemStyleOwner=\"figma-dockwidget-captured\"]{background:transparent;"
        "border:0;font-family:'Inter';font-size:14px;min-height:0;}");
    if (!dock->findChild<QObject*>("figmaDockStyle", Qt::FindDirectChildrenOnly))
        dock->setStyle(new DockStyle(dock));
    if (!dock->findChild<QObject*>("figmaDockBinding", Qt::FindDirectChildrenOnly))
        new Binding(dock);
    dock->layout()->invalidate();
    dock->update();
    for (auto* b : dock->findChildren<QAbstractButton*>(QString(), Qt::FindDirectChildrenOnly))
        b->update();
}
}
