#include "NavigationGallery.h"
#include "FigmaGalleryView.h"
#include "NavigationAppearance.h"
#include "Z_DesignComponents.h"
#include "native/Z_Menu.h"
#include "native/Z_MenuBar.h"
#include "native/Z_ToolBar.h"
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QPainter>
#include <QSignalBlocker>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QTimer>
#include <QtMath>

namespace native_gallery {
namespace nav = commonWidget::navigation;
namespace {
QWidget* panel(QWidget* parent, const QRect& rect, const QString& color = "white") {
    auto* w = new QWidget(parent);
    w->setGeometry(rect);
    w->setObjectName("galleryPanel");
    w->setStyleSheet(
        QString("QWidget#galleryPanel {background:%1;border-radius:10px;}").arg(color));
    return w;
}
QLabel* text(QWidget* parent, const QRect& rect, const QString& value, int size = 12) {
    auto* label = new QLabel(value, parent);
    label->setGeometry(rect);
    label->setWordWrap(true);
    label->setStyleSheet(
        QString("background:transparent;border:0;color:#505868;font:600 %1px 'Inter';").arg(size));
    return label;
}
commonWidget::Z_Menu* menu(QWidget* parent, int x, int y, bool specimen = false,
                           const QString& type = {}, const QString& state = {}) {
    auto* m = new commonWidget::Z_Menu(parent);
    m->setWindowFlags(Qt::Widget);
    if (specimen)
        nav::configureMenuItem(m, type, state);
    else {
        nav::populateMenu(m);
        m->setFixedSize(272, 303);
    }
    m->move(x, y);
    m->show();
    return m;
}
void specimenStates(QMenu* m) {
    const QStringList states{"Default", "Hover",   "",         "Active",
                             "Active",  "Default", "Disabled", "Hover"};
    for (int i = 0; i < m->actions().size(); ++i)
        if (!m->actions()[i]->isSeparator())
            nav::setPreviewState(m->actions()[i], states.value(i));
}
commonWidget::Z_MenuBar* menuBar(QWidget* parent, const QRect& rect,
                                 const QStringList& labels = {}) {
    auto* m = new commonWidget::Z_MenuBar(parent);
    nav::populateMenuBar(m, labels);
    m->setGeometry(rect);
    m->show();
    return m;
}
void barStates(QMenuBar* bar) {
    const QStringList states{"Default", "Hover", "Open", "Keyboard", "Disabled"};
    for (int i = 0; i < bar->actions().size(); ++i)
        nav::setPreviewState(bar->actions()[i], states.value(i));
}
QGraphicsView* scaled(QWidget* widget, QWidget* parent, const QRect& rect, qreal factor) {
    // Nested graphics proxies cannot compose QWidget graphics effects reliably.
    // Put the shadow on the graphics item rather than its embedded widget.
    auto* originalShadow = qobject_cast<QGraphicsDropShadowEffect*>(widget->graphicsEffect());
    const bool shadow = originalShadow != nullptr;
    const QPointF offset = shadow ? originalShadow->offset() : QPointF();
    const qreal blur = shadow ? originalShadow->blurRadius() : 0;
    const QColor color = shadow ? originalShadow->color() : QColor();
    const int padding =
        shadow ? qCeil((blur + qMax(qAbs(offset.x()), qAbs(offset.y()))) * factor) + 2 : 0;
    widget->setGraphicsEffect(nullptr);
    auto* v = new QGraphicsView(parent);
    auto* s = new QGraphicsScene(v);
    v->setScene(s);
    auto* proxy = s->addWidget(widget);
    proxy->setPos(0, 0);
    if (shadow) {
        auto* effect = new QGraphicsDropShadowEffect;
        effect->setOffset(offset);
        effect->setBlurRadius(blur);
        effect->setColor(color);
        proxy->setGraphicsEffect(effect);
    }
    const qreal scenePadding = padding / factor;
    s->setSceneRect(
        QRectF(widget->rect()).adjusted(-scenePadding, -scenePadding, scenePadding, scenePadding));
    widget->show();
    v->setGeometry(rect.adjusted(-padding, -padding, padding, padding));
    v->setFrameShape(QFrame::NoFrame);
    v->setStyleSheet("background:transparent;border:0;");
    v->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    v->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    v->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    v->scale(factor, factor);
    return v;
}
QMap<QString, QString> toolbarProps(int row, int col) {
    return {{"Orientation", row < 2 ? "Horizontal" : "Vertical"},
            {"Placement", col < 2 ? "Docked" : "Floating"},
            {"Capacity", col % 2 ? "Overflow" : "Fit"},
            {"Presentation", row % 2 ? "TextBesideIcon" : "IconOnly"}};
}
void inspect(QTreeWidget* tree, QWidget* preview, const QString& pageId) {
    tree->clear();
    auto* identity = new QTreeWidgetItem(
        tree, {QString::fromUtf8("身份与契约"), preview->property("designSystemClass").toString()});
    new QTreeWidgetItem(identity, {"Qt", preview->metaObject()->className()});
    new QTreeWidgetItem(identity, {"Figma page", pageId});
    for (const auto& key : preview->dynamicPropertyNames())
        if (!key.startsWith('_'))
            new QTreeWidgetItem(identity, {key, preview->property(key).toString()});
    new QTreeWidgetItem(
        identity,
        {QString::fromUtf8("交互状态"),
         QString::fromUtf8("画廊强制状态与 QAction 数据分开；留空时使用真实鼠标/键盘状态")});
    for (auto* a : preview->actions()) {
        auto* row = new QTreeWidgetItem(
            tree,
            {a->text().isEmpty() ? QString::fromUtf8("分隔线 / 容器") : a->text(), "QAction"});
        for (const auto* key :
             {"enabled", "checkable", "checked", "text", "shortcut", "menuItemType",
              "figmaPreviewState", "showMnemonic", "showIcon", "showShortcut"})
            new QTreeWidgetItem(row, {key, a->property(key).toString()});
    }
    identity->setExpanded(true);
}
} // namespace
bool hasNavigationGallery(const QString& id) {
    return id == "482:335" || id == "486:10" || id == "492:10";
}

QWidget* createNavigationAppearance(const QString& id, QWidget* parent, const QString& profile) {
    auto* page = new QWidget(parent);
    auto* layout = new QVBoxLayout(page);
    auto* split = new QSplitter(page);
    layout->addWidget(split);
    auto* settings = new QWidget(split);
    settings->setMinimumWidth(230);
    auto* form = new QFormLayout(settings);
    auto* stage = new QWidget(split);
    stage->setMinimumWidth(300);
    auto* stageLayout = new QVBoxLayout(stage);
    auto* previewHost = new QWidget(stage);
    auto* previewLayout = new QVBoxLayout(previewHost);
    previewLayout->setContentsMargins(20, 24, 20, 24);
    stageLayout->addWidget(previewHost);
    stageLayout->addStretch();
    auto* events = new QLabel(QString::fromUtf8("操作预览以检查 QAction 触发与状态变化"), stage);
    events->setObjectName("navigationEventLog");
    events->setWordWrap(true);
    stageLayout->addWidget(events);
    auto* tree = new QTreeWidget(split);
    tree->setObjectName("navigationContractTree");
    tree->setHeaderLabels({QString::fromUtf8("属性 / 身份"), QString::fromUtf8("值 / 契约")});
    tree->setMinimumWidth(250);
    tree->setColumnWidth(0, 130);
    auto combo = [&](const QString& name, const QStringList& values) {
        auto* c = new QComboBox(settings);
        c->setObjectName("navigation" + name);
        c->addItems(values);
        form->addRow(name, c);
        return c;
    };
    QWidget* preview = nullptr;
    if (id == "1307:2") {
        settings->setMaximumWidth(330);
        stage->setMinimumWidth(880);
        auto* bottom = new commonWidget::Z_DesktopBottomNavigation(profile, previewHost);
        preview = bottom;
        events->setText(QString::fromUtf8("点击主导航或次导航，检查选中状态与激活信号"));
        auto* placement = combo("IconPlacement", {"Top", "Start"});
        placement->setObjectName("desktopBottomNavigationPlacementControl");
        auto* primary = new QComboBox(settings);
        primary->setObjectName("desktopBottomNavigationPrimaryControl");
        primary->addItems(bottom->primaryItems());
        form->addRow(QString::fromUtf8("主导航选中项"), primary);
        auto* secondary = new QComboBox(settings);
        secondary->setObjectName("desktopBottomNavigationSecondaryControl");
        secondary->addItems(bottom->secondaryItems());
        form->addRow(QString::fromUtf8("次导航选中项"), secondary);
        auto* focus = new QCheckBox(QString::fromUtf8("显示组件焦点指示"), settings);
        focus->setObjectName("desktopBottomNavigationFocusIndicatorControl");
        focus->setChecked(bottom->focusIndicatorEnabled());
        form->addRow(focus);
        auto* note = new QLabel(QString::fromUtf8(
            "内部组件不单独实例化。\n"
            "路由与页面切换由宿主负责。"), settings);
        note->setObjectName("desktopBottomNavigationAnatomyNote");
        note->setWordWrap(true);
        form->addRow(note);
        // The public component is 840 px wide in Figma. Keep the contract tree,
        // but place it below the controls so it cannot cover the specimen.
        tree->setParent(settings);
        tree->setMinimumWidth(0);
        tree->setMinimumHeight(240);
        form->addRow(tree);
        const auto sync = [=] {
            const QSignalBlocker a(placement), b(primary), c(secondary), d(focus);
            placement->setCurrentText(bottom->iconPlacement());
            primary->setCurrentIndex(bottom->currentIndex());
            secondary->setCurrentIndex(bottom->secondaryIndex());
            focus->setChecked(bottom->focusIndicatorEnabled());
            bottom->setFixedSize(bottom->sizeHint());
            inspect(tree, bottom, id);
        };
        QObject::connect(placement, &QComboBox::currentTextChanged, bottom,
                         [=](const QString& value) { bottom->setIconPlacement(value); sync(); });
        QObject::connect(primary, qOverload<int>(&QComboBox::currentIndexChanged), bottom,
                         [=](int index) { if (index >= 0) bottom->setCurrentIndex(index); sync(); });
        QObject::connect(secondary, qOverload<int>(&QComboBox::currentIndexChanged), bottom,
                         [=](int index) { if (index >= 0) bottom->setSecondaryIndex(index); sync(); });
        QObject::connect(focus, &QCheckBox::toggled, bottom,
                         [=](bool value) { bottom->setFocusIndicatorEnabled(value); sync(); });
        QObject::connect(bottom, &commonWidget::Z_DesktopBottomNavigation::primaryActivated,
                         page, [=](int index) {
                             events->setText(QStringLiteral("primaryActivated: %1 · %2")
                                                 .arg(index).arg(bottom->primaryItems().value(index)));
                             sync();
                         });
        QObject::connect(bottom, &commonWidget::Z_DesktopBottomNavigation::secondaryActivated,
                         page, [=](int index) {
                             events->setText(QStringLiteral("secondaryActivated: %1 · %2")
                                                 .arg(index).arg(bottom->secondaryItems().value(index)));
                             sync();
                         });
        sync();
    } else if (id == "392:2") {
        auto* sidebar = new commonWidget::Z_Sidebar(profile, previewHost);
        preview = sidebar;
        auto* density = combo("Density", {"Comfortable", "Compact"});
        auto* collapsed = new QCheckBox(QString::fromUtf8("折叠侧边栏"), settings);
        collapsed->setObjectName("sidebarCollapsedControl");
        form->addRow("Collapsed", collapsed);
        auto* active = new QComboBox(settings);
        active->setObjectName("sidebarActiveRouteControl");
        for (int index = 0; index < sidebar->routeIds().size(); ++index)
            active->addItem(sidebar->routeLabels().value(index), sidebar->routeIds().value(index));
        form->addRow(QString::fromUtf8("当前路由"), active);
        auto* focus = new QCheckBox(QString::fromUtf8("显示组件焦点指示"), settings);
        focus->setObjectName("sidebarFocusIndicatorControl");
        focus->setChecked(sidebar->focusIndicatorEnabled());
        form->addRow(focus);
        auto* note = new QLabel(QString::fromUtf8(
            "NavItem 是 Sidebar 的内部 anatomy，不创建第二个 SDK 类型；这里展示并操作公共 ComponentLab/Sidebar。"), settings);
        note->setObjectName("sidebarAnatomyNote");
        note->setWordWrap(true);
        form->addRow(note);
        const auto sync = [=] {
            const QSignalBlocker a(density), b(collapsed), c(active), d(focus);
            density->setCurrentText(sidebar->density());
            collapsed->setChecked(sidebar->isCollapsed());
            active->setCurrentIndex(active->findData(sidebar->activeRoute()));
            focus->setChecked(sidebar->focusIndicatorEnabled());
            sidebar->setFixedSize(sidebar->sizeHint());
            inspect(tree, sidebar, id);
        };
        QObject::connect(density, &QComboBox::currentTextChanged, sidebar,
                         [=](const QString& value) { sidebar->setDensity(value); sync(); });
        QObject::connect(collapsed, &QCheckBox::toggled, sidebar,
                         [=](bool value) { sidebar->setCollapsed(value); sync(); });
        QObject::connect(active, qOverload<int>(&QComboBox::currentIndexChanged), sidebar,
                         [=](int index) {
                             if (index >= 0) sidebar->setActiveRoute(active->itemData(index).toString());
                             sync();
                         });
        QObject::connect(focus, &QCheckBox::toggled, sidebar,
                         [=](bool value) { sidebar->setFocusIndicatorEnabled(value); sync(); });
        QObject::connect(sidebar, &commonWidget::Z_Sidebar::routeRequested, page,
                         [=](const QString& routeId) {
                             sidebar->setActiveRoute(routeId);
                             events->setText(QStringLiteral("routeRequested: %1").arg(routeId));
                             sync();
                         });
        QObject::connect(sidebar, &commonWidget::Z_Sidebar::collapsedChangeRequested, page,
                         [=](bool value) {
                             sidebar->setCollapsed(value);
                             events->setText(QStringLiteral("collapsedChangeRequested: %1")
                                 .arg(value ? QStringLiteral("true") : QStringLiteral("false")));
                             sync();
                         });
        sync();
    } else if (id == "482:335") {
        auto* m = new commonWidget::Z_Menu(previewHost);
        m->setWindowFlags(Qt::Widget);
        nav::populateMenu(m);
        preview = m;
        auto* item = combo("Item", {"New file", "Open…", "Auto save", "Workspace", "Recent files",
                                    "Export legacy", "Delete project"});
        auto* type = combo("Type", {"Action", "Check", "Radio", "Submenu", "Danger"});
        auto* state = combo(
            "State", {QString::fromUtf8("交互 / Auto"), "Default", "Hover", "Active", "Disabled"});
        auto* label = new QLineEdit("New file", settings);
        form->addRow("Label", label);
        auto* shortcut = new QLineEdit("Ctrl+N", settings);
        form->addRow("Shortcut", shortcut);
        auto* checked = new QCheckBox("Checked", settings);
        form->addRow(checked);
        auto* showIcon = new QCheckBox("Show icon", settings);
        showIcon->setChecked(true);
        form->addRow(showIcon);
        auto* showShortcut = new QCheckBox("Show shortcut", settings);
        showShortcut->setChecked(true);
        form->addRow(showShortcut);
        auto current = [m, item]() {
            QList<QAction*> items;
            for (auto* a : m->actions())
                if (!a->isSeparator())
                    items.append(a);
            return items.value(item->currentIndex());
        };
        auto apply = [=]() {
            auto* a = current();
            if (!a)
                return;
            a->setText(label->text());
            a->setShortcut(QKeySequence(shortcut->text()));
            a->setProperty("menuItemType", type->currentText());
            a->setCheckable(type->currentText() == "Check" || type->currentText() == "Radio");
            a->setChecked(checked->isChecked());
            a->setProperty("showIcon", showIcon->isChecked());
            a->setProperty("showShortcut", showShortcut->isChecked());
            if (type->currentText() == "Submenu" && !a->menu()) {
                auto* s = new commonWidget::Z_Menu(m);
                s->addAction("Recent file");
                a->setMenu(s);
            } else if (type->currentText() != "Submenu")
                a->setMenu(nullptr);
            nav::setPreviewState(a, state->currentIndex() ? state->currentText() : QString());
            m->update();
            inspect(tree, m, id);
        };
        QObject::connect(type, &QComboBox::currentTextChanged, page, [=] { apply(); });
        QObject::connect(state, &QComboBox::currentTextChanged, page, [=] { apply(); });
        QObject::connect(label, &QLineEdit::textEdited, page, [=] { apply(); });
        QObject::connect(shortcut, &QLineEdit::textEdited, page, [=] { apply(); });
        for (auto* c : {checked, showIcon, showShortcut})
            QObject::connect(c, &QCheckBox::clicked, page, [=] { apply(); });
        QObject::connect(item, qOverload<int>(&QComboBox::currentIndexChanged), page, [=] {
            auto* a = current();
            const QSignalBlocker bt(type), bs(state), bc(checked);
            label->setText(a->text());
            shortcut->setText(a->shortcut().toString());
            type->setCurrentText(a->property("menuItemType").toString());
            const QString previewState =
                !a->isEnabled() ? "Disabled" : a->property("figmaPreviewState").toString();
            state->setCurrentIndex(qMax(0, state->findText(previewState)));
            checked->setChecked(a->isChecked());
            showIcon->setChecked(a->property("showIcon").isNull() ||
                                 a->property("showIcon").toBool());
            showShortcut->setChecked(a->property("showShortcut").isNull() ||
                                     a->property("showShortcut").toBool());
            inspect(tree, m, id);
        });
        QObject::connect(m, &QMenu::triggered, page, [=](QAction* a) {
            events->setText("triggered: " + a->text() +
                            " · checked=" + (a->isChecked() ? "true" : "false"));
            if (a == current())
                checked->setChecked(a->isChecked());
            inspect(tree, m, id);
            // A popup should close after activation; this inline editor remains
            // on the page so users can inspect and trigger the next action.
            QTimer::singleShot(0, m, [m] { m->show(); });
        });
        m->setFixedSize(272, 303);
    } else if (id == "486:10") {
        auto* bar = new commonWidget::Z_MenuBar(previewHost);
        nav::populateMenuBar(bar);
        preview = bar;
        auto* item =
            combo("Item", {"1 · File", "2 · Edit", "3 · View", "4 · Navigate", "5 · Help"});
        auto* state = combo("State", {QString::fromUtf8("交互 / Auto"), "Default", "Hover", "Open",
                                      "Keyboard", "Disabled"});
        auto* label = new QLineEdit("File", settings);
        form->addRow("Label", label);
        auto* mnemonic = new QCheckBox("Show mnemonic", settings);
        mnemonic->setChecked(true);
        form->addRow(mnemonic);
        auto* corner = new QCheckBox("Show corner", settings);
        corner->setChecked(true);
        form->addRow(corner);
        auto* up = new QCheckBox("defaultUp", settings);
        form->addRow(up);
        auto apply = [=] {
            auto* a = bar->actions().value(item->currentIndex());
            a->setText(label->text());
            a->setProperty("showMnemonic", mnemonic->isChecked());
            nav::setPreviewState(a, state->currentIndex() ? state->currentText() : QString());
            bar->cornerWidget()->setVisible(corner->isChecked());
            bar->setDefaultUp(up->isChecked());
            bar->update();
            inspect(tree, bar, id);
        };
        QObject::connect(state, &QComboBox::currentTextChanged, page, [=] { apply(); });
        QObject::connect(label, &QLineEdit::textEdited, page, [=] { apply(); });
        for (auto* c : {mnemonic, corner, up})
            QObject::connect(c, &QCheckBox::clicked, page, [=] { apply(); });
        QObject::connect(item, qOverload<int>(&QComboBox::currentIndexChanged), page, [=] {
            auto* a = bar->actions().value(item->currentIndex());
            QSignalBlocker b(state);
            label->setText(a->text());
            mnemonic->setChecked(a->property("showMnemonic").toBool());
            state->setCurrentIndex(
                qMax(0, state->findText(a->property("figmaPreviewState").toString())));
            inspect(tree, bar, id);
        });
        QObject::connect(bar, &QMenuBar::triggered, page,
                         [=](QAction* a) { events->setText("triggered: " + a->text()); });
        bar->setFixedHeight(56);
        bar->setMinimumWidth(460);
    } else {
        auto* bar = new commonWidget::Z_ToolBar(previewHost);
        preview = bar;
        auto* orientation = combo("Orientation", {"Horizontal", "Vertical"});
        auto* placement = combo("Placement", {"Docked", "Floating"});
        auto* capacity = combo("Capacity", {"Fit", "Overflow"});
        auto* presentation = combo("Presentation", {"IconOnly", "TextBesideIcon"});
        auto* handle = new QCheckBox("Show handle", settings);
        handle->setChecked(true);
        form->addRow(handle);
        auto* note = new QLabel(
            QString::fromUtf8(
                "Placement 为 Figma 外观样例；真实 floating 是 QMainWindow 管理的只读状态。"),
            settings);
        note->setWordWrap(true);
        form->addRow(note);
        auto apply = [=] {
            nav::configureToolBar(bar, {{"Orientation", orientation->currentText()},
                                        {"Placement", placement->currentText()},
                                        {"Capacity", capacity->currentText()},
                                        {"Presentation", presentation->currentText()},
                                        {"Show handle", handle->isChecked() ? "true" : "false"}});
            inspect(tree, bar, id);
            for (auto* b : bar->findChildren<QToolButton*>())
                if (b->defaultAction())
                    QObject::connect(b, &QToolButton::clicked, page, [=] {
                        events->setText("triggered: " + b->defaultAction()->text());
                    });
        };
        for (auto* c : {orientation, placement, capacity, presentation})
            QObject::connect(c, &QComboBox::currentTextChanged, page, [=] { apply(); });
        QObject::connect(handle, &QCheckBox::clicked, page, [=] { apply(); });
        apply();
    }
    preview->setObjectName("navigationInteractivePreview");
    previewLayout->addWidget(preview, 0, Qt::AlignLeft);
    inspect(tree, preview, id);
    split->setSizes(id == "1307:2" ? QList<int>{320, 1100}
                                     : QList<int>{240, 500, 300});
    return page;
}

QWidget* createNavigationGallery(const QString& id, QWidget* parent) {
    auto* canvas = new QWidget;
    canvas->setObjectName("figmaGalleryCanvas");
    canvas->setStyleSheet("QWidget#figmaGalleryCanvas {background:white;border-radius:16px;}");
    canvas->setFixedSize(1312, id == "482:335" ? 834 : id == "486:10" ? 1026 : 1107);
    canvas->setProperty("galleryNodeId", id == "482:335"  ? "482:40"
                                         : id == "486:10" ? "486:37"
                                                          : "492:37");
    canvas->setProperty("variantSpecimenCount", id == "482:335" ? 20 : id == "486:10" ? 5 : 16);
    auto* key = panel(canvas,
                      QRect(28, 28,
                            id == "482:335"  ? 853
                            : id == "486:10" ? 893
                                             : 798,
                            39),
                      "#f7f9fc");
    if (id == "482:335") {
        text(key, QRect(12, 0, 829, 39),
             "Type · Action / Check / Radio / Submenu / Danger     State · Default / Hover / "
             "Active / Disabled     Properties · Label / Shortcut / Show shortcut");
        const QStringList types{"Action", "Check", "Radio", "Submenu", "Danger"},
            states{"Default", "Hover", "Active", "Disabled"};
        for (int row = 0; row < 5; ++row)
            for (int col = 0; col < 4; ++col) {
                auto* m =
                    menu(canvas, 52 + col * 280, 109 + row * 72, true, types[row], states[col]);
                m->setProperty("figmaVariant",
                               QString("Type=%1, State=%2").arg(types[row], states[col]));
                m->setAttribute(Qt::WA_TransparentForMouseEvents);
            }
        auto* m = menu(canvas, 28, 503);
        specimenStates(m);
    } else if (id == "486:10") {
        text(key, QRect(12, 0, 869, 39),
             "State · Default / Hover / Open / Keyboard / Disabled     Properties · Label / Show "
             "mnemonic     Composition · MenuBarItem / MenuBar / QMenu popup");
        const QStringList states{"Default", "Hover", "Open", "Keyboard", "Disabled"};
        for (int i = 0; i < 5; ++i) {
            auto* b = new commonWidget::Z_MenuBar(canvas);
            b->setProperty("figmaSingleItem", true);
            nav::install(b);
            auto* a = b->addAction("File");
            a->setProperty("showMnemonic", true);
            nav::setPreviewState(a, states[i]);
            b->setGeometry(52 + 240 * i, 109, 64, 40);
            b->setAttribute(Qt::WA_TransparentForMouseEvents);
            b->setProperty("figmaVariant", "State=" + states[i]);
        }
        menuBar(canvas, QRect(28, 191, 640, 56));
        auto* qa = panel(canvas, QRect(28, 265, 1256, 405), "#f7f9fc");
        text(qa, QRect(20, 20, 900, 15),
             QString::fromUtf8("1:1 新建实例、内容编辑与 DSM Semantic Light / Dark "
                               "颜色校验（Profile 尺寸/排印待验证）"));
        const QStringList captions{"A · baseline · 640 px",
                                   QString::fromUtf8("B · states + EN/中文 · DSM Light · 640 px"),
                                   "C · constrained · 520 px",
                                   QString::fromUtf8("D · states + EN/中文 · DSM Dark · 640 px"),
                                   "E · fresh insert · nested edits · corner off · 640 px"};
        for (int row = 0; row < 5; ++row) {
            text(qa, QRect(20, 61 + 70 * row, 240, 32), captions[row]);
            QStringList labels;
            if (row == 1 || row == 3)
                labels = QStringList{"Project Configuration", "Edit", QString::fromUtf8("设备"),
                                     "Navigate", QString::fromUtf8("帮助")};
            else if (row == 2)
                labels = QStringList{QString::fromUtf8("文件"), QString::fromUtf8("编辑"),
                                     QString::fromUtf8("视图"), QString::fromUtf8("设备配置"),
                                     QString::fromUtf8("帮助")};
            else if (row == 4)
                labels = QStringList{QString::fromUtf8("项目"), QString::fromUtf8("编辑"),
                                     QString::fromUtf8("设备管理与诊断"), "Navigate", "Help"};
            auto* b = menuBar(qa, QRect(280, 49 + 70 * row, row == 2 ? 520 : 640, 56), labels);
            if (row)
                barStates(b);
            if (row == 4)
                b->cornerWidget()->hide();
            if (row == 3)
                b->setProperty("figmaSemanticMode", "Dark");
        }
        for (int up = 0; up < 2; ++up) {
            auto* card = panel(canvas, QRect(28 + 640 * up, 688, 616, 310), "#f7f8fa");
            text(card, QRect(17, 15, 570, 13),
                 up ? "POPUP UP · defaultUp=true" : "POPUP DOWN · defaultUp=false", 11);
            auto* stage = panel(card, QRect(17, 38, 584, 250));
            auto* b = menuBar(nullptr, QRect(0, 0, 640, 56));
            barStates(b);
            b->setDefaultUp(up);
            scaled(b, stage, QRect(20, up ? 194 : 16, 512, 45), .8);
            auto* m = menu(nullptr, 0, 0);
            specimenStates(m);
            scaled(m, stage, QRect(28, up ? 18 : 65, 150, 167), .55);
        }
    } else {
        text(key, QRect(12, 0, 774, 39),
             "Axes · Orientation / Placement / Capacity / Presentation     Variants · 2 × 2 × 2 × "
             "2 = 16     Reuse · ComponentLab/ToolButton / Menu");
        const int rows[]{109, 197, 285, 537};
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 4; ++col) {
                auto* b = new commonWidget::Z_ToolBar(canvas);
                auto props = toolbarProps(row, col);
                nav::configureToolBar(b, props);
                b->move(52 + 304 * col, rows[row]);
                QStringList variant;
                for (auto it = props.cbegin(); it != props.cend(); ++it)
                    variant << it.key() + "=" + it.value();
                b->setProperty("figmaVariant", variant.join(", "));
            }
        for (int side = 0; side < 2; ++side) {
            auto* card = panel(canvas, QRect(28 + 640 * side, 799, 616, 280), "#f7f8fa");
            text(card, QRect(17, 15, 570, 13),
                 side ? "VERTICAL FLOATING · text beside icon"
                      : "DOCKED OVERFLOW · extension popup",
                 11);
            auto* stage = panel(card, QRect(17, 38, 584, 220));
            auto* b = new commonWidget::Z_ToolBar;
            nav::configureToolBar(b, toolbarProps(side ? 3 : 0, side ? 2 : 1));
            scaled(b, stage, side ? QRect(252, 10, 108, 198) : QRect(20, 18, 448, 90),
                   side ? .9 : 1.6);
            if (!side) {
                auto* m = menu(nullptr, 0, 0);
                specimenStates(m);
                scaled(m, stage, QRect(420, 83, 123, 137), .45);
            }
        }
    }
    return new FigmaGalleryView(canvas, parent);
}
} // namespace native_gallery
