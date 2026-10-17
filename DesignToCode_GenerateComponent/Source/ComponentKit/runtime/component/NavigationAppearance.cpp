#include "NavigationAppearance.h"
#include "Z_Menu.h"
#include "Z_ToolButton.h"
#include <QBoxLayout>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QEvent>
#include <QDynamicPropertyChangeEvent>
#include <QFile>
#include <QFontDatabase>
#include <QGraphicsDropShadowEffect>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOptionMenuItem>
#include <QToolBar>
#include <QToolButton>
#include <QWidgetAction>
#include <stdexcept>

namespace commonWidget::navigation {
namespace {
QColor role(const char* name, const QWidget* widget = nullptr) {
    static const auto values = [] {
        QFile file(":/figma/navigation-visuals.json");
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("Figma navigation visual contract missing");
        return QJsonDocument::fromJson(file.readAll()).object();
    }();
    const auto key = QString::fromLatin1(name);
    auto variables = values["variables"].toObject();
    if (widget && widget->property("figmaSemanticMode").toString() == "Dark") {
        const auto dark = values["darkVariables"].toObject();
        if (dark.contains(key))
            variables[key] = dark[key];
    }
    const QColor result(variables.value(key).toString());
    if (!result.isValid())
        throw std::runtime_error("Figma navigation color missing");
    return result;
}

QFont labelFont(int size = 14, bool strong = false) {
    static const bool fontsLoaded = [] {
        if (QFontDatabase::addApplicationFont(
                QStringLiteral(":/fonts/HarmonyOS_Sans_Regular.ttf")) < 0)
            throw std::runtime_error("HarmonyOS Sans font unavailable");
        return true;
    }();
    Q_UNUSED(fontsLoaded);
    QFont font("HarmonyOS Sans");
    font.setPixelSize(size);
    font.setWeight(strong ? QFont::DemiBold : QFont::Normal);
    return font;
}

// QMenu computes action hit rectangles from the style. Supply the Figma item
// metrics there rather than clipping a wider, platform-dependent native layout.
class NavigationStyle final : public QProxyStyle {
public:
    NavigationStyle() : QProxyStyle(QStyleFactory::create("Fusion")) {}

    QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size,
                           const QWidget* widget) const override {
        if (type == CT_MenuItem) {
            const auto* item = qstyleoption_cast<const QStyleOptionMenuItem*>(option);
            int shortcutWidth = 0;
            // Qt 5.15 adds this column *after* sizeFromContents (qmenu.cpp,
            // updateActionRects). Reserve it within, not beyond, the 260px item.
            for (auto* action : widget->actions()) {
                if (!action->isVisible() || action->isSeparator())
                    continue;
                const auto text = action->text().contains('\t')
                                      ? action->text().section('\t', 1)
                                      : action->shortcut().toString(QKeySequence::NativeText);
                shortcutWidth = qMax(shortcutWidth, widget->fontMetrics().horizontalAdvance(text));
            }
            return QSize(qMax(1, 260 - shortcutWidth),
                         item && item->menuItemType == QStyleOptionMenuItem::Separator ? 9 : 40);
        }
        if (type == CT_MenuBarItem) {
            const auto* item = qstyleoption_cast<const QStyleOptionMenuItem*>(option);
            return QSize(
                qMax(64, item ? QFontMetrics(labelFont())
                                        .horizontalAdvance(QString(item->text).remove('&')) +
                                    32
                              : 64),
                40);
        }
        return QProxyStyle::sizeFromContents(type, option, size, widget);
    }

