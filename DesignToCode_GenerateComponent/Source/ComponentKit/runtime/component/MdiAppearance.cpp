#include "MdiAppearance.h"
#include "FocusIndicator.h"
#include "native/Z_MdiSubWindow.h"
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTabBar>
#include <QAbstractButton>
#include <QEvent>
#include <QCloseEvent>
#include <QFontDatabase>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOptionTitleBar>
#include <QStyleOptionTab>
#include <QSvgRenderer>
#include <QTextLayout>
#include <QTimer>
#include <QVariant>
#include <QVBoxLayout>
#include <stdexcept>

namespace commonWidget {
namespace {
class MdiDemoDocument final : public QWidget {
public:
    explicit MdiDemoDocument(QWidget* parent = nullptr) : QWidget(parent) {
        setObjectName(QStringLiteral("mdiDocument"));
        setStyleSheet(QStringLiteral(
            "QWidget#mdiDocument{background:white;border:0;}"));
    }

protected:
    void closeEvent(QCloseEvent* event) override {
        if (property("vetoClose").toBool())
            event->ignore();
        else
            QWidget::closeEvent(event);
    }
};

QFont font(int size, int weight = QFont::Normal) {
    QFont f("HarmonyOS Sans");
    f.setPixelSize(size);
    f.setWeight(weight);
    return f;
}

QLabel* demoLabel(QWidget* parent, const QString& text, int size, int weight,
                  const QString& color) {
    auto* result = new QLabel(text, parent);
    result->setObjectName(QStringLiteral("mdiDemoLabel"));
    result->setTextFormat(Qt::PlainText);
    result->setWordWrap(true);
    result->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    result->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    result->setFont(font(size, weight));
    result->setStyleSheet(
        QStringLiteral("background:transparent;border:0;color:%1;").arg(color));
    return result;
}

void svg(QPainter* p, const QString& name, const QRectF& r) {
    QSvgRenderer asset(":/figma/mdi/" + name + ".svg");
    asset.render(p, r);
}

void wrapped(QPainter* p, const QString& text, const QRectF& r, int lineHeight) {
    QTextLayout layout(text, p->font());
    QTextOption option;
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    layout.setTextOption(option);
    layout.beginLayout();
    qreal y = 0;
    while (true) {
        auto line = layout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(r.width());
        line.setPosition(QPointF(0, y + (lineHeight - line.height()) / 2));
        y += lineHeight;
    }
    layout.endLayout();
    layout.draw(p, r.topLeft());
}

QString state(const QMdiSubWindow* sub, const QStyleOptionTitleBar* opt) {
    if (!sub->isEnabled())
        return "Disabled";
    const auto fixed = sub->property("figmaMdiSnapshotState").toString();
    if (!fixed.isEmpty())
        return fixed;
    if (sub->isShaded())
        return "Shaded";
    if (sub->isMinimized())
        return "Minimized";
    if (sub->isMaximized())
        return "Maximized";
    return opt->state.testFlag(QStyle::State_Active) ? "Active" : "Inactive";
}

bool standardControls(const QMdiSubWindow* sub) {
    return !(sub->windowFlags() & (Qt::WindowShadeButtonHint | Qt::WindowContextHelpButtonHint));
}

// Outline above managed content, without replacing it or intercepting input.
class FrameOutline final : public QWidget {
public:
    explicit FrameOutline(QWidget* owner) : QWidget(owner), owner_(owner) {
        setObjectName("figmaMdiFrameOutline");
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        owner->installEventFilter(this);
        sync();
    }

    void sync() {
        setGeometry(owner_->rect());
        raise();
        show();
        update();
    }

protected:
    bool eventFilter(QObject*, QEvent* e) override {
        if (e->type() == QEvent::Resize || e->type() == QEvent::Show ||
            e->type() == QEvent::WindowStateChange || e->type() == QEvent::ActivationChange ||
            e->type() == QEvent::DynamicPropertyChange)
            sync();
        if (e->type() == QEvent::ChildAdded)
            QTimer::singleShot(0, this, [this] {
                sync();
            });
        return false;
    }

