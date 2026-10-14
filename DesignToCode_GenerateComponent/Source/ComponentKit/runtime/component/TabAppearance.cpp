#include "TabAppearance.h"
#include "FocusIndicator.h"
#include <QCoreApplication>
#include <QEvent>
#include <QFontDatabase>
#include <QLabel>
#include <QPainter>
#include <QProxyStyle>
#include <QSizePolicy>
#include <QStyleFactory>
#include <QStyleOptionTab>
#include <QStyleOptionTabWidgetFrame>
#include <QSvgRenderer>
#include <QTabBar>
#include <QTabWidget>
#include <QToolButton>
#include <QTimer>
#include <QVariant>
#include <QVBoxLayout>
#include <stdexcept>

namespace commonWidget {
namespace {
QLabel* compositionLabel(QWidget* parent, const QString& text, const QRect& rect, int size,
                         int weight, const QString& color) {
    auto* result = new QLabel(text, parent);
    result->setTextFormat(Qt::PlainText);
    result->setGeometry(rect);
    result->setWordWrap(true);
    result->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    QFont labelFont("Inter");
    labelFont.setPixelSize(size);
    labelFont.setWeight(weight);
    result->setFont(labelFont);
    result->setStyleSheet(
        QString("background:transparent;border:0;color:%1;font-family:'Inter';font-size:%2px;")
            .arg(color)
            .arg(size));
    return result;
}

QFont font() {
    QFont f("Inter");
    f.setPixelSize(14);
    f.setWeight(QFont::DemiBold);
    return f;
}

void setFigmaTabClosePolicyImpl(QTabWidget* tabs, bool closable, bool behaviorOnly) {
    if (!tabs)
        throw std::invalid_argument("TabWidget is null");
    tabs->setTabsClosable(false);
    tabs->setTabsClosable(closable);
    if (!closable || !behaviorOnly)
        return;
    for (int i = 0; i < tabs->count(); ++i) {
        if (tabs->widget(i)->objectName() == QStringLiteral("tabDemoBehavior"))
            continue;
        for (auto side : {QTabBar::LeftSide, QTabBar::RightSide})
            if (auto* button = tabs->tabBar()->tabButton(i, side)) {
                tabs->tabBar()->setTabButton(i, side, nullptr);
                button->deleteLater();
            }
    }
}

void populateFigmaTabWidgetImpl(QTabWidget* tabs, bool sourceSnapshot) {
    if (!tabs)
        throw std::invalid_argument("TabWidget is null");
    while (tabs->count()) {
        auto* page = tabs->widget(0);
        tabs->removeTab(0);
        delete page;
    }
    for (const auto& name :
         {QStringLiteral("Overview"), QStringLiteral("Appearance"),
          QStringLiteral("Behavior"), QStringLiteral("Advanced")}) {
        auto* page = new QWidget;
        page->setObjectName(QStringLiteral("tabDemo") + name);
        auto* title = compositionLabel(page, name + QStringLiteral(" page"),
                                       QRect(23, 27, 700, 26), 18, QFont::DemiBold,
                                       QStringLiteral("#191919"));
        auto* description = compositionLabel(
            page,
            QStringLiteral("Only the current page is visible. Selection, focus, close requests "
                           "and page ownership remain distinct contracts."),
            QRect(23, 63, 700, 19), 13, QFont::Normal, QStringLiteral("#191919"));
        auto* note = compositionLabel(
            page, QStringLiteral("removeTab() detaches the page; it does not delete it."),
            QRect(23, 127, 700, 18), 12, QFont::Normal, QStringLiteral("#8a8a8a"));
        if (!sourceSnapshot) {
            auto* layout = new QVBoxLayout(page);
            layout->setContentsMargins(23, 27, 23, 20);
            layout->setSpacing(10);
            for (auto* text : {title, description, note}) {
                text->setMinimumWidth(0);
                text->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
                layout->addWidget(text);
            }
            layout->addStretch();
        }
        tabs->addTab(page, name);
    }
    tabs->setTabIcon(0, figmaTabDocumentIcon(true));
    tabs->setTabIcon(1, figmaTabDocumentIcon(true));
    tabs->setIconSize(QSize(16, 16));
    tabs->setTabEnabled(3, false);
    setFigmaTabClosePolicyImpl(tabs, true, true);
    tabs->setCurrentIndex(0);
    applyFigmaTabWidgetAppearance(tabs, {}, -1, sourceSnapshot);
}

bool vertical(QTabBar::Shape shape) {
    return shape == QTabBar::RoundedWest || shape == QTabBar::RoundedEast ||
           shape == QTabBar::TriangularWest || shape == QTabBar::TriangularEast;
}

void svg(QPainter* p, const QString& name, const QRectF& rect) {
    QSvgRenderer asset(":/figma/tabs/" + name + ".svg");
    asset.render(p, rect);
}

QString state(const QTabBar* bar, const QStyleOptionTab* opt) {
    const auto* v4 = qstyleoption_cast<const QStyleOptionTabV4*>(opt);
    const int index = v4 ? v4->tabIndex : bar->tabAt(opt->rect.center());
    const bool enabled = opt->state.testFlag(QStyle::State_Enabled);
    if (!enabled)
        return "Disabled";
    const int target = bar->property("figmaTabPreviewIndex").toInt();
    const auto fixed = bar->property("figmaTabSnapshotState").toString();
    if (!fixed.isEmpty() && (target < 0 || target == index))
        return fixed;
    if (opt->state.testFlag(QStyle::State_Selected))
        return "Selected";
    if (opt->state.testFlag(QStyle::State_MouseOver))
        return "Hover";
    return "Default";
}

class TabStyle final : public QProxyStyle {
public:
    explicit TabStyle(QWidget* parent) : QProxyStyle(QStyleFactory::create("Fusion")) {
        setParent(parent);
        setObjectName("figmaTabStyle");
    }

    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                    const QWidget* widget = nullptr) const override {
        if (metric == PM_TabCloseIndicatorWidth || metric == PM_TabCloseIndicatorHeight)
            return 16;
        if (qobject_cast<const QTabBar*>(widget)) {
            if (metric == PM_TabBarScrollButtonWidth)
                return 22;
            if (metric == PM_TabCloseIndicatorWidth || metric == PM_TabCloseIndicatorHeight)
                return 16;
            if (metric == PM_TabBarTabOverlap || metric == PM_TabBarBaseOverlap)
                return 0;
        }
        return QProxyStyle::pixelMetric(metric, option, widget);
    }

    QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size,
                           const QWidget* widget) const override {
        if (type == CT_TabBarTab)
            if (const auto* bar = qobject_cast<const QTabBar*>(widget)) {
                const auto* opt = qstyleoption_cast<const QStyleOptionTab*>(option);
                if (bar->property("figmaTabSourceSnapshot").toBool())
                    return QSize(176, 48);
                const int text =
                    opt ? QFontMetrics(font()).size(Qt::TextShowMnemonic, opt->text).width() : 0;
                return QSize(qMax(176, text + 72), 48);
            }
        return QProxyStyle::sizeFromContents(type, option, size, widget);
    }

    QRect subElementRect(SubElement element, const QStyleOption* option,
                         const QWidget* widget) const override {
        if (const auto* tabs = qobject_cast<const QTabWidget*>(widget)) {
            if (element == SE_TabWidgetTabBar || element == SE_TabWidgetTabPane ||
                element == SE_TabWidgetTabContents) {
                const auto r = tabs->rect();
                const bool anatomy = tabs->property("figmaTabAnatomyOnly").toBool();
                if (anatomy)
                    return element == SE_TabWidgetTabBar ? r : QRect();
                const auto* opt = qstyleoption_cast<const QStyleOptionTabWidgetFrame*>(option);
                const auto position = tabs->tabPosition();
                const bool side = position == QTabWidget::West || position == QTabWidget::East;
                const int extent = side ? 176 : 48;
                QRect pane = r.adjusted(16, 16, -16, -20), bar;
                if (position == QTabWidget::North) {
                    bar = QRect(16, 16, qMax(0, r.width() - 32), extent);
                    pane.setTop(64);
                } else if (position == QTabWidget::South) {
                    bar = QRect(16, r.height() - 20 - extent, qMax(0, r.width() - 32), extent);
                    pane.setBottom(bar.top() - 1);
                } else if (position == QTabWidget::West) {
                    bar = QRect(16, 16, extent, qMax(0, r.height() - 36));
                    pane.setLeft(192);
                } else {
                    bar = QRect(r.width() - 16 - extent, 16, extent, qMax(0, r.height() - 36));
                    pane.setRight(bar.left() - 1);
                }
                if (opt) {
                    if (side)
                        bar.setHeight(qMin(bar.height(), opt->tabBarSize.height()));
                    else
                        bar.setWidth(qMin(bar.width(), opt->tabBarSize.width()));
                }
                return element == SE_TabWidgetTabBar        ? bar
                       : element == SE_TabWidgetTabContents ? pane.adjusted(1, 1, -1, -1)
                                                            : pane;
            }
        }
        if (const auto* bar = qobject_cast<const QTabBar*>(widget)) {
            const QRect r = option->rect;
            if (element == SE_TabBarTearIndicatorLeft || element == SE_TabBarTearIndicatorRight)
                return {};
            if (element == SE_TabBarScrollLeftButton || element == SE_TabBarScrollRightButton) {
                const bool first = element == SE_TabBarScrollLeftButton;
                if (vertical(bar->shape()))
                    return QRect(0, bar->height() - (first ? 44 : 22), bar->width(), 22);
                return visualRect(bar->layoutDirection(), bar->rect(),
                                  QRect(bar->width() - (first ? 44 : 22), 0, 22, bar->height()));
            }
            if (element == SE_TabBarTabText)
                return visualRect(option->direction, r,
                                  QRect(r.x() + 36, r.y() + 15, qMax(0, r.width() - 72), 22));
            if (element == SE_TabBarTabRightButton || element == SE_TabBarTabLeftButton) {
                const auto* opt = qstyleoption_cast<const QStyleOptionTab*>(option);
                const bool right = element == SE_TabBarTabRightButton;
                const auto size =
                    opt ? (right ? opt->rightButtonSize : opt->leftButtonSize) : QSize(16, 16);
                return visualRect(option->direction, r,
                                  QRect(right ? r.right() + 1 - 12 - size.width() : r.x() + 16,
                                        r.y() + 15, size.width(), size.height()));
            }
        }
        return QProxyStyle::subElementRect(element, option, widget);
    }