    int pixelMetric(PixelMetric metric, const QStyleOption* option,
                    const QWidget* widget) const override {
        const bool single = widget && widget->property("figmaSingleItem").toBool();
        switch (metric) {
        case PM_MenuPanelWidth:
            return 0;
        case PM_MenuHMargin:
        case PM_MenuVMargin:
            return single ? 0 : 6;
        case PM_MenuBarPanelWidth:
            return 0;
        case PM_MenuBarHMargin:
        case PM_MenuBarVMargin:
            return single ? 0 : 8;
        case PM_MenuBarItemSpacing:
            return single ? 0 : 4;
        default:
            return QProxyStyle::pixelMetric(metric, option, widget);
        }
    }
};

QString stateOf(QAction* action, QAction* active) {
    if (!action->isEnabled())
        return "Disabled";
    const auto forced = action->property("figmaPreviewState").toString();
    if (!forced.isEmpty())
        return forced;
    return action == active ? "Hover" : "Default";
}

void paintIcon(QPainter& painter, const QString& name, const QRect& bounds) {
    QIcon(":/figma/navigation/" + name + ".svg").paint(&painter, bounds);
}

void paintMenuItem(QPainter& p, const QRect& rect, QAction* action, QAction* active) {
    p.save();
    p.translate(rect.topLeft());
    const auto state = stateOf(action, active);
    const auto type = action->property("menuItemType").toString();
    const bool disabled = state == "Disabled", highlighted = state == "Active";
    const bool danger = type == "Danger" && !disabled;
    const QColor foreground = role(disabled ? "var(--component-menu-disabled)"
                                   : danger ? "var(--component-menu-danger)"
                                            : "var(--component-menu-text)");
    p.setPen(Qt::NoPen);
    p.setBrush(role(highlighted && danger             ? "var(--component-menu-dangersurface)"
                    : highlighted || state == "Hover" ? "var(--component-menu-hover)"
                                                      : "var(--component-menu-surface)"));
    p.drawRoundedRect(QRectF(0, 0, rect.width(), rect.height()), 6, 6);
    if (highlighted && !danger) {
        p.setBrush(role("var(--component-menu-active)"));
        p.drawRoundedRect(QRectF(0, 5, 3, 30), 2, 2);
    }
    p.setFont(labelFont(14, highlighted));
    p.setPen(foreground);
    const bool check = action->isCheckable();
    if (check) {
        p.setPen(action->isChecked() && !disabled ? role("var(--component-menu-indicator)")
                                                  : foreground);
        const QString mark =
            type == "Radio"
                ? (action->isChecked() ? QString::fromUtf8("●") : QString::fromUtf8("○"))
                : (action->isChecked() ? QString::fromUtf8("✓") : QString::fromUtf8("□"));
        p.setFont(labelFont(14, true));
        p.drawText(QRect(12, 10, 20, 22), Qt::AlignVCenter | Qt::AlignLeft, mark);
    } else if (action->property("showIcon").isNull() || action->property("showIcon").toBool()) {
        paintIcon(p,
                  disabled ? "action-disabled"
                  : danger ? "action-danger"
                           : "action-primary",
                  QRect(12, 12, 16, 16));
    }
    p.setPen(foreground);
    p.setFont(labelFont(14, highlighted));
    p.drawText(QRect(40, 10, qMax(0, rect.width() - 48), 22), Qt::AlignVCenter | Qt::AlignLeft,
               action->text().section('\t', 0, 0).remove('&'));
    if (action->property("showShortcut").isNull() || action->property("showShortcut").toBool()) {
        p.setFont(labelFont(12));
        p.setPen(
            role(disabled ? "var(--component-menu-disabled)" : "var(--component-menu-secondary)"));
        const QString shortcut = action->shortcut().toString(QKeySequence::NativeText);
        p.drawText(QRect(rect.width() - 84, 0, 78, 40), Qt::AlignVCenter | Qt::AlignLeft, shortcut);
    }
    if (action->menu())
        paintIcon(p, disabled ? "chevron-disabled" : "chevron-right",
                  QRect(rect.width() - 28, 10, 16, 16));
    p.restore();
}

class PaintFilter final : public QObject {
public:
    explicit PaintFilter(QWidget* parent) : QObject(parent) {}

