#include "ToolButtonAppearance.h"
#include <QAction>
#include <QCoreApplication>
#include <QEvent>
#include <QFontDatabase>
#include <QPainter>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOptionToolButton>
#include <QToolButton>
#include <QVariant>
#include <stdexcept>

namespace commonWidget {
namespace {
QFont labelFont() {
    QFont f("Inter");
    f.setPixelSize(14);
    f.setWeight(QFont::DemiBold);
    return f;
}

bool split(const QStyleOptionToolButton* opt) {
    return opt->features.testFlag(QStyleOptionToolButton::MenuButtonPopup);
}

QFont paintFont(const QToolButton* button) {
    if (const auto* action = button->defaultAction(); action && action->font().resolve())
        return action->font().resolve(labelFont());
    return labelFont();
}

QString state(const QToolButton* button, const QStyleOptionToolButton* opt) {
    if (!button->isEnabled())
        return "Disabled";
    const auto fixed = button->property("figmaToolSnapshotState").toString();
    if (!fixed.isEmpty())
        return fixed;
    if (split(opt) && opt->state.testFlag(QStyle::State_Sunken) &&
        opt->activeSubControls.testFlag(QStyle::SC_ToolButtonMenu))
        return "Open";
    if (button->isDown())
        return "Pressed";
    const QString checked = button->isChecked() ? "Checked" : "";
    if (button->hasFocus())
        return checked + "Focus";
    if (opt->state.testFlag(QStyle::State_MouseOver))
        return checked + "Hover";
    return checked.isEmpty() ? "Default" : checked;
}

struct Parts {
    QRect main, menu, divider, mainHit, menuHit;
};

Parts geometry(const QToolButton* button, const QStyleOptionToolButton* opt) {
    const auto visual = state(button, opt);
    const int stroke = visual.endsWith("Focus") || visual == "Open" ? 2 : 1;
    const int inset = button->property("figmaToolSourceSnapshot").toBool() ? stroke : 2;
    const QRect r = opt->rect;
    const int reserve = split(opt) ? 29 : 0;
    Parts g;
    g.main = QRect(r.x() + inset, r.y(), qMax(0, r.width() - inset * 2 - reserve), r.height());
    if (split(opt)) {
        g.menu = QRect(r.right() + 1 - inset - 28, r.y(), 28, r.height());
        g.divider = QRect(g.menu.x() - 1, r.y() + (r.height() - 20) / 2, 1, 20);
        g.menuHit = QRect(g.divider.x(), r.y(), r.right() - g.divider.x() + 1, r.height());
        g.mainHit = QRect(r.x(), r.y(), g.divider.x() - r.x(), r.height());
    } else
        g.mainHit = r;
    for (auto* part : {&g.main, &g.menu, &g.divider, &g.mainHit, &g.menuHit})
        *part = QStyle::visualRect(opt->direction, r, *part);
    return g;
}

class ToolStyle final : public QProxyStyle {
public:
    explicit ToolStyle(QToolButton* button) : QProxyStyle(QStyleFactory::create("Fusion")) {
        setParent(button);
        setObjectName("figmaToolStyle");
    }

    QSize sizeFromContents(ContentsType ct, const QStyleOption* option, const QSize& size,
                           const QWidget* widget) const override {
        const auto* button = qobject_cast<const QToolButton*>(widget);
        const auto* opt = qstyleoption_cast<const QStyleOptionToolButton*>(option);
        if (ct != CT_ToolButton || !button || !opt)
            return QProxyStyle::sizeFromContents(ct, option, size, widget);
        const bool icon = opt->toolButtonStyle != Qt::ToolButtonTextOnly;
        const bool text = opt->toolButtonStyle != Qt::ToolButtonIconOnly && !opt->text.isEmpty();
        const QSize slot = icon ? opt->iconSize.expandedTo(QSize(18, 18)) : QSize(0, 0);
        const int tw =
            text ? QFontMetrics(paintFont(button)).size(Qt::TextShowMnemonic, opt->text).width()
                 : 0;
        const int th = text ? qMax(22, QFontMetrics(paintFont(button)).height()) : 0;
        const bool vertical = opt->toolButtonStyle == Qt::ToolButtonTextUnderIcon;
        const int gap = icon && text ? 6 : 0;
        const int width = vertical ? qMax(slot.width(), tw) : slot.width() + gap + tw;
        const int height = vertical ? slot.height() + gap + th : qMax(slot.height(), th);
        return QSize(width + 20 + (split(opt) ? 29 : 0), qMax(32, height + 8));
    }

    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex* option, SubControl sc,
                         const QWidget* widget) const override {
        const auto* button = qobject_cast<const QToolButton*>(widget);
        const auto* opt = qstyleoption_cast<const QStyleOptionToolButton*>(option);
        if (cc == CC_ToolButton && button && opt) {
            const auto g = geometry(button, opt);
            if (sc == SC_ToolButton)
                return g.mainHit;
            if (sc == SC_ToolButtonMenu)
                return g.menuHit;
        }
        return QProxyStyle::subControlRect(cc, option, sc, widget);
    }