    void drawPrimitive(PrimitiveElement pe, const QStyleOption* option, QPainter* p,
                       const QWidget* widget) const override {
        if (pe == PE_IndicatorTabClose) {
            svg(p, option->state.testFlag(State_Enabled) ? "ClosePrimary" : "CloseDisabled",
                option->rect);
            return;
        }
        if (pe == PE_FrameTabBarBase)
            return;
        if (pe == PE_FrameTabWidget)
            if (const auto* tabs = qobject_cast<const QTabWidget*>(widget)) {
                if (tabs->property("figmaTabAnatomyOnly").toBool())
                    return;
                p->save();
                p->setRenderHint(QPainter::Antialiasing);
                p->setPen(QColor("#d0d0d0"));
                p->setBrush(Qt::white);
                p->drawRoundedRect(QRectF(tabs->rect()).adjusted(.5, .5, -.5, -.5), 9.5, 9.5);
                p->drawRoundedRect(QRectF(option->rect).adjusted(.5, .5, -.5, -.5), 7.5, 7.5);
                p->restore();
                return;
            }
        QProxyStyle::drawPrimitive(pe, option, p, widget);
    }

    void drawControl(ControlElement ce, const QStyleOption* option, QPainter* p,
                     const QWidget* widget) const override {
        const auto* bar = qobject_cast<const QTabBar*>(widget);
        const auto* opt = qstyleoption_cast<const QStyleOptionTab*>(option);
        if (!bar || !opt ||
            (ce != CE_TabBarTab && ce != CE_TabBarTabShape && ce != CE_TabBarTabLabel)) {
            QProxyStyle::drawControl(ce, option, p, widget);
            return;
        }
        const auto visual = state(bar, opt);
        const bool selected = visual == "Selected", disabled = visual == "Disabled";
        const bool fixedFocus = visual == "Focus";
        const auto* v4 = qstyleoption_cast<const QStyleOptionTabV4*>(opt);
        const int index = v4 ? v4->tabIndex : bar->tabAt(opt->rect.center());
        const int target = bar->property("figmaTabPreviewIndex").toInt();
        const bool overridden = !bar->property("figmaTabSnapshotState").toString().isEmpty() &&
                                (target < 0 || target == index);
        const bool nativeFocus = !overridden && opt->state.testFlag(State_HasFocus) &&
                                 FocusIndicator::instance().visibleFor(bar);
        const bool focus = fixedFocus || nativeFocus;
        const auto r = opt->rect;
        p->save();
        p->setClipRect(r);
        p->setRenderHint(QPainter::Antialiasing, false);
        if (ce != CE_TabBarTabLabel) {
            const qreal border = focus ? 2 : 1;
            p->setPen(QPen(QColor(focus ? "#6235f7" : "#d0d0d0"), border));
            p->setBrush(QColor(selected || visual == "Hover" ? "#f3f3f3" : "#ffffff"));
            p->drawRect(QRectF(r).adjusted(border / 2, border / 2, -border / 2, -border / 2));
            if (selected) {
                QRect strip;
                switch (opt->shape) {
                case QTabBar::RoundedSouth:
                case QTabBar::TriangularSouth:
                    strip = QRect(r.x(), r.y(), r.width(), 3);
                    break;
                case QTabBar::RoundedWest:
                case QTabBar::TriangularWest:
                    strip = QRect(r.right() - 2, r.y(), 3, r.height());
                    break;
                case QTabBar::RoundedEast:
                case QTabBar::TriangularEast:
                    strip = QRect(r.x(), r.y(), 3, r.height());
                    break;
                default:
                    strip = QRect(r.x(), r.bottom() - 2, r.width(), 3);
                    break;
                }
                p->fillRect(strip, QColor("#6235f7"));
            }
        }
        if (ce != CE_TabBarTabShape) {
            const auto textRect = subElementRect(SE_TabBarTabText, opt, bar);
            p->setFont(font());
            p->setPen(QColor(disabled ? "#8a8a8a" : selected ? "#6235f7" : "#191919"));
            int flags = Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextShowMnemonic |
                        (opt->direction == Qt::RightToLeft ? Qt::AlignRight : Qt::AlignLeft);
            if (!styleHint(SH_UnderlineShortcut, opt, bar))
                flags |= Qt::TextHideMnemonic;
            p->drawText(textRect, flags, opt->text);
            if (!opt->icon.isNull()) {
                const QRect slot =
                    visualRect(opt->direction, r, QRect(r.x() + 16, r.y() + 15, 16, 16));
                opt->icon.paint(p, slot, Qt::AlignLeft | Qt::AlignTop,
                                disabled ? QIcon::Disabled : QIcon::Normal,
                                selected ? QIcon::On : QIcon::Off);
            }
            const_cast<QTabBar*>(bar)->setProperty("figmaTabPaintFont", font());
        }
        p->restore();
    }
};