    void paintEvent(QPaintEvent*) override {
        auto* sub = qobject_cast<QMdiSubWindow*>(owner_);
        bool active = false;
        if (sub) {
            if (!standardControls(sub))
                return;
            if (sub->mdiArea() && sub->mdiArea()->viewMode() == QMdiArea::TabbedView)
                return;
            if (sub->windowFlags().testFlag(Qt::FramelessWindowHint))
                return;
            const auto fixed = sub->property("figmaMdiSnapshotState").toString();
            active = sub->isEnabled() &&
                     (fixed == "Active" || (fixed.isEmpty() && sub->mdiArea() &&
                                            sub->mdiArea()->activeSubWindow() == sub));
        }
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const int b = active ? 2 : 1;
        p.setPen(QPen(QColor(active ? "#6235f7" : "#d0d0d0"), b));
        p.setBrush(Qt::NoBrush);
        const qreal radius = (sub ? 8 : 10) - b / 2.0;
        p.drawRoundedRect(QRectF(rect()).adjusted(b / 2.0, b / 2.0, -b / 2.0, -b / 2.0), radius,
                          radius);
    }

private:
    QWidget* owner_;
};

class SubStyle final : public QProxyStyle {
public:
    explicit SubStyle(QMdiSubWindow* sub)
        : QProxyStyle(QStyleFactory::create("Fusion")), sub_(sub) {
        setParent(sub);
        setObjectName("figmaMdiSubStyle");
    }

    int pixelMetric(PixelMetric m, const QStyleOption* o, const QWidget* w) const override {
        if (!standardControls(sub_))
            return QProxyStyle::pixelMetric(m, o, w);
        if (m == PM_TitleBarHeight)
            return 34;
        if (m == PM_MdiSubWindowFrameWidth)
            return 1;
        if (m == PM_MdiSubWindowMinimizedWidth)
            return 280;
        return QProxyStyle::pixelMetric(m, o, w);
    }

    int styleHint(StyleHint h, const QStyleOption* o, const QWidget* w,
                  QStyleHintReturn* r) const override {
        if (!standardControls(sub_))
            return QProxyStyle::styleHint(h, o, w, r);
        if (h == SH_TitleBar_NoBorder)
            return 1;
        return QProxyStyle::styleHint(h, o, w, r);
    }

    QRect subControlRect(ComplexControl c, const QStyleOptionComplex* o, SubControl s,
                         const QWidget* w) const override {
        const auto* t = qstyleoption_cast<const QStyleOptionTitleBar*>(o);
        if (c != CC_TitleBar || !t || !standardControls(sub_))
            return QProxyStyle::subControlRect(c, o, s, w);
        const auto flags = t->titleBarFlags;
        const bool minimized = t->titleBarState & Qt::WindowMinimized,
                   maximized = t->titleBarState & Qt::WindowMaximized;
        const int width = sub_->width();
        QRect rect;
        if (s == SC_TitleBarLabel)
            rect = QRect(32, 9, qMax(0, width - 114), 22);
        else if (s == SC_TitleBarSysMenu && flags.testFlag(Qt::WindowSystemMenuHint))
            rect = QRect(10, 10, 14, 14);
        else if (s == SC_TitleBarCloseButton && flags.testFlag(Qt::WindowCloseButtonHint))
            rect = QRect(width - 26, 6, 22, 22);
        else if (s == SC_TitleBarMinButton && flags.testFlag(Qt::WindowMinimizeButtonHint) &&
                 !minimized)
            rect = QRect(width - 78, 6, 22, 22);
        else if (s == SC_TitleBarMaxButton && flags.testFlag(Qt::WindowMaximizeButtonHint) &&
                 !maximized)
            rect = QRect(width - 52, 6, 22, 22);
        else if (s == SC_TitleBarNormalButton &&
                 ((minimized && flags.testFlag(Qt::WindowMinimizeButtonHint)) ||
                  (maximized && flags.testFlag(Qt::WindowMaximizeButtonHint))))
            rect = QRect(width - (minimized ? 78 : 52), 6, 22, 22);
        // Native optional controls have no authored specimen; retain Qt's placement.
        else if (s == SC_TitleBarShadeButton || s == SC_TitleBarUnshadeButton ||
                 s == SC_TitleBarContextHelpButton)
            return QProxyStyle::subControlRect(c, o, s, w);
        return visualRect(sub_->layoutDirection(), sub_->rect(), rect);
    }

    SubControl hitTestComplexControl(ComplexControl c, const QStyleOptionComplex* o,
                                     const QPoint& pt, const QWidget* w) const override {
        if (c != CC_TitleBar || !standardControls(sub_))
            return QProxyStyle::hitTestComplexControl(c, o, pt, w);
        for (auto s : {SC_TitleBarCloseButton, SC_TitleBarNormalButton, SC_TitleBarMinButton,
                       SC_TitleBarMaxButton, SC_TitleBarSysMenu, SC_TitleBarShadeButton,
                       SC_TitleBarUnshadeButton, SC_TitleBarContextHelpButton, SC_TitleBarLabel})
            if (subControlRect(c, o, s, w).contains(pt))
                return s;
        return SC_None;
    }