    SubControl hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex* opt,
                                     const QPoint& pos, const QWidget* widget) const override {
        if (cc == CC_ToolButton && qobject_cast<const QToolButton*>(widget)) {
            if (subControlRect(cc, opt, SC_ToolButtonMenu, widget).contains(pos))
                return SC_ToolButtonMenu;
            if (subControlRect(cc, opt, SC_ToolButton, widget).contains(pos))
                return SC_ToolButton;
            return SC_None;
        }
        return QProxyStyle::hitTestComplexControl(cc, opt, pos, widget);
    }

    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex* option, QPainter* p,
                            const QWidget* widget) const override {
        const auto* button = qobject_cast<const QToolButton*>(widget);
        const auto* opt = qstyleoption_cast<const QStyleOptionToolButton*>(option);
        if (cc != CC_ToolButton || !button || !opt) {
            QProxyStyle::drawComplexControl(cc, option, p, widget);
            return;
        }
        const auto visual = state(button, opt);
        const bool disabled = visual == "Disabled",
                   accent = visual.startsWith("Checked") || visual == "Open";
        const bool focus = visual.endsWith("Focus") || visual == "Open";
        const bool transparent = button->autoRaise() && (visual == "Default" || visual == "Focus");
        const bool hover = visual.endsWith("Hover"), pressed = visual == "Pressed";
        const QColor fill(disabled           ? "#f3f3f3"
                          : hover || pressed ? "#f1ecff"
                          : accent           ? "#f3efff"
                          : transparent      ? "#00000000"
                                             : "#ffffff");
        const QColor border(disabled          ? "#d0d0d0"
                            : focus || accent ? "#6235f7"
                            : pressed         ? "#532cd1"
                            : transparent     ? "#00000000"
                                              : "#a7a7a7");
        const QColor ink(disabled ? "#8a8a8a" : accent ? "#6235f7" : "#191919");
        const int stroke = focus ? 2 : 1;
        const auto g = geometry(button, opt);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(QPen(border, stroke));
        p->setBrush(fill);
        const qreal half = stroke / 2.;
        p->drawRoundedRect(QRectF(opt->rect).adjusted(half, half, -half, -half), 4 - half,
                           4 - half);
        if (split(opt)) {
            p->fillRect(g.divider, QColor("#d0d0d0"));
            // Authored 7x2 rounded rectangles and transforms, not a substituted glyph.
            const QPointF origin(g.menu.x() + 7, g.menu.y() + (g.menu.height() - 14) / 2.);
            p->save();
            p->translate(origin);
            p->setPen(Qt::NoPen);
            p->setBrush(ink);
            p->save();
            p->translate(1.5, 5);
            p->rotate(45);
            p->drawRoundedRect(QRectF(0, 0, 7, 2), 1, 1);
            p->restore();
            p->translate(6, 5);
            p->rotate(-45);
            p->drawRoundedRect(QRectF(0, 0, 7, 2), 1, 1);
            p->restore();
        }
        const bool hasIcon = opt->toolButtonStyle != Qt::ToolButtonTextOnly;
        const bool hasText = opt->toolButtonStyle != Qt::ToolButtonIconOnly && !opt->text.isEmpty();
        const bool vertical = opt->toolButtonStyle == Qt::ToolButtonTextUnderIcon;
        const auto font = paintFont(button);
        const int lineHeight = qMax(22, QFontMetrics(font).height());
        const int slotW = hasIcon ? qMax(18, opt->iconSize.width()) : 0;
        const int slotH = hasIcon ? qMax(18, opt->iconSize.height()) : 0;
        QRect logicalMain = visualRect(opt->direction, opt->rect, g.main);
        QRect iconRect, textRect;
        if (vertical) {
            const int total = slotH + (hasIcon && hasText ? 6 : 0) + (hasText ? lineHeight : 0);
            const int y = logicalMain.y() + (logicalMain.height() - total) / 2;
            if (hasIcon)
                iconRect = QRect(logicalMain.x() + (logicalMain.width() - slotW) / 2, y,
                                 opt->iconSize.width(), opt->iconSize.height());
            if (hasText)
                textRect = QRect(logicalMain.x() + 8, y + slotH + (hasIcon ? 6 : 0),
                                 qMax(0, logicalMain.width() - 16), lineHeight);
        } else {
            if (hasIcon)
                iconRect =
                    QRect(logicalMain.x() + (hasText ? 8 : (logicalMain.width() - slotW) / 2),
                          logicalMain.y() + (logicalMain.height() - slotH) / 2,
                          opt->iconSize.width(), opt->iconSize.height());
            const int x = logicalMain.x() + 8 + (hasIcon ? slotW + 6 : 0);
            if (hasText)
                textRect = QRect(x, logicalMain.y() + (logicalMain.height() - lineHeight) / 2,
                                 qMax(0, logicalMain.right() + 1 - 8 - x), lineHeight);
        }
        iconRect = visualRect(opt->direction, opt->rect, iconRect);
        textRect = visualRect(opt->direction, opt->rect, textRect);
        p->setClipRect(g.main);
        if (hasIcon) {
            if (opt->arrowType != Qt::NoArrow) {
                QStyleOption arrow;
                arrow.rect = iconRect;
                arrow.palette = opt->palette;
                arrow.palette.setColor(QPalette::ButtonText, ink);
                arrow.state = opt->state;
                const auto primitive = opt->arrowType == Qt::UpArrow     ? PE_IndicatorArrowUp
                                       : opt->arrowType == Qt::DownArrow ? PE_IndicatorArrowDown
                                       : opt->arrowType == Qt::LeftArrow ? PE_IndicatorArrowLeft
                                                                         : PE_IndicatorArrowRight;
                QProxyStyle::drawPrimitive(primitive, &arrow, p, widget);
            } else
                opt->icon.paint(p, iconRect, Qt::AlignLeft | Qt::AlignTop,
                                disabled ? QIcon::Disabled : QIcon::Normal,
                                accent ? QIcon::On : QIcon::Off);
        }
        if (hasText) {
            p->setPen(ink);
            p->setFont(font);
            int flags = Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextShowMnemonic;
            flags |= vertical || !hasIcon                ? Qt::AlignHCenter
                     : opt->direction == Qt::RightToLeft ? Qt::AlignRight
                                                         : Qt::AlignLeft;
            if (!styleHint(SH_UnderlineShortcut, opt, widget))
                flags |= Qt::TextHideMnemonic;
            p->drawText(textRect, flags, opt->text);
        }
        p->restore();
        auto* mutableButton = const_cast<QToolButton*>(button);
        mutableButton->setProperty("figmaToolVisualState", visual);
        mutableButton->setProperty("figmaToolPaintFont", font);
        mutableButton->setProperty("figmaToolIconBounds", iconRect);
        mutableButton->setProperty("figmaToolLabelBounds", textRect);
        mutableButton->setProperty("figmaToolMenuBounds", g.menu);
    }
};
}