class TabBinding final : public QObject {
public:
    explicit TabBinding(QTabBar* bar) : QObject(bar), bar_(bar) {
        setObjectName("figmaTabBinding");
        bar_->installEventFilter(this);
    }

    void refresh() {
        bool resized = false;
        for (int i = 0; i < bar_->count(); ++i)
            for (auto side : {QTabBar::LeftSide, QTabBar::RightSide})
                if (auto* close = bar_->tabButton(i, side)) {
                    // Qt 5.15 CloseButton uses its own style, not the bar's style.
                    // Do not restyle application-supplied tab buttons.
                    if (!close->inherits("CloseButton"))
                        continue;
                    if (!close->findChild<QObject*>("figmaTabStyle", Qt::FindDirectChildrenOnly))
                        close->setStyle(new TabStyle(close));
                    if (close->size() != QSize(16, 16)) {
                        close->resize(16, 16);
                        resized = true;
                    }
                }
        if (resized) {
            QEvent change(QEvent::StyleChange);
            QCoreApplication::sendEvent(bar_, &change);
        }
        for (auto* child :
             bar_->findChildren<QToolButton*>(QString(), Qt::FindDirectChildrenOnly)) {
            if (!child->property("figmaTabScroller").toBool()) {
                child->setProperty("figmaTabScroller", true);
                child->setStyleSheet(
                    "QToolButton "
                    "{min-width:0;min-height:0;border:0;padding:0;background:transparent;}");
                child->installEventFilter(this);
            }
        }
    }

protected:
    bool eventFilter(QObject* object, QEvent* event) override {
        if (object == bar_ &&
            (event->type() == QEvent::ChildAdded || event->type() == QEvent::Show))
            QTimer::singleShot(0, this, [this] {
                refresh();
            });
        if (event->type() == QEvent::Paint && object != bar_)
            if (auto* button = qobject_cast<QToolButton*>(object)) {
                if (button->arrowType() != Qt::LeftArrow && button->arrowType() != Qt::RightArrow)
                    return false;
                QPainter p(button);
                p.fillRect(button->rect(), QColor("#f3f3f3"));
                if (!button->isEnabled() && !bar_->property("figmaTabSourceSnapshot").toBool())
                    p.setOpacity(.4);
                svg(&p, button->arrowType() == Qt::LeftArrow ? "ChevronLeft" : "ChevronRight",
                    QRectF((button->width() - 16) / 2., (button->height() - 16) / 2., 16, 16));
                return true;
            }
        return false;
    }

private:
    QTabBar* bar_;
};
} // namespace