    void drawPrimitive(PrimitiveElement e, const QStyleOption* o, QPainter* p,
                       const QWidget* w) const override {
        if (e == PE_FrameWindow && standardControls(sub_))
            return; // Draw the whole captured frame with CC_TitleBar.
        QProxyStyle::drawPrimitive(e, o, p, w);
    }

    void drawComplexControl(ComplexControl c, const QStyleOptionComplex* o, QPainter* p,
                            const QWidget* w) const override {
        const auto* t = qstyleoption_cast<const QStyleOptionTitleBar*>(o);
        if (c != CC_TitleBar || !t || !standardControls(sub_)) {
            QProxyStyle::drawComplexControl(c, o, p, w);
            return;
        }
        const QString visual = state(sub_, t);
        const bool disabled = visual == "Disabled";
        const bool active =
            visual == "Active" || (sub_->property("figmaMdiSnapshotState").toString().isEmpty() &&
                                   t->state.testFlag(State_Active));
        const bool source = sub_->property("figmaMdiSourceSnapshot").toBool();
        const auto r = QRectF(sub_->rect());
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        QPainterPath clip;
        clip.addRoundedRect(r, 8, 8);
        p->setClipPath(clip, Qt::IntersectClip);
        p->fillRect(r, disabled ? QColor("#8a8a8a") : Qt::white);
        p->setPen(Qt::NoPen);
        p->setBrush(active ? QColor("#f3f3f3") : Qt::white);
        p->drawRoundedRect(QRectF(0, 0, sub_->width(), 34), 7, 7);
        p->save();
        p->setClipRect(QRect(0, 0, sub_->width(), 34), Qt::IntersectClip);
        p->setBrush(QColor(disabled ? "#8a8a8a" : "#6235f7"));
        auto icon = visualRect(sub_->layoutDirection(), sub_->rect(), QRect(10, 10, 14, 14));
        if (source || sub_->windowIcon().isNull())
            p->drawRoundedRect(icon, 3, 3);
        else
            sub_->windowIcon().paint(p, icon);
        p->setFont(font(14, QFont::Medium));
        p->setPen(QColor(disabled ? "#8a8a8a" : "#191919"));
        QRect textRect = subControlRect(c, t, SC_TitleBarLabel, w);
        if (source)
            wrapped(p, sub_->windowTitle(), QRectF(32, 9, visual == "Active" ? 78 : 76, 44), 22);
        else
            p->drawText(
                textRect,
                Qt::AlignVCenter | Qt::TextSingleLine |
                    (sub_->isRightToLeft() ? Qt::AlignRight : Qt::AlignLeft),
                p->fontMetrics().elidedText(sub_->windowTitle(), Qt::ElideRight, textRect.width()));
        const QList<SubControl> controls{SC_TitleBarMinButton, SC_TitleBarMaxButton,
                                         SC_TitleBarCloseButton};
        for (int i = 0; i < 3; ++i) {
            const char* property = i == 0   ? "figmaMdiMinimizeGlyph"
                                   : i == 1 ? "figmaMdiMaximizeGlyph"
                                            : "figmaMdiCloseGlyph";
            if (!sub_->property(property).toBool())
                continue;
            auto control = controls[i];
            if ((i == 0 && sub_->isMinimized()) || (i == 1 && sub_->isMaximized()))
                control = SC_TitleBarNormalButton;
            QRect slot = subControlRect(c, t, control, w);
            if (slot.isEmpty())
                continue;
            p->setPen(Qt::NoPen);
            p->setBrush(Qt::white);
            p->drawRoundedRect(slot, 5, 5);
            if (!source && !disabled && t->activeSubControls.testFlag(control)) {
                p->setBrush(QColor(t->state.testFlag(State_Sunken) ? "#d0d0d0" : "#f3f3f3"));
                p->drawRoundedRect(slot, 5, 5);
            }
            QString asset = i == 0   ? "Minus"
                            : i == 1 ? (visual == "Maximized" ? "Restore" : "Maximize")
                                     : "Close";
            if (!source && control == SC_TitleBarNormalButton)
                asset = "Restore";
            if (disabled && asset != "Restore")
                asset += "Disabled";
            if (disabled && asset == "Restore")
                p->setOpacity(.5); // No authored disabled Restore.
            const QRect glyph = slot.adjusted(3, 3, -3, -3);
            svg(p, asset, glyph);
            p->setOpacity(1);
            sub_->setProperty(i == 0   ? "figmaMdiMinusBounds"
                              : i == 1 ? "figmaMdiMaximizeBounds"
                                       : "figmaMdiCloseBounds",
                              glyph);
        }
        p->restore();
        const int border = active ? 2 : 1;
        p->setPen(QPen(QColor(active ? "#6235f7" : "#d0d0d0"), border));
        p->setBrush(Qt::NoBrush);
        p->drawRoundedRect(r.adjusted(border / 2.0, border / 2.0, -border / 2.0, -border / 2.0),
                           8 - border / 2.0, 8 - border / 2.0);
        p->restore();
        sub_->setProperty("figmaMdiTitleBounds", QRect(0, 0, sub_->width(), 34));
    }

private:
    QMdiSubWindow* sub_;
};

class AreaTabStyle final : public QProxyStyle {
public:
    explicit AreaTabStyle(QTabBar* bar) : QProxyStyle(QStyleFactory::create("Fusion")) {
        setParent(bar);
        setObjectName("figmaMdiTabStyle");
    }