    bool eventFilter(QObject* object, QEvent* event) override {
        if (event->type() == QEvent::DynamicPropertyChange &&
            static_cast<QDynamicPropertyChangeEvent*>(event)->propertyName() ==
                "figmaSemanticMode") {
            if (auto* bar = qobject_cast<QMenuBar*>(object)) {
                if (auto* corner = bar->cornerWidget())
                    corner->setStyleSheet(
                        QString("QToolButton {border:0;background:%1;color:%2;border-radius:4px;}")
                            .arg(role("menu-bar-item/base/hover/container/color", bar).name(),
                                 role("menu-bar-item/base/default/label/color", bar).name()));
                bar->update();
            }
        }
        if (event->type() != QEvent::Paint)
            return false;
        auto* widget = qobject_cast<QWidget*>(object);
        auto* menu = qobject_cast<QMenu*>(widget);
        auto* bar = qobject_cast<QMenuBar*>(widget);
        if (!menu && !bar)
            return false;
        QPainter p(widget);
        p.setRenderHint(QPainter::Antialiasing);
        const bool specimen = widget->property("figmaSingleItem").toBool();
        p.setPen(specimen ? QPen(Qt::NoPen)
                          : QPen(role(bar ? "menu-bar/base/default/border/color"
                                          : "var(--component-menu-border)",
                                      widget),
                                 1));
        p.setBrush(
            role(bar ? "menu-bar/base/default/container/color" : "var(--component-menu-surface)",
                 widget));
        p.drawRoundedRect(QRectF(widget->rect()).adjusted(.5, .5, -.5, -.5), menu ? 8 : 4,
                          menu ? 8 : 4);
        for (auto* action : widget->actions()) {
            if (!action->isVisible())
                continue;
            const QRect rect = menu ? menu->actionGeometry(action) : bar->actionGeometry(action);
            if (rect.isEmpty())
                continue;
            if (action->isSeparator()) {
                p.setPen(role("var(--component-menu-divider)"));
                p.drawLine(rect.left() + 8, rect.center().y(), rect.right() - 8, rect.center().y());
            } else if (menu) {
                paintMenuItem(p, rect, action, menu->activeAction());
            } else {
                auto state = stateOf(action, bar->activeAction());
                if (action->property("figmaPreviewState").toString().isEmpty() && action->menu() &&
                    action->menu()->isVisible())
                    state = "Open";
                const bool disabled = state == "Disabled";
                p.setPen(state == "Keyboard"
                             ? QPen(role("menu-bar-item/base/focused/focus-ring/color", widget), 2)
                             : QPen(Qt::NoPen));
                p.setBrush(role(state == "Open"    ? "menu-bar-item/base/open/container/color"
                                : state == "Hover" ? "menu-bar-item/base/hover/container/color"
                                                   : "menu-bar/base/default/container/color",
                                widget));
                p.drawRoundedRect(QRectF(rect).adjusted(1, 1, -1, -1), 4, 4);
                p.setFont(labelFont());
                p.setPen(role(disabled ? "menu-bar-item/base/disabled/label/color"
                                       : "menu-bar-item/base/default/label/color",
                              widget));
                p.drawText(rect.adjusted(16, 0, -10, 0), Qt::AlignVCenter | Qt::AlignLeft,
                           action->text().remove('&'));
                if (action->property("showMnemonic").toBool()) {
                    p.setPen(role(disabled ? "menu-bar-item/base/disabled/label/color"
                                           : "menu-bar-item/base/default/mnemonic/color",
                                  widget));
                    p.drawLine(rect.left() + 16, rect.bottom() - 10, rect.left() + 24,
                               rect.bottom() - 10);
                }
            }
        }
        return true;
    }
};

class Handle final : public QWidget {
    bool vertical;

public:
    Handle(bool v, QWidget* p) : QWidget(p), vertical(v) {
        setObjectName("figmaToolBarHandle");
        setAccessibleName("Move handle");
        setFixedSize(v ? QSize(56, 12) : QSize(12, 32));
    }

    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setPen(Qt::NoPen);
        p.setBrush(role("var(--component-toolbar-handle)"));
        for (int i = -1; i <= 1; ++i)
            p.drawEllipse(QPointF(width() / 2. + (vertical ? i * 5 : 0),
                                  height() / 2. + (vertical ? 0 : i * 5)),
                          1., 1.);
    }
};

class ActionButton final : public Z_ToolButton {
public:
    explicit ActionButton(QWidget* parent) : Z_ToolButton(parent) {}

    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        if (underMouse() || isDown()) {
            p.setPen(Qt::NoPen);
            p.setBrush(role("var(--component-toolbar-hover)"));
            p.drawRoundedRect(rect(), 4, 4);
        }
        paintIcon(p, "action-primary", QRect(8, 7, 16, 16));
        if (toolButtonStyle() == Qt::ToolButtonTextBesideIcon) {
            p.setFont(labelFont(14, true));
            p.setPen(role("var(--component-toolbar-text)"));
            p.drawText(QRect(32, 5, width() - 32, 22), Qt::AlignLeft | Qt::AlignVCenter, text());
        }
    }
};
}