QIcon figmaToolButtonIcon() {
    QIcon icon;
    for (const auto mode : {QIcon::Normal, QIcon::Active, QIcon::Selected, QIcon::Disabled})
        for (const auto value : {QIcon::Off, QIcon::On})
            icon.addFile(":/figma/toolbutton/Action" +
                             QString(mode == QIcon::Disabled ? "Disabled"
                                     : value == QIcon::On    ? "Accent"
                                                             : "Primary") +
                             ".svg",
                         QSize(16, 16), mode, value);
    return icon;
}

void applyFigmaToolButtonAppearance(QToolButton* button, const QString& snapshotState,
                                    bool sourceSnapshot) {
    if (!button ||
        !QStringList{"", "Default", "Hover", "Pressed", "Focus", "Checked", "CheckedHover",
                     "CheckedFocus", "Open"}
             .contains(snapshotState) ||
        (snapshotState == "Open" && button->popupMode() != QToolButton::MenuButtonPopup))
        throw std::invalid_argument("Captured ToolButton requires a native button and valid source "
                                    "state; Open requires Split");
    static const bool fonts = [] {
        QFontDatabase::addApplicationFont(":/figma/fonts/Inter-SemiBold.otf");
        return true;
    }();
    Q_UNUSED(fonts);
    button->setProperty("figmaAppearance", "figma-light");
    button->setProperty("designSystemStyleOwner", "figma-toolbutton-captured");
    button->setProperty("figmaToolSnapshotState", snapshotState);
    button->setProperty("figmaToolSourceSnapshot", sourceSnapshot);
    button->setAttribute(Qt::WA_Hover);
    if (!button->defaultAction() || !button->defaultAction()->font().resolve())
        button->setFont(labelFont());
    if (!button->findChild<QObject*>("figmaToolStyle", Qt::FindDirectChildrenOnly))
        button->setStyle(new ToolStyle(button));
    button->ensurePolished();
    // Qt 5.15 setPopupMode does not clear its cached sizeHint. Use the native
    // style-change path to invalidate geometry without toggling command properties.
    QEvent appearanceChanged(QEvent::StyleChange);
    QCoreApplication::sendEvent(button, &appearanceChanged);
    button->updateGeometry();
    button->update();
}
}