    int pixelMetric(PixelMetric m, const QStyleOption* o, const QWidget* w) const override {
        if (m == PM_TabCloseIndicatorWidth || m == PM_TabCloseIndicatorHeight)
            return 16;
        if (m == PM_TabBarTabOverlap || m == PM_TabBarBaseOverlap)
            return 0;
        return QProxyStyle::pixelMetric(m, o, w);
    }

    QSize sizeFromContents(ContentsType t, const QStyleOption* o, const QSize& s,
                           const QWidget* w) const override {
        if (t == CT_TabBarTab)
            return QSize(120, 36);
        return QProxyStyle::sizeFromContents(t, o, s, w);
    }

    QRect subElementRect(SubElement e, const QStyleOption* o, const QWidget* w) const override {
        if (e == SE_TabBarTabRightButton || e == SE_TabBarTabLeftButton)
            return visualRect(o->direction, o->rect,
                              QRect(o->rect.x() + 92, o->rect.y() + 10, 16, 16));
        return QProxyStyle::subElementRect(e, o, w);
    }

    void drawPrimitive(PrimitiveElement e, const QStyleOption* o, QPainter* p,
                       const QWidget* w) const override {
        if (e == PE_IndicatorTabClose) {
            svg(p, "Close", o->rect);
            return;
        }
        if (e == PE_FrameTabBarBase)
            return;
        QProxyStyle::drawPrimitive(e, o, p, w);
    }

    void drawControl(ControlElement e, const QStyleOption* o, QPainter* p,
                     const QWidget* w) const override {
        const auto* t = qstyleoption_cast<const QStyleOptionTab*>(o);
        const auto* bar = qobject_cast<const QTabBar*>(w);
        if (!bar || !t || (e != CE_TabBarTab && e != CE_TabBarTabShape && e != CE_TabBarTabLabel)) {
            QProxyStyle::drawControl(e, o, p, w);
            return;
        }
        const bool selected = t->state.testFlag(State_Selected),
                   source = bar->property("figmaMdiSourceSnapshot").toBool();
        const QRect r = t->rect.adjusted(0, 1, -2, -1);
        p->save();
        p->setClipRect(r);
        p->setRenderHint(QPainter::Antialiasing);
        if (e != CE_TabBarTabLabel) {
            p->setPen(Qt::NoPen);
            p->setBrush(selected ? QColor("#f3f3f3") : Qt::white);
            p->drawRoundedRect(r, 5, 5);
        }
        if (e != CE_TabBarTabShape) {
            p->setFont(font(12, selected ? QFont::DemiBold : QFont::Normal));
            p->setPen(QColor(selected ? "#6235f7" : "#191919"));
            if (source)
                wrapped(p, t->text,
                        QRectF(r.x() + 12, r.y() + 9,
                               t->text == "Trend"    ? 31
                               : t->text == "Recipe" ? 36
                                                     : 30,
                               36),
                        18);
            else {
                auto tr = visualRect(t->direction, r, r.adjusted(12, 0, -32, 0));
                p->drawText(tr, Qt::AlignVCenter | Qt::TextSingleLine,
                            p->fontMetrics().elidedText(t->text, Qt::ElideRight, tr.width()));
            }
        }
        if (!source && t->state.testFlag(State_HasFocus) &&
            FocusIndicator::instance().visibleFor(bar)) {
            p->setPen(QPen(QColor("#6235f7"), 2));
            p->setBrush(Qt::NoBrush);
            p->drawRoundedRect(r.adjusted(1, 1, -1, -1), 4, 4);
        }
        p->restore();
    }
};

class AreaBinding final : public QObject {
public:
    explicit AreaBinding(QMdiArea* area) : QObject(area), area_(area) {
        setObjectName("figmaMdiAreaBinding");
        area->installEventFilter(this);
    }