void setFigmaTabClosePolicy(QTabWidget* tabs, bool closable, bool behaviorOnly) {
    setFigmaTabClosePolicyImpl(tabs, closable, behaviorOnly);
}

void populateFigmaTabWidget(QTabWidget* tabs, bool sourceSnapshot) {
    populateFigmaTabWidgetImpl(tabs, sourceSnapshot);
}

QIcon figmaTabDocumentIcon(bool composition) {
    QIcon icon;
    for (auto mode : {QIcon::Normal, QIcon::Active, QIcon::Selected, QIcon::Disabled})
        for (auto state : {QIcon::Off, QIcon::On})
            icon.addFile(":/figma/tabs/Document" +
                             QString(mode == QIcon::Disabled ? "Disabled"
                                     : composition ? (state == QIcon::On ? "CompositionAccent"
                                                                         : "CompositionPrimary")
                                     : state == QIcon::On ? "Accent"
                                                          : "Primary") +
                             ".svg",
                         QSize(16, 16), mode, state);
    return icon;
}

void applyFigmaTabBarAppearance(QTabBar* bar, const QString& snapshotState, int previewIndex,
                                bool sourceSnapshot) {
    if (!bar || !QStringList{"", "Default", "Hover", "Selected", "Focus", "Disabled"}.contains(
                    snapshotState))
        throw std::invalid_argument("Native tab bar and captured state required");
    static const bool fonts = [] {
        QFontDatabase::addApplicationFont(":/figma/fonts/Inter-SemiBold.otf");
        return true;
    }();
    Q_UNUSED(fonts);
    bar->setProperty("figmaTabSnapshotState", snapshotState);
    bar->setProperty("figmaTabPreviewIndex", previewIndex);
    bar->setProperty("figmaTabSourceSnapshot", sourceSnapshot);
    bar->setFont(font());
    bar->setAttribute(Qt::WA_Hover);
    if (!bar->findChild<QObject*>("figmaTabStyle", Qt::FindDirectChildrenOnly))
        bar->setStyle(new TabStyle(bar));
    auto* binding = static_cast<TabBinding*>(
        bar->findChild<QObject*>("figmaTabBinding", Qt::FindDirectChildrenOnly));
    if (!binding)
        binding = new TabBinding(bar);
    binding->refresh();
    bar->ensurePolished();
    QEvent change(QEvent::StyleChange);
    QCoreApplication::sendEvent(bar, &change);
    bar->updateGeometry();
    bar->update();
}

void applyFigmaTabWidgetAppearance(QTabWidget* tabs, const QString& snapshotState, int previewIndex,
                                   bool sourceSnapshot, bool anatomyOnly) {
    if (!tabs)
        throw std::invalid_argument("Native tab widget required");
    tabs->setProperty("designSystemStyleOwner", "figma-tabs-captured");
    tabs->setProperty("figmaTabAnatomyOnly", anatomyOnly);
    tabs->setProperty("figmaTabSourceSnapshot", sourceSnapshot);
    if (!tabs->findChild<QObject*>("figmaTabStyle", Qt::FindDirectChildrenOnly))
        tabs->setStyle(new TabStyle(tabs));
    applyFigmaTabBarAppearance(tabs->tabBar(), snapshotState, previewIndex, sourceSnapshot);
    tabs->ensurePolished();
    QEvent change(QEvent::StyleChange);
    QCoreApplication::sendEvent(tabs, &change);
    tabs->updateGeometry();
    tabs->update();
}
} // namespace commonWidget
