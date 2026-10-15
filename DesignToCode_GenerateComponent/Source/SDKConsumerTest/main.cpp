#include <MyDesignSystem/MyDesignSystem.h>

#include <QApplication>
#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QColor>
#include <QDockWidget>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QItemSelectionModel>
#include <QListView>
#include <QMenu>
#include <QMenuBar>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QProgressBar>
#include <QStackedWidget>
#include <QWizard>
#include <QWizardPage>
#include <QSet>
#include <QStringListModel>
#include <QToolButton>
#include <QToolBar>
#include <QTabBar>
#include <QTabWidget>
#include <QToolBox>
#include <QWidget>
#include <stdexcept>

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);
    component_theme::install(application, QStringLiteral("light-comfortable"));

    QWidget owner;
    commonWidget::ComponentFactory factory(QStringLiteral("light-comfortable"));
    const auto descriptors = commonWidget::ComponentFactory::descriptors();
    QSet<QString> classes;
    for (const auto& descriptor : descriptors) {
        if (descriptor.sdkClass.isEmpty() || descriptor.designComponent.isEmpty() ||
            classes.contains(descriptor.sdkClass))
            return 2;
        classes.insert(descriptor.sdkClass);
        auto* component = factory.create(descriptor.sdkClass, &owner);
        if (!component)
            return 3;
        if (descriptor.mappingKind == QStringLiteral("visual-component")) {
            if (!descriptor.sdkClass.startsWith(QStringLiteral("commonWidget::Z_")) ||
                !component->property("designSystemNative").toBool() ||
                component->property("designSystemClass").toString() != descriptor.sdkClass ||
                component->property("nativeQtClass").toString() != descriptor.qtClass)
                return 6;
            const QString metaClass = QString::fromLatin1(component->metaObject()->className());
            if (!metaClass.endsWith(descriptor.sdkClass.mid(QStringLiteral("commonWidget::").size())))
                return 7;
        }
    }
    if (descriptors.isEmpty() || classes.size() != descriptors.size())
        return 4;

    auto* lineEdit = factory.create(QStringLiteral("commonWidget::Z_LineEdit"), &owner);
    QJsonArray calls;
    calls.append(QJsonObject{{QStringLiteral("api"), QStringLiteral("setPlaceholderText")},
                             {QStringLiteral("value"), QStringLiteral("SDK consumer")}});
    factory.applyCalls(lineEdit, calls);
    if (lineEdit->property("placeholderText").toString() != QStringLiteral("SDK consumer"))
        return 5;

    auto* chart = qobject_cast<commonWidget::Z_Chart*>(
        factory.create(QStringLiteral("commonWidget::Z_Chart"), &owner));
    if (!chart)
        return 8;
    chart->setVariantTuple(QStringLiteral("Kind=Line, State=Data"));
    if (chart->variantValue(QStringLiteral("Kind")) != QStringLiteral("Line") ||
        chart->property("figmaVariant").toString() != QStringLiteral("Kind=Line, State=Data"))
        return 9;

    auto* designSwitch = qobject_cast<commonWidget::Z_Switch*>(
        factory.create(QStringLiteral("commonWidget::Z_Switch"), &owner));
    if (!designSwitch)
        return 10;
    designSwitch->setVariantTuple(
        QStringLiteral("Size=Small, Value=On, State=Focus-visible"));
    if (!designSwitch->isChecked() ||
        designSwitch->variantProperties().value(QStringLiteral("Size")).toString() !=
            QStringLiteral("Small"))
        return 11;

    auto* focusButton = qobject_cast<commonWidget::Z_PushButton*>(
        factory.create(QStringLiteral("commonWidget::Z_PushButton"), &owner));
    auto* focusCheck = qobject_cast<commonWidget::Z_CheckBox*>(
        factory.create(QStringLiteral("commonWidget::Z_CheckBox"), &owner));
    auto* focusInput = qobject_cast<commonWidget::Z_InputField*>(
        factory.create(QStringLiteral("commonWidget::Z_InputField"), &owner));
    if (!focusButton || !focusCheck || !focusInput || !focusInput->lineEdit() ||
        focusButton->focusPolicy() != Qt::StrongFocus ||
        focusCheck->focusPolicy() != Qt::StrongFocus ||
        focusInput->focusPolicy() != Qt::NoFocus ||
        focusInput->lineEdit()->focusPolicy() == Qt::NoFocus)
        return 131;
    QString coreFocusError;
    for (auto* component : {static_cast<QWidget*>(focusButton),
                            static_cast<QWidget*>(focusCheck),
                            static_cast<QWidget*>(focusInput)})
        if (!factory.apply(component, QStringLiteral("setFocusRingEnabled"), false,
                           &coreFocusError) ||
            commonWidget::FocusIndicator::instance().enabledFor(component))
            return 132;
    if (commonWidget::FocusIndicator::instance().enabledFor(focusInput->lineEdit()) ||
        focusButton->focusPolicy() != Qt::StrongFocus ||
        focusCheck->focusPolicy() != Qt::StrongFocus ||
        focusInput->lineEdit()->focusPolicy() == Qt::NoFocus)
        return 133;
    for (auto* component : {static_cast<QWidget*>(focusButton),
                            static_cast<QWidget*>(focusCheck),
                            static_cast<QWidget*>(focusInput)})
        if (!factory.apply(component, QStringLiteral("setFocusRingEnabled"), true,
                           &coreFocusError) ||
            !commonWidget::FocusIndicator::instance().enabledFor(component))
            return 134;

    auto* displayLabel = qobject_cast<QLabel*>(
        factory.create(QStringLiteral("commonWidget::Z_Label"), &owner));
    auto* displayProgress = qobject_cast<QProgressBar*>(
        factory.create(QStringLiteral("commonWidget::Z_ProgressBar"), &owner));
    if (!displayLabel || !displayProgress)
        return 135;
    displayLabel->setText(QStringLiteral("Equipment running"));
    commonWidget::configureFigmaLabel(
        displayLabel, {{QStringLiteral("Alignment"), QStringLiteral("End")},
                       {QStringLiteral("Wrap"), QStringLiteral("True")},
                       {QStringLiteral("State"), QStringLiteral("Disabled")}});
    if (displayLabel->isEnabled() || !displayLabel->wordWrap() ||
        displayLabel->focusPolicy() != Qt::NoFocus ||
        displayLabel->property("figmaLogicalAlignment").toString() != QStringLiteral("End"))
        return 136;
    displayProgress->setRange(0, 100);
    displayProgress->setValue(68);
    commonWidget::configureFigmaProgressBar(
        displayProgress,
        {{QStringLiteral("Orientation"), QStringLiteral("Horizontal")},
         {QStringLiteral("State"), QStringLiteral("Completed")},
         {QStringLiteral("Inverted"), QStringLiteral("True")},
         {QStringLiteral("Show label"), QStringLiteral("False")}},
        true);
    if (displayProgress->value() != 68 || !displayProgress->isEnabled() ||
        !displayProgress->invertedAppearance() || !displayProgress->isTextVisible() ||
        displayProgress->format() != QStringLiteral("Completed") ||
        displayProgress->focusPolicy() != Qt::NoFocus)
        return 137;
    commonWidget::configureFigmaProgressBar(
        displayProgress,
        {{QStringLiteral("Orientation"), QStringLiteral("Horizontal")},
         {QStringLiteral("State"), QStringLiteral("Indeterminate")},
         {QStringLiteral("Inverted"), QStringLiteral("False")},
         {QStringLiteral("Show label"), QStringLiteral("True")}},
        true);
    if (displayProgress->minimum() != 0 || displayProgress->maximum() != 0 ||
        displayProgress->isTextVisible())
        return 138;

    auto* metricCard = qobject_cast<commonWidget::Z_MetricCard*>(
        factory.create(QStringLiteral("commonWidget::Z_MetricCard"), &owner));
    auto* statistic = qobject_cast<commonWidget::Z_Statistic*>(
        factory.create(QStringLiteral("commonWidget::Z_Statistic"), &owner));
    if (!metricCard || !statistic)
        return 139;
    commonWidget::configureFigmaMetricCard(
        metricCard, {{QStringLiteral("State"), QStringLiteral("Empty")}});
    metricCard->setTitleText(QStringLiteral("Yield"));
    metricCard->setValueText(QStringLiteral("99.8%"));
    metricCard->setDetailText(QStringLiteral("Last lot"));
    if (metricCard->state() != QStringLiteral("Empty") ||
        metricCard->titleText() != QStringLiteral("Yield") ||
        metricCard->detailText() != QStringLiteral("Last lot") ||
        metricCard->focusPolicy() != Qt::NoFocus || metricCard->sizeHint() != QSize(360, 176))
        return 140;
    commonWidget::configureFigmaStatistic(
        statistic,
        {{QStringLiteral("Size"), QStringLiteral("Compact")},
         {QStringLiteral("State"), QStringLiteral("Default")},
         {QStringLiteral("Show title"), QStringLiteral("True")},
         {QStringLiteral("Show prefix"), QStringLiteral("True")},
         {QStringLiteral("Show suffix"), QStringLiteral("True")}});
    statistic->setTitleText(QStringLiteral("Active users"));
    statistic->setPrefix(QStringLiteral("$"));
    statistic->setSuffix(QStringLiteral("users"));
    statistic->setLocaleName(QStringLiteral("en_US"));
    statistic->setPrecision(1);
    statistic->setNumericValue(93241.5);
    if (statistic->size() != QStringLiteral("Compact") || !statistic->hasNumericValue() ||
        statistic->numericValue() != 93241.5 || statistic->valueText() != QStringLiteral("93,241.5") ||
        statistic->accessibleDescription() != QStringLiteral("$, 93,241.5, users") ||
        statistic->focusPolicy() != Qt::NoFocus || statistic->sizeHint() != QSize(300, 150))
        return 141;
    statistic->setUseGrouping(false);
    if (statistic->numericValue() != 93241.5 || statistic->valueText() != QStringLiteral("93241.5"))
        return 142;

    auto* bottomNavigation = qobject_cast<commonWidget::Z_DesktopBottomNavigation*>(
        factory.create(QStringLiteral("commonWidget::Z_DesktopBottomNavigation"), &owner));
    if (!bottomNavigation || bottomNavigation->primaryItems().size() != 3 ||
        bottomNavigation->secondaryItems().size() != 3)
        return 12;
    if (bottomNavigation->primaryItems().at(0).size() != 2 ||
        bottomNavigation->primaryItems().at(0).at(0).unicode() != 0x9996 ||
        bottomNavigation->primaryItems().at(0).at(1).unicode() != 0x9875 ||
        bottomNavigation->secondaryItems().at(0).size() != 2 ||
        bottomNavigation->secondaryItems().at(0).at(0).unicode() != 0x6982 ||
        bottomNavigation->secondaryItems().at(0).at(1).unicode() != 0x89C8)
        return 100;
    bottomNavigation->setVariantTuple(QStringLiteral("IconPlacement=Start"));
    if (bottomNavigation->sizeHint() != QSize(840, 122))
        return 13;
    QString focusError;
    if (!factory.apply(bottomNavigation, QStringLiteral("setFocusRingEnabled"), false,
                       &focusError) ||
        commonWidget::FocusIndicator::instance().enabledFor(bottomNavigation))
        return 14;
    commonWidget::ComponentFactory darkFactory(QStringLiteral("dark-comfortable"));
    auto* darkBottomNavigation = qobject_cast<commonWidget::Z_DesktopBottomNavigation*>(
        darkFactory.create(QStringLiteral("commonWidget::Z_DesktopBottomNavigation"), &owner));
    if (!darkBottomNavigation ||
        !darkBottomNavigation->styleSheet().contains(QStringLiteral("#272727")) ||
        !darkBottomNavigation->styleSheet().contains(QStringLiteral("#f3f3f3")))
        return 99;

    auto* navbar = qobject_cast<commonWidget::Z_Navbar*>(
        factory.create(QStringLiteral("commonWidget::Z_Navbar"), &owner));
    auto* sidebar = qobject_cast<commonWidget::Z_Sidebar*>(
        factory.create(QStringLiteral("commonWidget::Z_Sidebar"), &owner));
    if (!navbar || !sidebar || navbar->focusPolicy() != Qt::NoFocus ||
        sidebar->focusPolicy() != Qt::NoFocus)
        return 93;
    navbar->setDensity(QStringLiteral("Compact"));
    navbar->setProfileName(QStringLiteral("Taylor Lee"));
    navbar->setSearchText(QStringLiteral("ETCH"));
    bool notificationRequested = false;
    bool profileRequested = false;
    QObject::connect(navbar, &commonWidget::Z_Navbar::notificationRequested,
                     [&notificationRequested] { notificationRequested = true; });
    QObject::connect(navbar, &commonWidget::Z_Navbar::profileRequested,
                     [&profileRequested] { profileRequested = true; });
    navbar->notificationButton()->click();
    navbar->profileButton()->click();
    if (navbar->density() != QStringLiteral("Compact") ||
        navbar->profileName() != QStringLiteral("Taylor Lee") ||
        navbar->searchText() != QStringLiteral("ETCH") ||
        navbar->sizeHint() != QSize(1200, 56) || !notificationRequested || !profileRequested ||
        navbar->notificationButton()->focusPolicy() != Qt::StrongFocus ||
        navbar->profileButton()->focusPolicy() != Qt::StrongFocus)
        return 94;
    navbar->setFocusIndicatorEnabled(false);
    if (commonWidget::FocusIndicator::instance().enabledFor(navbar->searchInput()->lineEdit()))
        return 95;

    sidebar->setVariantTuple(QStringLiteral("Density=Compact, Collapsed=True"));
    bool routeRequested = false;
    bool collapseRequested = false;
    QObject::connect(sidebar, &commonWidget::Z_Sidebar::routeRequested,
                     [&routeRequested](const QString& route) {
                         routeRequested = route == QStringLiteral("orders");
                     });
    QObject::connect(sidebar, &commonWidget::Z_Sidebar::collapsedChangeRequested,
                     [&collapseRequested](bool collapsed) { collapseRequested = !collapsed; });
    auto* orders = sidebar->findChild<QToolButton*>(QStringLiteral("sidebarRoute1"));
    if (!orders || sidebar->findChildren<QToolButton*>().size() != 7 ||
        sidebar->sizeHint() != QSize(80, 466) ||
        sidebar->collapseButton()->focusPolicy() != Qt::StrongFocus)
        return 96;
    orders->click();
    sidebar->collapseButton()->click();
    if (!routeRequested || !collapseRequested ||
        sidebar->activeRoute() != QStringLiteral("dashboard"))
        return 97;
    sidebar->setActiveRoute(QStringLiteral("orders"));
    if (sidebar->activeRoute() != QStringLiteral("orders") || !orders->isChecked())
        return 98;

    auto* drawer = qobject_cast<commonWidget::Z_Drawer*>(
        factory.create(QStringLiteral("commonWidget::Z_Drawer"), &owner));
    if (!drawer || drawer->componentName() != QStringLiteral("Drawer") ||
        drawer->property("designComponent").toString() != QStringLiteral("ComponentLab/Drawer") ||
        drawer->property("designSystemClass").toString() != QStringLiteral("commonWidget::Z_Drawer"))
        return 15;
    auto* legacyDrawer = qobject_cast<commonWidget::Z_OverlayDrawer*>(
        factory.create(QStringLiteral("commonWidget::Z_OverlayDrawer"), &owner));
    if (!legacyDrawer || !commonWidget::ComponentFactory::isRegistered(
            QStringLiteral("commonWidget::Z_OverlayDrawer")))
        return 16;

    const auto libraryEntries = commonWidget::designLibraryEntries();
    const auto catalogComponents = commonWidget::designComponentDescriptors();
    QSet<QString> canonicalIds;
    QSet<QString> componentIds;
    QSet<QString> layoutHelperIds;
    QSet<QString> assetIds;
    for (const auto& entry : libraryEntries) {
        if (entry.canonicalId.isEmpty() || canonicalIds.contains(entry.canonicalId))
            return 17;
        canonicalIds.insert(entry.canonicalId);
        if (entry.kind == QStringLiteral("Widget") ||
            entry.kind == QStringLiteral("Composite")) {
            componentIds.insert(entry.canonicalId);
        } else if (entry.kind == QStringLiteral("LayoutHelper")) {
            if (entry.instantiable || entry.engineeringStatus != QStringLiteral("IMPLEMENTED"))
                return 18;
            layoutHelperIds.insert(entry.canonicalId);
        } else if (entry.kind == QStringLiteral("Asset")) {
            if (entry.instantiable || entry.engineeringStatus != QStringLiteral("IMPLEMENTED"))
                return 19;
            assetIds.insert(entry.canonicalId);
        } else {
            return 20;
        }
    }
    QSet<QString> descriptorIds;
    for (const auto& descriptor : catalogComponents)
        descriptorIds.insert(descriptor.designComponent);
    if (componentIds != descriptorIds)
        return 21;
    const auto verifyCompositeFigmaIdentity = [&catalogComponents](
        const QString& canonicalId, const QString& publicNodeId,
        const QString& variantSourceId, int variantCount) {
        for (const auto& descriptor : catalogComponents) {
            if (descriptor.designComponent != canonicalId)
                continue;
            return descriptor.componentSetId.isEmpty() &&
                descriptor.publicDesignNodeId == publicNodeId &&
                descriptor.variantSourceComponentSetId == variantSourceId &&
                descriptor.variantCount == variantCount;
        }
        return false;
    };
    if (!verifyCompositeFigmaIdentity(QStringLiteral("ComponentLab/Menu"),
                                      QStringLiteral("482:491"),
                                      QStringLiteral("482:490"), 20) ||
        !verifyCompositeFigmaIdentity(QStringLiteral("ComponentLab/MenuBar"),
                                      QStringLiteral("487:23"),
                                      QStringLiteral("487:22"), 5) ||
        !verifyCompositeFigmaIdentity(QStringLiteral("ComponentLab/TabWidget"),
                                      QStringLiteral("500:849"),
                                      QStringLiteral("500:847"), 20) ||
        !verifyCompositeFigmaIdentity(QStringLiteral("ComponentLab/ToolBox"),
                                      QStringLiteral("505:555"),
                                      QStringLiteral("505:553"), 5))
        return 60;
    auto* menu = qobject_cast<commonWidget::Z_Menu*>(
        factory.create(QStringLiteral("commonWidget::Z_Menu"), &owner));
    auto* menuBar = qobject_cast<commonWidget::Z_MenuBar*>(
        factory.create(QStringLiteral("commonWidget::Z_MenuBar"), &owner));
    if (!menu || !menuBar)
        return 101;
    commonWidget::navigation::populateMenu(menu);
    if (menu->actions().size() != 8 || !menu->actions().at(2)->isSeparator() ||
        !menu->actions().at(3)->isCheckable() || !menu->actions().at(3)->isChecked() ||
        !menu->actions().at(5)->menu() || menu->actions().at(6)->isEnabled())
        return 102;
    commonWidget::navigation::populateMenuBar(menuBar);
    if (menuBar->actions().size() != 5 || !menuBar->actions().first()->menu() ||
        !menuBar->cornerWidget())
        return 103;
    commonWidget::navigation::setMenuBarCornerVisible(menuBar, false);
    if (!menuBar->cornerWidget()->isHidden() || menuBar->property("showCorner").toBool())
        return 104;
    commonWidget::navigation::setMenuBarCornerVisible(menuBar, true);
    if (menuBar->cornerWidget()->isHidden() || !menuBar->property("showCorner").toBool())
        return 105;
    QString menuFocusError;
    if (!factory.apply(menu, QStringLiteral("setFocusRingEnabled"), false,
                       &menuFocusError) ||
        commonWidget::FocusIndicator::instance().enabledFor(menu))
        return 106;
    if (!factory.apply(menuBar, QStringLiteral("setFocusRingEnabled"), false,
                       &menuFocusError) ||
        commonWidget::FocusIndicator::instance().enabledFor(menuBar))
        return 107;
    auto* toolBar = qobject_cast<commonWidget::Z_ToolBar*>(
        factory.create(QStringLiteral("commonWidget::Z_ToolBar"), &owner));
    if (!toolBar)
        return 108;
    commonWidget::navigation::configureToolBar(
        toolBar, {{QStringLiteral("Orientation"), QStringLiteral("Vertical")},
                  {QStringLiteral("Placement"), QStringLiteral("Floating")},
                  {QStringLiteral("Capacity"), QStringLiteral("Overflow")},
                  {QStringLiteral("Presentation"), QStringLiteral("TextBesideIcon")},
                  {QStringLiteral("Show handle"), QStringLiteral("false")}});
    if (toolBar->orientation() != Qt::Vertical || toolBar->size() != QSize(120, 220) ||
        toolBar->property("figmaPlacementPreview").toString() != QStringLiteral("Floating") ||
        !toolBar->graphicsEffect() ||
        toolBar->findChild<QWidget*>(QStringLiteral("figmaToolBarHandle")) ||
        !toolBar->findChild<QToolButton*>(QStringLiteral("figmaOverflowButton")))
        return 109;
    if (!factory.apply(toolBar, QStringLiteral("setFocusRingEnabled"), false,
                       &menuFocusError) ||
        commonWidget::FocusIndicator::instance().enabledFor(toolBar))
        return 110;
    auto* tabs = qobject_cast<commonWidget::Z_TabWidget*>(
        factory.create(QStringLiteral("commonWidget::Z_TabWidget"), &owner));
    auto* toolBox = qobject_cast<commonWidget::Z_ToolBox*>(
        factory.create(QStringLiteral("commonWidget::Z_ToolBox"), &owner));
    if (!tabs || !toolBox)
        return 111;
    commonWidget::populateFigmaTabWidget(tabs);
    if (tabs->count() != 4 || tabs->currentIndex() != 0 || tabs->tabText(0) != "Overview" ||
        tabs->widget(2)->objectName() != "tabDemoBehavior" || tabs->isTabEnabled(3) ||
        tabs->tabBar()->tabButton(0, QTabBar::LeftSide) ||
        tabs->tabBar()->tabButton(0, QTabBar::RightSide) ||
        (!tabs->tabBar()->tabButton(2, QTabBar::LeftSide) &&
         !tabs->tabBar()->tabButton(2, QTabBar::RightSide)))
        return 112;
    commonWidget::populateFigmaToolBox(toolBox);
    if (toolBox->count() != 3 || toolBox->currentIndex() != 1 ||
        toolBox->itemText(0) != "&General" ||
        toolBox->widget(1)->objectName() != "toolBoxDemoRules" ||
        toolBox->isItemEnabled(2))
        return 113;
    if (!factory.apply(tabs, QStringLiteral("setFocusRingEnabled"), false,
                       &menuFocusError) ||
        commonWidget::FocusIndicator::instance().enabledFor(tabs) ||
        !factory.apply(toolBox, QStringLiteral("setFocusRingEnabled"), false,
                       &menuFocusError) ||
        commonWidget::FocusIndicator::instance().enabledFor(toolBox))
        return 114;
    auto* dock = qobject_cast<commonWidget::Z_DockWidget*>(
        factory.create(QStringLiteral("commonWidget::Z_DockWidget"), &owner));
    if (!dock)
        return 115;
    const auto dockFeatures = dock->features();
    const auto dockAllowedAreas = dock->allowedAreas();
    commonWidget::configureFigmaDockWidget(
        dock, {{QStringLiteral("Area"), QStringLiteral("Bottom")},
               {QStringLiteral("Placement"), QStringLiteral("Floating")}});
    auto* dockCaption = dock->findChild<QLabel*>(QStringLiteral("dockDemoCaption"));
    if (!dockCaption || dockCaption->text() != QString::fromUtf8("Bottom dock · Floating") ||
        dock->windowTitle() != QStringLiteral("Inspector") ||
        dock->property("figmaDockAreaPreview").toString() != QStringLiteral("Bottom") ||
        dock->property("figmaDockPlacementPreview").toString() != QStringLiteral("Floating") ||
        dock->features() != dockFeatures || dock->allowedAreas() != dockAllowedAreas ||
        dock->isFloating())
        return 116;
    if (!factory.apply(dock, QStringLiteral("setFocusRingEnabled"), false,
                       &menuFocusError) ||
        commonWidget::FocusIndicator::instance().enabledFor(dock))
        return 117;
    int namedDockActions = 0;
    for (auto* action :
         dock->findChildren<QAbstractButton*>(QString(), Qt::FindDirectChildrenOnly)) {
        if (!action->inherits("QDockWidgetTitleButton"))
            continue;
        if (action->accessibleName().isEmpty())
            return 118;
        ++namedDockActions;
    }
    if (namedDockActions != 2)
        return 119;
    auto* mdiSub = qobject_cast<commonWidget::Z_MdiSubWindow*>(
        factory.create(QStringLiteral("commonWidget::Z_MdiSubWindow"), &owner));
    if (!mdiSub)
        return 120;
    const auto mdiSubFlags = mdiSub->windowFlags();
    const auto mdiSubState = mdiSub->windowState();
    const bool mdiSubEnabled = mdiSub->isEnabled();
    commonWidget::configureFigmaMdiSubWindow(
        mdiSub, {{QStringLiteral("State"), QStringLiteral("Maximized")}});
    if (mdiSub->property("figmaMdiSnapshotState").toString() !=
            QStringLiteral("Maximized") ||
        mdiSub->windowFlags() != mdiSubFlags || mdiSub->windowState() != mdiSubState ||
        mdiSub->isEnabled() != mdiSubEnabled || !mdiSub->widget() ||
        mdiSub->widget()->objectName() != QStringLiteral("mdiDocument"))
        return 121;
    auto* mdiArea = qobject_cast<commonWidget::Z_MdiArea*>(
        factory.create(QStringLiteral("commonWidget::Z_MdiArea"), &owner));
    if (!mdiArea)
        return 122;
    const auto activationOrder = mdiArea->activationOrder();
    const bool tabsMovable = mdiArea->tabsMovable();
    commonWidget::configureFigmaMdiArea(
        mdiArea, {{QStringLiteral("Presentation"), QStringLiteral("Free")}});
    if (mdiArea->property("figmaMdiPresentationPreview").toString() !=
            QStringLiteral("Free") ||
        mdiArea->viewMode() != QMdiArea::SubWindowView ||
        mdiArea->subWindowList().size() != 2 ||
        mdiArea->activationOrder() != activationOrder || mdiArea->tabsMovable() != tabsMovable ||
        mdiArea->subWindowList().first()->windowTitle() != QStringLiteral("Trend"))
        return 123;
    commonWidget::configureFigmaMdiArea(
        mdiArea, {{QStringLiteral("Presentation"), QStringLiteral("Tabbed")}});
    if (mdiArea->viewMode() != QMdiArea::TabbedView ||
        mdiArea->subWindowList().size() != 3 || !mdiArea->tabsClosable())
        return 124;
    if (!factory.apply(mdiSub, QStringLiteral("setFocusRingEnabled"), false,
                       &menuFocusError) ||
        commonWidget::FocusIndicator::instance().enabledFor(mdiSub) ||
        !factory.apply(mdiArea, QStringLiteral("setFocusRingEnabled"), false,
                       &menuFocusError) ||
        commonWidget::FocusIndicator::instance().enabledFor(mdiArea))
        return 125;
    auto* flowStack = qobject_cast<commonWidget::Z_StackedWidget*>(
        factory.create(QStringLiteral("commonWidget::Z_StackedWidget"), &owner));
    auto* flowPage = qobject_cast<commonWidget::Z_WizardPage*>(
        factory.create(QStringLiteral("commonWidget::Z_WizardPage"), &owner));
    auto* flowWizard = qobject_cast<commonWidget::Z_Wizard*>(
        factory.create(QStringLiteral("commonWidget::Z_Wizard"), &owner));
    if (!flowStack || !flowPage || !flowWizard)
        return 126;
    commonWidget::configureFigmaStackedWidget(
        flowStack, {{QStringLiteral("State"), QStringLiteral("Disabled")}});
    if (flowStack->count() != 1 || flowStack->isEnabled() ||
        flowStack->property("figmaFlowStatePreview").toString() != QStringLiteral("Disabled"))
        return 127;
    commonWidget::configureFigmaWizardPage(
        flowPage, {{QStringLiteral("Kind"), QStringLiteral("Commit")},
                   {QStringLiteral("Completion"), QStringLiteral("Incomplete")}});
    // isFinalPage() is contextual: an unattached page is also the terminal page.
    // Verify the explicit Figma kind metadata plus the writable commit flag instead.
    if (!flowPage->isCommitPage() || !flowPage->isComplete() ||
        flowPage->property("figmaFlowKindPreview").toString() != QStringLiteral("Commit") ||
        flowPage->property("figmaFlowCompletionPreview").toString() !=
            QStringLiteral("Incomplete"))
        return 128;
    commonWidget::configureFigmaWizard(
        flowWizard, {{QStringLiteral("Style"), QStringLiteral("Modern")},
                     {QStringLiteral("Phase"), QStringLiteral("Finish")}});
    if (flowWizard->wizardStyle() != QWizard::ModernStyle || flowWizard->currentId() != 0 ||
        flowWizard->pageIds().size() != 4 || !flowWizard->page(2)->isCommitPage() ||
        !flowWizard->page(3)->isFinalPage() ||
        flowWizard->property("figmaFlowPhasePreview").toString() != QStringLiteral("Finish"))
        return 129;
    for (auto* flow : {static_cast<QWidget*>(flowStack), static_cast<QWidget*>(flowPage),
                       static_cast<QWidget*>(flowWizard)})
        if (!factory.apply(flow, QStringLiteral("setFocusRingEnabled"), false,
                           &menuFocusError) ||
            commonWidget::FocusIndicator::instance().enabledFor(flow))
            return 130;
    if (layoutHelperIds != QSet<QString>{QStringLiteral("ComponentLab/Spacer"),
                                         QStringLiteral("ComponentLab/LayoutContainer")} ||
        assetIds != QSet<QString>{QStringLiteral("ComponentLab/IconAsset"),
                                  QStringLiteral("ComponentLab/ImageAsset"),
                                  QStringLiteral("ComponentLab/ModelAsset")})
        return 22;

    const auto iconAssets = commonWidget::IconAssetCatalog::descriptors();
    if (iconAssets.size() != 7 ||
        commonWidget::IconAssetCatalog::resolve(QStringLiteral("icon.action.search")) !=
            QStringLiteral(":/icons/search.svg") ||
        !commonWidget::IconAssetCatalog::contains(QStringLiteral("icon.action.previous")) ||
        commonWidget::IconAssetCatalog::descriptor(QStringLiteral("icon.action.search"))
                .supportedSizes != QList<int>{16, 20, 24})
        return 46;
    bool unknownIconRejected = false;
    try {
        commonWidget::IconAssetCatalog::resolve(QStringLiteral("icon.action.unknown"));
    } catch (const std::invalid_argument& error) {
        unknownIconRejected = QString::fromStdString(error.what()).startsWith(
            QStringLiteral("UNMAPPED_RESOURCE:"));
    }
    if (!unknownIconRejected)
        return 47;

    const auto visualAssets = commonWidget::VisualAssetCatalog::descriptors();
    if (visualAssets.size() != 2 ||
        commonWidget::VisualAssetCatalog::imageDescriptors().size() != 1 ||
        commonWidget::VisualAssetCatalog::modelDescriptors().size() != 1 ||
        commonWidget::VisualAssetCatalog::resolve(
            QStringLiteral("image.foundation.geometric-preview")) !=
            QStringLiteral(":/assets/images/geometric-preview.svg") ||
        commonWidget::VisualAssetCatalog::resolve(
            QStringLiteral("model.foundation.schematic"), QStringLiteral("Error")) !=
            QStringLiteral(":/assets/models/foundation-schematic-error.svg") ||
        commonWidget::VisualAssetCatalog::sha256(
            QStringLiteral("model.foundation.schematic"), QStringLiteral("Ready")).size() != 64)
        return 48;
    const auto imagePreview = commonWidget::VisualAssetCatalog::render(
        QStringLiteral("image.foundation.geometric-preview"), QStringLiteral("Loaded"),
        QSize(230, 145), QStringLiteral("Contain"), QStringLiteral("light-comfortable"), 2.0);
    const auto modelPreview = commonWidget::VisualAssetCatalog::render(
        QStringLiteral("model.foundation.schematic"), QStringLiteral("Loading"),
        QSize(310, 156), QStringLiteral("Contain"), QStringLiteral("dark-compact"));
    if (imagePreview.isNull() || imagePreview.size() != QSize(460, 290) ||
        !qFuzzyCompare(imagePreview.devicePixelRatio(), 2.0) || modelPreview.isNull() ||
        modelPreview.size() != QSize(310, 156))
        return 49;
    bool noPayloadRejected = false;
    try {
        commonWidget::VisualAssetCatalog::resolve(
            QStringLiteral("image.foundation.geometric-preview"), QStringLiteral("Empty"));
    } catch (const std::invalid_argument& error) {
        noPayloadRejected = QString::fromStdString(error.what()).startsWith(
            QStringLiteral("NO_ASSET_PAYLOAD:"));
    }
    bool unknownVisualRejected = false;
    try {
        commonWidget::VisualAssetCatalog::resolve(QStringLiteral("image.unknown"));
    } catch (const std::invalid_argument& error) {
        unknownVisualRejected = QString::fromStdString(error.what()).startsWith(
            QStringLiteral("UNMAPPED_RESOURCE:"));
    }
    if (!noPayloadRejected || !unknownVisualRejected)
        return 50;

    for (const auto& descriptor : catalogComponents) {
        if (descriptor.instantiable &&
            !commonWidget::ComponentFactory::isRegistered(descriptor.sdkClass))
            return 23;
    }

    auto* badge = qobject_cast<commonWidget::Z_Badge*>(
        factory.create(QStringLiteral("commonWidget::Z_Badge"), &owner));
    auto* indicator = qobject_cast<commonWidget::Z_StatusIndicator*>(
        factory.create(QStringLiteral("commonWidget::Z_StatusIndicator"), &owner));
    auto* link = qobject_cast<commonWidget::Z_Link*>(
        factory.create(QStringLiteral("commonWidget::Z_Link"), &owner));
    auto* tag = qobject_cast<commonWidget::Z_Tag*>(
        factory.create(QStringLiteral("commonWidget::Z_Tag"), &owner));
    auto* tooltip = qobject_cast<commonWidget::Z_Tooltip*>(
        factory.create(QStringLiteral("commonWidget::Z_Tooltip"), &owner));
    if (!badge || !indicator || !link || !tag || !tooltip)
        return 24;
    badge->setKind(QStringLiteral("Dot"));
    badge->setTone(QStringLiteral("Danger"));
    indicator->setSize(QStringLiteral("Small"));
    indicator->setTone(QStringLiteral("Process"));
    indicator->setShowLabel(false);
    tag->setDismissible(true);
    bool dismissed = false;
    QObject::connect(tag, &commonWidget::Z_Tag::dismissed, [&dismissed] { dismissed = true; });
    auto* dismissButton = tag->findChild<QToolButton*>();
    if (badge->focusPolicy() != Qt::NoFocus || indicator->focusPolicy() != Qt::NoFocus ||
        badge->sizeHint() != QSize(8, 8) || indicator->showLabel() || !dismissButton)
        return 25;
    dismissButton->click();
    if (!dismissed || tag->focusPolicy() != Qt::NoFocus ||
        dismissButton->focusPolicy() != Qt::StrongFocus)
        return 26;
    bool activated = false;
    QObject::connect(link, &commonWidget::Z_Link::activated,
                     [&activated](const QUrl&) { activated = true; });
    link->setTarget(QUrl(QStringLiteral("https://example.invalid/details")));
    link->click();
    if (!activated || link->focusPolicy() != Qt::StrongFocus)
        return 27;
    activated = false;
    link->setSnapshotState(QStringLiteral("Disabled"));
    link->click();
    if (activated || link->isEnabled() ||
        link->snapshotState() != QStringLiteral("Disabled"))
        return 90;
    link->setSnapshotState(QString());
    link->click();
    if (!activated || !link->isEnabled() || !link->snapshotState().isEmpty())
        return 91;
    try {
        link->setSnapshotState(QStringLiteral("Selected"));
        return 92;
    } catch (const std::invalid_argument&) {
    }
    commonWidget::FocusIndicator::instance().setEnabledFor(link, false);
    if (commonWidget::FocusIndicator::instance().enabledFor(link))
        return 28;
    tooltip->setPlacement(QStringLiteral("Right"));
    tooltip->attachTo(link);
    if (tooltip->targetWidget() != link || tooltip->focusPolicy() != Qt::NoFocus)
        return 29;
    tooltip->detach();

    auto* search = qobject_cast<commonWidget::Z_SearchInput*>(
        factory.create(QStringLiteral("commonWidget::Z_SearchInput"), &owner));
    auto* number = qobject_cast<commonWidget::Z_NumberInput*>(
        factory.create(QStringLiteral("commonWidget::Z_NumberInput"), &owner));
    auto* overflow = qobject_cast<commonWidget::Z_OverflowMenuButton*>(
        factory.create(QStringLiteral("commonWidget::Z_OverflowMenuButton"), &owner));
    auto* segmented = qobject_cast<commonWidget::Z_SegmentedControl*>(
        factory.create(QStringLiteral("commonWidget::Z_SegmentedControl"), &owner));
    if (!search || !number || !overflow || !segmented)
        return 30;
    search->setValue(QStringLiteral("ETCH-01"));
    search->setSnapshotState(QStringLiteral("Filled"));
    if (search->value() != QStringLiteral("ETCH-01") || search->focusPolicy() != Qt::NoFocus ||
        !search->lineEdit() || search->lineEdit()->focusPolicy() == Qt::NoFocus)
        return 31;
    number->setRange(-10.0, 20.0);
    number->setType(QStringLiteral("Decimal"));
    number->setSingleStep(0.25);
    number->setValue(12.5);
    if (number->type() != QStringLiteral("Decimal") ||
        !qFuzzyCompare(number->value() + 1.0, 13.5) || number->focusPolicy() != Qt::NoFocus ||
        !number->editor() || number->editor()->focusPolicy() != Qt::StrongFocus)
        return 32;
    bool overflowClicked = false;
    QObject::connect(overflow, &QToolButton::clicked,
                     [&overflowClicked] { overflowClicked = true; });
    overflow->click();
    if (!overflowClicked || overflow->focusPolicy() != Qt::StrongFocus)
        return 33;
    segmented->setSegmentCount(4);
    segmented->setCurrentIndex(2);
    if (segmented->segmentCount() != 4 || segmented->currentIndex() != 2 ||
        segmented->currentText() != QStringLiteral("History") ||
        segmented->focusPolicy() != Qt::NoFocus)
        return 34;
    const auto segmentButtons = segmented->findChildren<QToolButton*>();
    if (segmentButtons.size() != 4)
        return 35;
    for (auto* segment : segmentButtons)
        if (segment->focusPolicy() != Qt::StrongFocus)
            return 36;

    auto* stepper = qobject_cast<commonWidget::Z_Stepper*>(
        factory.create(QStringLiteral("commonWidget::Z_Stepper"), &owner));
    auto* timeline = qobject_cast<commonWidget::Z_Timeline*>(
        factory.create(QStringLiteral("commonWidget::Z_Timeline"), &owner));
    auto* anchor = qobject_cast<commonWidget::Z_AnchorNavigation*>(
        factory.create(QStringLiteral("commonWidget::Z_AnchorNavigation"), &owner));
    auto* filter = qobject_cast<commonWidget::Z_Filter*>(
        factory.create(QStringLiteral("commonWidget::Z_Filter"), &owner));
    auto* colorPicker = qobject_cast<commonWidget::Z_ColorPicker*>(
        factory.create(QStringLiteral("commonWidget::Z_ColorPicker"), &owner));
    auto* transfer = qobject_cast<commonWidget::Z_Transfer*>(
        factory.create(QStringLiteral("commonWidget::Z_Transfer"), &owner));
    auto* upload = qobject_cast<commonWidget::Z_Upload*>(
        factory.create(QStringLiteral("commonWidget::Z_Upload"), &owner));
    auto* numericKeypad = qobject_cast<commonWidget::Z_NumericKeypad*>(
        factory.create(QStringLiteral("commonWidget::Z_NumericKeypad"), &owner));
    auto* pagination = qobject_cast<commonWidget::Z_Pagination*>(
        factory.create(QStringLiteral("commonWidget::Z_Pagination"), &owner));
    auto* pageHeader = qobject_cast<commonWidget::Z_PageHeader*>(
        factory.create(QStringLiteral("commonWidget::Z_PageHeader"), &owner));
    auto* titleBar = qobject_cast<commonWidget::Z_TitleBar*>(
        factory.create(QStringLiteral("commonWidget::Z_TitleBar"), &owner));
    if (!stepper || !timeline || !anchor || !filter || !colorPicker || !transfer || !upload || !numericKeypad ||
        !pagination || !pageHeader || !titleBar)
        return 37;
    stepper->setCurrentIndex(2);
    stepper->setSnapshotState(QStringLiteral("Completed"));
    if (stepper->currentIndex() != 2 || stepper->snapshotState() != QStringLiteral("Completed") ||
        stepper->focusPolicy() != Qt::NoFocus)
        return 38;
    timeline->setDensity(QStringLiteral("Compact"));
    timeline->setEvents({QStringLiteral("Queued"), QStringLiteral("Running"),
                         QStringLiteral("Completed")},
                        {QStringLiteral("09:10"), QStringLiteral("09:20"),
                         QStringLiteral("10:05")},
                        {QStringLiteral("Brand"), QStringLiteral("Process"),
                         QStringLiteral("Success")});
    if (timeline->density() != QStringLiteral("Compact") ||
        timeline->sizeHint() != QSize(420, 190) ||
        timeline->eventLabels().last() != QStringLiteral("Completed") ||
        timeline->focusPolicy() != Qt::NoFocus ||
        !timeline->accessibleDescription().contains(QStringLiteral("Completed")))
        return 46;
    anchor->setOrientation(QStringLiteral("Horizontal"));
    anchor->setSnapshotState(QStringLiteral("Scrolled"));
    bool anchorRequested = false;
    QObject::connect(anchor, &commonWidget::Z_AnchorNavigation::anchorActivated,
                     [&anchorRequested](int index, const QString& text) {
                         anchorRequested = index == 2 && text == QStringLiteral("Results");
                     });
    const auto anchorButtons = anchor->findChildren<QToolButton*>();
    if (anchorButtons.size() != 3)
        return 47;
    anchorButtons[2]->click();
    if (!anchorRequested || anchor->orientation() != QStringLiteral("Horizontal") ||
        anchor->snapshotState() != QStringLiteral("Scrolled") ||
        anchor->currentIndex() != 1 || anchor->sizeHint() != QSize(520, 64) ||
        anchor->focusPolicy() != Qt::NoFocus)
        return 48;
    for (auto* anchorButton : anchorButtons)
        if (anchorButton->focusPolicy() != Qt::StrongFocus)
            return 49;
    filter->setLayoutMode(QStringLiteral("Panel"));
    filter->setSnapshotState(QStringLiteral("Applied"));
    filter->setCriteria({QStringLiteral("Material: Si"), QStringLiteral("Status")});
    bool filterCriterionRequested = false;
    bool filterApplyRequested = false;
    QObject::connect(filter, &commonWidget::Z_Filter::criterionActivated,
                     [&filterCriterionRequested](int index, const QString& text) {
                         filterCriterionRequested = index == 0 && text == QStringLiteral("Material: Si");
                     });
    QObject::connect(filter, &commonWidget::Z_Filter::applyRequested,
                     [&filterApplyRequested] { filterApplyRequested = true; });
    const auto filterButtons = filter->findChildren<QToolButton*>();
    if (filterButtons.size() != 3 || filter->sizeHint() != QSize(430, 112) ||
        filter->focusPolicy() != Qt::NoFocus)
        return 70;
    filter->findChild<QToolButton*>(QStringLiteral("filterCriterion0"))->click();
    filter->findChild<QToolButton*>(QStringLiteral("filterApply"))->click();
    if (!filterCriterionRequested || !filterApplyRequested ||
        filter->snapshotState() != QStringLiteral("Applied"))
        return 71;
    for (auto* filterButton : filterButtons)
        if (filterButton->focusPolicy() != Qt::StrongFocus)
            return 72;
    bool pickerRequested = false;
    QObject::connect(colorPicker, &commonWidget::Z_ColorPicker::pickerRequested,
                     [&pickerRequested](const QColor& color) {
                         pickerRequested = color == QColor(QStringLiteral("#5B5BD6"));
                     });
    colorPicker->click();
    colorPicker->setColor(QColor(0x12, 0x34, 0x56, 0x80));
    colorPicker->setColorFormat(QStringLiteral("HexArgb"));
    if (!pickerRequested || colorPicker->colorText() != QStringLiteral("#80123456") ||
        colorPicker->sizeHint() != QSize(300, 40) ||
        colorPicker->focusPolicy() != Qt::StrongFocus)
        return 77;
    colorPicker->setSnapshotState(QStringLiteral("Disabled"));
    if (colorPicker->isEnabled())
        return 78;
    colorPicker->setSnapshotState(QStringLiteral("Default"));
    if (!colorPicker->isEnabled())
        return 79;
    QStringListModel availableModel(
        {QStringLiteral("Chamber A"), QStringLiteral("Chamber B"),
         QStringLiteral("Chamber C")});
    QStringListModel selectedModel({QStringLiteral("Chamber D")});
    transfer->setAvailableModel(&availableModel);
    transfer->setSelectedModel(&selectedModel);
    transfer->availableView()->selectionModel()->select(
        availableModel.index(1, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
    bool moveToSelectedRequested = false;
    QObject::connect(transfer, &commonWidget::Z_Transfer::moveToSelectedRequested,
                     [&moveToSelectedRequested](const QModelIndexList& rows) {
                         moveToSelectedRequested = rows.size() == 1 && rows.first().row() == 1;
                     });
    transfer->moveToSelectedButton()->click();
    if (!moveToSelectedRequested || transfer->availableModel() != &availableModel ||
        transfer->selectedModel() != &selectedModel ||
        transfer->sizeHint() != QSize(520, 210) ||
        transfer->focusPolicy() != Qt::NoFocus)
        return 82;
    if (transfer->availableView()->focusPolicy() != Qt::StrongFocus ||
        transfer->selectedView()->focusPolicy() != Qt::StrongFocus ||
        transfer->moveToSelectedButton()->focusPolicy() != Qt::StrongFocus ||
        transfer->moveToAvailableButton()->focusPolicy() != Qt::StrongFocus)
        return 83;
    transfer->setSnapshotState(QStringLiteral("Loading"));
    if (!transfer->isEnabled() || transfer->snapshotState() != QStringLiteral("Loading"))
        return 84;
    transfer->setMoveToSelectedAllowed(false);
    if (transfer->moveToSelectedButton()->isEnabled())
        return 85;
    transfer->setSnapshotState(QStringLiteral("Disabled"));
    if (transfer->isEnabled())
        return 86;
    commonWidget::FocusIndicator::instance().setEnabledFor(transfer, false);
    if (commonWidget::FocusIndicator::instance().enabledFor(transfer->availableView()))
        return 87;
    bool uploadBrowseRequested = false;
    QObject::connect(upload, &commonWidget::Z_Upload::browseRequested,
                     [&uploadBrowseRequested] { uploadBrowseRequested = true; });
    upload->click();
    upload->setFileName(QStringLiteral("recipe"));
    upload->setProgress(48);
    upload->setSnapshotState(QStringLiteral("Uploading"));
    if (!uploadBrowseRequested || upload->fileName() != QStringLiteral("recipe") ||
        upload->progress() != 48 || upload->snapshotState() != QStringLiteral("Uploading") ||
        upload->sizeHint() != QSize(390, 150) ||
        upload->focusPolicy() != Qt::StrongFocus)
        return 80;
    uploadBrowseRequested = false;
    upload->click();
    if (uploadBrowseRequested)
        return 81;
    numericKeypad->setDensity(QStringLiteral("Compact"));
    numericKeypad->setDecimalSeparator(QStringLiteral(","));
    bool numericKeyRequested = false;
    bool numericCommitRequested = false;
    QObject::connect(numericKeypad, &commonWidget::Z_NumericKeypad::keyPressed,
                     [&numericKeyRequested](const QString& key) {
                         numericKeyRequested = key == QStringLiteral("0");
                     });
    QObject::connect(numericKeypad, &commonWidget::Z_NumericKeypad::commitRequested,
                     [&numericCommitRequested] { numericCommitRequested = true; });
    const auto keypadButtons = numericKeypad->findChildren<QToolButton*>();
    if (numericKeypad->density() != QStringLiteral("Compact") ||
        numericKeypad->decimalSeparator() != QStringLiteral(",") ||
        numericKeypad->sizeHint() != QSize(260, 250) ||
        numericKeypad->focusPolicy() != Qt::NoFocus || keypadButtons.size() != 12)
        return 73;
    numericKeypad->findChild<QToolButton*>(QStringLiteral("numericKeypadKey10"))->click();
    if (!numericKeyRequested)
        return 74;
    for (auto* keypadButton : keypadButtons)
        if (keypadButton->focusPolicy() != Qt::StrongFocus)
            return 75;
    auto keypadLayout = numericKeypad->keyLayout();
    keypadLayout[11] = QStringLiteral("commit");
    numericKeypad->setKeyLayout(keypadLayout);
    numericKeypad->findChild<QToolButton*>(QStringLiteral("numericKeypadKey11"))->click();
    if (!numericCommitRequested)
        return 76;
    pagination->setPageCount(7);
    pagination->setCurrentPage(4);
    pagination->setDensity(QStringLiteral("Compact"));
    const auto paginationButtons = pagination->findChildren<QToolButton*>();
    if (pagination->pageCount() != 7 || pagination->currentPage() != 4 ||
        pagination->focusPolicy() != Qt::NoFocus || paginationButtons.size() != 5)
        return 39;
    for (auto* pageButton : paginationButtons)
        if (pageButton->focusPolicy() != Qt::StrongFocus)
            return 40;
    bool headerAction = false;
    QObject::connect(pageHeader, &commonWidget::Z_PageHeader::actionTriggered,
                     [&headerAction] { headerAction = true; });
    pageHeader->actionButton()->click();
    pageHeader->setActionsVisible(false);
    if (!headerAction || pageHeader->actionsVisible() ||
        pageHeader->focusPolicy() != Qt::NoFocus ||
        pageHeader->actionButton()->focusPolicy() != Qt::StrongFocus)
        return 41;
    bool maximizeRequested = false;
    QObject::connect(titleBar, &commonWidget::Z_TitleBar::maximizeRestoreRequested,
                     [&maximizeRequested](bool maximize) { maximizeRequested = maximize; });
    auto* maximizeButton = titleBar->findChild<QToolButton*>(
        QStringLiteral("maximizeRestoreWindow"));
    if (!maximizeButton)
        return 42;
    maximizeButton->click();
    if (!maximizeRequested || !titleBar->isMaximizedState() ||
        titleBar->focusPolicy() != Qt::NoFocus)
        return 43;
    const auto titleButtons = titleBar->findChildren<QToolButton*>();
    if (titleButtons.size() != 3)
        return 44;
    for (auto* titleButton : titleButtons)
        if (titleButton->focusPolicy() != Qt::StrongFocus)
            return 45;
    return 0;
}