    void refresh() {
        area_->setBackground(Qt::white);
        if (auto* bar = area_->findChild<QTabBar*>(QString(), Qt::FindDirectChildrenOnly)) {
            bar->setProperty("figmaMdiSourceSnapshot", area_->property("figmaMdiSourceSnapshot"));
            if (!bar->findChild<QObject*>("figmaMdiTabStyle", Qt::FindDirectChildrenOnly)) {
                bar->setStyle(new AreaTabStyle(bar));
                bar->setExpanding(false);
                bar->setDrawBase(false);
                bar->setFont(font(12));
                bar->setStyleSheet(
                    "QTabBar{background:white;border:1px solid #d0d0d0;border-radius:6px;}");
            }
            for (auto* b :
                 bar->findChildren<QAbstractButton*>(QString(), Qt::FindDirectChildrenOnly))
                if (b->inherits("CloseButton")) {
                    b->setStyle(bar->style());
                    b->setFixedSize(16, 16);
                }
        }
    }

    bool eventFilter(QObject*, QEvent* e) override {
        if (!queued_ && (e->type() == QEvent::ChildAdded || e->type() == QEvent::Show ||
                         e->type() == QEvent::LayoutRequest)) {
            queued_ = true;
            QTimer::singleShot(0, this, [this] {
                queued_ = false;
                refresh();
            });
        }
        return false;
    }

private:
    QMdiArea* area_;
    bool queued_ = false;
};
}

void populateFigmaMdiSubWindow(QMdiSubWindow* sub, const QString& title) {
    if (!sub)
        throw std::invalid_argument("MdiSubWindow is null");
    if (auto* previous = sub->widget()) {
        sub->setWidget(nullptr);
        delete previous;
    }
    sub->setWindowTitle(title);
    auto* page = new MdiDemoDocument;
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(16, 20, 16, 20);
    outer->setSpacing(16);
    auto* heading = demoLabel(page, QStringLiteral("Document content slot"), 12,
                              QFont::Normal, QStringLiteral("#191919"));
    auto* card = new QWidget(page);
    card->setObjectName(QStringLiteral("mdiDataPreview"));
    card->setStyleSheet(QStringLiteral(
        "QWidget#mdiDataPreview{background:white;border:1px solid #d0d0d0;border-radius:8px;}"));
    auto* inner = new QVBoxLayout(card);
    inner->setContentsMargins(12, 12, 12, 12);
    auto* detail = demoLabel(card, QStringLiteral("Pressure trend · 1.20 Pa"), 12,
                             QFont::Normal, QStringLiteral("#191919"));
    inner->addWidget(detail);
    outer->addWidget(heading);
    outer->addWidget(card);
    outer->addStretch();
    sub->setWidget(page);
}

void configureFigmaMdiSubWindow(QMdiSubWindow* sub,
                                const QMap<QString, QString>& properties) {
    if (!sub)
        throw std::invalid_argument("MdiSubWindow is null");
    const auto snapshot =
        properties.value(QStringLiteral("State"), QStringLiteral("Inactive"));
    if (!QStringList{QStringLiteral("Inactive"), QStringLiteral("Active"),
                     QStringLiteral("Maximized"), QStringLiteral("Minimized"),
                     QStringLiteral("Shaded"), QStringLiteral("Disabled")}
             .contains(snapshot))
        throw std::invalid_argument("Invalid MdiSubWindow State");
    const auto title = properties.value(QStringLiteral("Title"), QStringLiteral("Trend"));
    populateFigmaMdiSubWindow(sub, title);
    applyFigmaMdiSubWindowAppearance(sub, snapshot);
}

void applyFigmaMdiSubWindowAppearance(QMdiSubWindow* sub, const QString& snapshot, bool closeGlyph,
                                      bool minimizeGlyph, bool maximizeGlyph) {
    if (!sub ||
        !QStringList{"", "Inactive", "Active", "Maximized", "Minimized", "Shaded", "Disabled"}
             .contains(snapshot))
        throw std::invalid_argument("Native QMdiSubWindow and captured State required");
    static const int f = QFontDatabase::addApplicationFont(":/fonts/HarmonyOS_Sans_Regular.ttf");
    Q_UNUSED(f);
    sub->setProperty("designSystemStyleOwner", "figma-mdi-captured");
    sub->setProperty("figmaMdiSnapshotState", snapshot);
    sub->setProperty("figmaMdiCloseGlyph", closeGlyph);
    sub->setProperty("figmaMdiMinimizeGlyph", minimizeGlyph);
    sub->setProperty("figmaMdiMaximizeGlyph", maximizeGlyph);
    sub->setFont(font(14, QFont::Medium));
    if (!sub->findChild<QObject*>("figmaMdiSubStyle", Qt::FindDirectChildrenOnly))
        sub->setStyle(new SubStyle(sub));
    sub->updateGeometry();
    sub->update();
    if (!sub->findChild<QWidget*>("figmaMdiFrameOutline", Qt::FindDirectChildrenOnly))
        new FrameOutline(sub);
}

void configureFigmaMdiArea(QMdiArea* area, const QMap<QString, QString>& properties) {
    if (!area)
        throw std::invalid_argument("MdiArea is null");
    const auto presentation =
        properties.value(QStringLiteral("Presentation"), QStringLiteral("Free"));
    if (!QStringList{QStringLiteral("Free"), QStringLiteral("Cascade"),
                     QStringLiteral("Tile"), QStringLiteral("Tabbed")}
             .contains(presentation))
        throw std::invalid_argument("Invalid MdiArea Presentation");

    for (auto* previous : area->subWindowList()) {
        area->removeSubWindow(previous);
        delete previous;
    }
    area->setProperty("figmaMdiPresentationPreview", presentation);
    applyFigmaMdiAreaAppearance(area);
    const QList<QRect> positions =
        presentation == QStringLiteral("Free")
            ? QList<QRect>{{36, 70, 280, 170}, {292, 158, 280, 170}}
        : presentation == QStringLiteral("Cascade")
            ? QList<QRect>{{18, 52, 280, 170}, {104, 92, 280, 170}, {190, 132, 280, 170}}
            : QList<QRect>{{12, 42, 280, 170}, {312, 42, 280, 170},
                           {12, 188, 280, 170}, {312, 188, 280, 170}};
    const bool tabbed = presentation == QStringLiteral("Tabbed");
    const int count = tabbed ? 3 : positions.size();
    const QStringList titles{QStringLiteral("Trend"), QStringLiteral("Recipe"),
                             QStringLiteral("Alarm"), QStringLiteral("Report")};
    QList<QMdiSubWindow*> documents;
    for (int i = 0; i < count; ++i) {
        auto* sub = new Z_MdiSubWindow;
        sub->setObjectName(QStringLiteral("mdiDemoDocument%1").arg(i));
        sub->setAttribute(Qt::WA_DeleteOnClose, false);
        populateFigmaMdiSubWindow(sub, titles[i]);
        applyFigmaMdiSubWindowAppearance(sub);
        area->addSubWindow(sub);
        if (!tabbed)
            sub->setGeometry(positions[i]);
        sub->show();
        documents.append(sub);
    }
    area->setViewMode(tabbed ? QMdiArea::TabbedView : QMdiArea::SubWindowView);
    if (tabbed)
        area->setTabsClosable(true);
    if (!documents.isEmpty())
        area->setActiveSubWindow(
            presentation == QStringLiteral("Cascade") ? documents.last() : documents.first());
    applyFigmaMdiAreaAppearance(area);
}

void applyFigmaMdiAreaAppearance(QMdiArea* area) {
    if (!area)
        throw std::invalid_argument("Native QMdiArea required");
    QFontDatabase::addApplicationFont(
        QStringLiteral(":/fonts/HarmonyOS_Sans_Regular.ttf"));
    area->setProperty("designSystemStyleOwner", "figma-mdi-captured");
    area->setStyleSheet("QMdiArea[designSystemStyleOwner=\"figma-mdi-captured\"]{background:white;"
                        "border:1px solid #d0d0d0;border-radius:10px;}");
    auto* binding = area->findChild<QObject*>("figmaMdiAreaBinding", Qt::FindDirectChildrenOnly);
    if (!binding)
        binding = new AreaBinding(area);
    static_cast<AreaBinding*>(binding)->refresh();
    if (!area->findChild<QWidget*>("figmaMdiFrameOutline", Qt::FindDirectChildrenOnly))
        new FrameOutline(area);
}
}