void install(QWidget* widget) {
    if (widget->property("figmaNavigationStyle").toBool())
        return;
    widget->setProperty("figmaNavigationStyle", true);
    widget->setFont(labelFont());
    auto* style = new NavigationStyle;
    style->setParent(widget);
    widget->setStyle(style);
    if (qobject_cast<QMenu*>(widget)) {
        widget->installEventFilter(new PaintFilter(widget));
        if (!widget->property("figmaSingleItem").toBool()) {
            auto* effect = new QGraphicsDropShadowEffect(widget);
            effect->setOffset(0, 6);
            effect->setBlurRadius(18);
            effect->setColor(QColor(0, 0, 0, 36));
            widget->setGraphicsEffect(effect);
        }
    } else if (auto* bar = qobject_cast<QMenuBar*>(widget)) {
        bar->setNativeMenuBar(false);
        bar->installEventFilter(new PaintFilter(widget));
    }
}

void setPreviewState(QAction* action, const QString& state) {
    action->setProperty("figmaPreviewState", state);
    action->setEnabled(state != "Disabled");
    for (auto* widget : action->associatedWidgets())
        widget->update();
}

void populateMenu(QMenu* menu) {
    install(menu);
    menu->clear();
    const QStringList labels{"New file",     "Open…",         "Auto save",     "Workspace",
                             "Recent files", "Export legacy", "Delete project"};
    const QStringList types{"Action", "Action", "Radio", "Check", "Submenu", "Action", "Danger"};
    const QStringList shortcuts{"Ctrl+N", "Ctrl+O", "", "", "", "Ctrl+E", "Shift+Del"};
    for (int i = 0; i < labels.size(); ++i) {
        if (i == 2)
            menu->addSeparator();
        auto* action = menu->addAction(labels[i]);
        action->setProperty("menuItemType", types[i]);
        action->setShortcut(QKeySequence(shortcuts[i]));
        action->setShortcutVisibleInContextMenu(true);
        action->setCheckable(i == 2 || i == 3);
        action->setChecked(i == 2 || i == 3);
        action->setEnabled(i != 5);
        if (i == 4) {
            auto* sub = new Z_Menu(menu);
            sub->addAction("Recent file");
            action->setMenu(sub);
        }
    }
}

void configureMenuItem(QMenu* menu, const QString& type, const QString& state) {
    menu->setProperty("figmaSingleItem", true);
    install(menu);
    menu->clear();
    auto* action = menu->addAction("Menu item");
    action->setProperty("menuItemType", type);
    action->setShortcut(QKeySequence("Ctrl+K"));
    action->setShortcutVisibleInContextMenu(true);
    action->setCheckable(type == "Check" || type == "Radio");
    // This is the checked-content specimen in Figma, not a state-to-data rule.
    action->setChecked(action->isCheckable() && state == "Active");
    if (type == "Submenu") {
        auto* sub = new Z_Menu(menu);
        sub->addAction("Recent file");
        action->setMenu(sub);
    }
    setPreviewState(action, state);
    menu->setFixedSize(260, 40);
}

void populateMenuBar(QMenuBar* bar, const QStringList& requested) {
    install(bar);
    bar->clear();
    const QStringList labels =
        requested.isEmpty() ? QStringList{"File", "Edit", "View", "Navigate", "Help"} : requested;
    for (int i = 0; i < labels.size(); ++i) {
        auto* popup = new Z_Menu(bar);
        populateMenu(popup);
        auto* action = bar->addMenu(popup);
        action->setText(labels[i]);
        action->setProperty("showMnemonic", i != 4);
    }
    auto* corner = new QToolButton(bar);
    corner->setObjectName("menuBarCorner");
    corner->setText("…");
    corner->setFixedSize(32, 32);
    corner->setStyleSheet("QToolButton {border:0;background:#f3f3f3;border-radius:4px;}");
    bar->setCornerWidget(corner);
}

void setMenuBarCornerVisible(QMenuBar* bar, bool visible) {
    if (!bar)
        return;
    if (!bar->cornerWidget() && visible) {
        auto* corner = new QToolButton(bar);
        corner->setObjectName("menuBarCorner");
        corner->setText("…");
        corner->setFixedSize(32, 32);
        corner->setStyleSheet(
            "QToolButton {border:0;background:#f3f3f3;border-radius:4px;}");
        bar->setCornerWidget(corner);
    }
    if (auto* corner = bar->cornerWidget())
        corner->setVisible(visible);
    bar->setProperty("showCorner", visible);
}

void configureToolBar(QToolBar* bar, const QMap<QString, QString>& props) {
    const auto previousActions = bar->actions();
    bar->clear();
    qDeleteAll(previousActions);
    const bool vertical = props.value("Orientation") == "Vertical";
    const bool text = props.value("Presentation") == "TextBesideIcon";
    const bool floating = props.value("Placement") == "Floating";
    const bool overflow = props.value("Capacity") == "Overflow";
    bar->setOrientation(vertical ? Qt::Vertical : Qt::Horizontal);
    bar->setMovable(false);
    bar->setProperty("figmaPlacementPreview", props.value("Placement"));
    bar->setStyleSheet(
        QString("QToolBar {background:white; border:%1px solid "
                "%2;border-radius:%3px;padding:8px;spacing:4px;} QToolButton {border:0; "
                "background:transparent; font:600 14px 'HarmonyOS Sans'; padding:0 8px; height:32px;} "
                "QToolButton:hover {background:#f3f3f3;} QToolBar::separator "
                "{background:#d0d0d0;width:1px;height:1px;margin:4px;}")
            .arg(floating ? 2 : 1)
            .arg(floating ? "#6235f7" : "#d0d0d0")
            .arg(floating ? 8 : 4));
    bar->setFixedSize(vertical ? QSize(text ? 120 : 88, 220) : QSize(280, 56));
    if (floating) {
        auto* effect = new QGraphicsDropShadowEffect(bar);
        effect->setOffset(0, 6);
        effect->setBlurRadius(12);
        effect->setColor(QColor(0, 0, 0, 41));
        bar->setGraphicsEffect(effect);
    } else
        bar->setGraphicsEffect(nullptr);
    auto addCentered = [bar, vertical](QWidget* widget) {
        if (!vertical) {
            bar->addWidget(widget);
            return;
        }
        auto* row = new QWidget(bar);
        auto* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(widget, 0, Qt::AlignHCenter);
        row->setFixedHeight(widget->height());
        bar->addWidget(row);
    };
    if (props.value("Show handle", "true") != "false")
        addCentered(new Handle(vertical, bar));
    for (int i = 0; i < 2; ++i) {
        if (i) {
            auto* separator = new QWidget(bar);
            separator->setObjectName("figmaToolBarSeparator");
            separator->setFixedSize(vertical ? QSize(56, 1) : QSize(1, 24));
            separator->setStyleSheet("background:#d0d0d0;");
            addCentered(separator);
        }
        auto* button = new ActionButton(bar);
        auto* action = new QAction(QIcon(":/figma/navigation/action-primary.svg"),
                                   i ? "Stop" : "Refresh", button);
        button->setDefaultAction(action);
        button->setAutoRaise(true);
        button->setIconSize(QSize(16, 16));
        button->setToolButtonStyle(text ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
        button->setFixedSize(text ? QSize(vertical ? 104 : 85, 32) : QSize(32, 32));
        addCentered(button);
    }
    auto* spacer = new QWidget(bar);
    spacer->setSizePolicy(vertical ? QSizePolicy::Preferred : QSizePolicy::Expanding,
                          vertical ? QSizePolicy::Expanding : QSizePolicy::Preferred);
    bar->addWidget(spacer);
    if (overflow) {
        auto* extension = new QToolButton(bar);
        extension->setObjectName("figmaOverflowButton");
        extension->setFixedSize(32, 32);
        extension->setIcon(QIcon(":/figma/navigation/chevron-right.svg"));
        extension->setIconSize(QSize(16, 16));
        extension->setStyleSheet("QToolButton{background:#f3f3f3;border:0;border-radius:4px;"
                                 "padding:0;} QToolButton::menu-indicator{image:none;}");
        auto* menu = new Z_Menu(extension);
        populateMenu(menu);
        extension->setMenu(menu);
        extension->setPopupMode(QToolButton::InstantPopup);
        addCentered(extension);
    }
}
} // namespace commonWidget::navigation
