#include "QtMapping.h"
#include "MappingGenerated.h"
#include "MappingPage.h"
#include "MappingGuide.h"
#include "DesignComponentCatalog.h"
#include "ComponentTheme.h"
#include "ProfileContext.h"
#include "Typography.h"
#include "Z_DesignComponents.h"
#include "Z_CheckBox.h"
#include "Z_InputField.h"
#include "Z_Select.h"
#include "RadioAppearance.h"
#include "SpinBoxAppearance.h"
#include "DateTimeAppearance.h"
#include "SliderAppearance.h"
#include "native/Z_Slider.h"
#include "ScrollBarAppearance.h"
#include "DialAppearance.h"
#include "LineEditAppearance.h"
#include "ShortcutAppearance.h"
#include "ComboBoxAppearance.h"
#include "CommandLinkAppearance.h"
#include "ToolButtonAppearance.h"
#include "DialogButtonBoxAppearance.h"
#include "TabAppearance.h"
#include "ToolBoxAppearance.h"
#include "DockWidgetAppearance.h"
#include "MdiAppearance.h"
#include "native/Z_MdiArea.h"
#include "native/Z_MdiSubWindow.h"
#include <QStyleOptionTitleBar>
#include <QCloseEvent>
#include "native/Z_DockWidget.h"
#include <QMainWindow>
#include "FocusIndicator.h"
#include "native/Z_ToolBox.h"
#include "native/Z_TabWidget.h"
#include <QTabBar>
#include "native/Z_DialogButtonBox.h"
#include <QDialog>
#include "native/Z_ToolButton.h"
#include <QStyleOptionToolButton>
#include <QMenu>
#include <QTimer>
#include "native/Z_CommandLinkButton.h"
#include <QButtonGroup>
#include "native/Z_ComboBox.h"
#include "native/Z_FontComboBox.h"
#include <QCompleter>
#include <QFontDatabase>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QAbstractItemView>
#include <QHeaderView>
#include "native/Z_KeySequenceEdit.h"
#include "native/Z_LineEdit.h"
#include <QInputMethodEvent>
#include <QAction>
#include <QIntValidator>
#include <QToolButton>
#include "native/Z_Dial.h"
#include "native/Z_ScrollBar.h"
#include <QScrollArea>
#include <QStyleOptionSlider>
#include <QMouseEvent>
#include "native/Z_DateEdit.h"
#include "native/Z_TimeEdit.h"
#include "native/Z_DateTimeEdit.h"
#include <QCalendarWidget>
#include <QStyleOptionComboBox>
#include "native/Z_SpinBox.h"
#include "native/Z_DoubleSpinBox.h"
#include <QStyleOptionSpinBox>
#include <QFontInfo>
#include <QRawFont>
#include <QAccessible>
#include "native/Z_RadioButton.h"
#include <QApplication>
#include <QBoxLayout>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QFile>
#include <QPushButton>
#include <QSignalSpy>
#include <QStyleFactory>
#include <QTemporaryDir>
#include <QTableWidget>
#include <QTabWidget>
#include <QTest>
#include <QTreeWidget>
#include <QXmlStreamReader>
#include <QSvgRenderer>
using namespace design_mapping;
using namespace commonWidget;
namespace {
QJsonObject mutate(const QString& id, const std::function<void(QJsonObject&)>& edit) {
    auto contract = example();
    std::function<QJsonObject(QJsonObject)> walk = [&](QJsonObject node) {
        if (node["id"] == id)
            edit(node);
        if (node.contains("children")) {
            QJsonArray children;
            for (auto child : node["children"].toArray())
                children.append(walk(child.toObject()));
            node["children"] = children;
        }
        return node;
    };
    contract["page"] = walk(contract["page"].toObject());
    return contract;
}
} // namespace
class MappingTests : public QObject {
    Q_OBJECT
    QString profile = qEnvironmentVariable("DEMO_PROFILE", "light-comfortable");
  private slots:
    void themePublishesProfileAndHarmonyTypographyWithoutReverseDependency() {
        QCOMPARE(component_theme::activeProfile(), profile);
        QCOMPARE(profile_context::activeProfile(), profile);
        QObject parent;
        QObject child(&parent);
        QCOMPARE(profile_context::resolve(&child), profile);
        parent.setProperty("designSystemProfile", "dark-compact");
        QCOMPARE(profile_context::resolve(&child), QStringLiteral("dark-compact"));

        QCOMPARE(typography::globalFamily(), QStringLiteral("HarmonyOS Sans"));
        QCOMPARE(typography::simplifiedChineseFamily(), QStringLiteral("HarmonyOS Sans SC"));
        QCOMPARE(QApplication::font().families(), typography::applicationFamilies());
        const QRawFont global = QRawFont::fromFont(QFont(typography::globalFamily()));
        const QRawFont chinese =
            QRawFont::fromFont(QFont(typography::simplifiedChineseFamily()));
        QVERIFY(global.isValid());
        QVERIFY(chinese.isValid());
        QVERIFY(global.supportsCharacter(QLatin1Char('A')));
        QVERIFY(chinese.supportsCharacter(QChar(0x7EC4))); // 组
    }

    void capturedMdiRetainsNativeStateOwnershipAndCloseVeto() {
        qRegisterMetaType<QMdiSubWindow*>("QMdiSubWindow*");qRegisterMetaType<Qt::WindowStates>("Qt::WindowStates");
        const auto click=[](QMdiSubWindow* sub,const QPoint& point){QMouseEvent hover(QEvent::MouseMove,QPointF(point),QPointF(sub->mapToGlobal(point)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(sub,&hover);QTest::mouseClick(sub,Qt::LeftButton,Qt::NoModifier,point);};
        for(const auto* name:{"Minus","Maximize","Restore","Close","MinusDisabled","MaximizeDisabled","CloseDisabled"}){QSvgRenderer svg(QString(":/figma/mdi/%1.svg").arg(name));QVERIFY(svg.isValid());QCOMPARE(svg.defaultSize(),QSize(16,16));}
        class VetoDocument:public QWidget {public:bool veto=true;protected:void closeEvent(QCloseEvent* e)override{if(veto)e->ignore();else QWidget::closeEvent(e);}};
        Z_MdiArea area;area.resize(800,560);auto* first=new Z_MdiSubWindow;auto* child=new VetoDocument;first->setWidget(child);first->setWindowTitle("Trend");first->setAttribute(Qt::WA_DeleteOnClose,false);area.addSubWindow(first);first->setGeometry(20,20,280,170);
        auto* second=new Z_MdiSubWindow;second->setWidget(new QLineEdit("Retained"));second->setWindowTitle("Recipe");area.addSubWindow(second);second->setGeometry(330,40,280,170);first->show();second->show();area.show();area.setActiveSubWindow(second);QTest::qWait(40);
        const auto flags=first->windowFlags();const auto geom=first->geometry();auto* layout=first->layout();auto* menu=first->systemMenu();QSignalSpy activations(&area,&QMdiArea::subWindowActivated);QSignalSpy states(first,&QMdiSubWindow::windowStateChanged);
        applyFigmaMdiAreaAppearance(&area);applyFigmaMdiSubWindowAppearance(first,"Maximized",false,false,false);applyFigmaMdiSubWindowAppearance(second);
        QCOMPARE(first->windowFlags(),flags);QCOMPARE(first->geometry(),geom);QCOMPARE(first->layout(),layout);QCOMPARE(first->widget(),child);QCOMPARE(first->systemMenu(),menu);QCOMPARE(area.activeSubWindow(),second);QVERIFY(!first->isMaximized());QVERIFY(first->isEnabled());QCOMPARE(activations.count(),0);QCOMPARE(states.count(),0);
        applyFigmaMdiSubWindowAppearance(first);QTest::qWait(30);first->grab();QCOMPARE(first->property("figmaMdiTitleBounds").toRect(),QRect(0,0,280,34));QCOMPARE(first->property("figmaMdiCloseBounds").toRect(),QRect(257,9,16,16));QCOMPARE(child->geometry(),QRect(1,34,278,135));
        // Real native title hit test and commands; visual flags never change command hints.
        click(first,QPoint(265,17));QVERIFY(first->isVisible());QCOMPARE(first->widget(),child);child->veto=false;click(first,QPoint(265,17));QVERIFY(first->isHidden());QCOMPARE(first->widget(),child);first->show();
        click(first,QPoint(first->width()-41,17));QTRY_VERIFY(first->isMaximized());QCOMPARE(first->widget(),child);first->showNormal();QTRY_VERIFY(!first->isMaximized());
        auto* baseline=new QMdiSubWindow;baseline->setWidget(new QWidget);area.addSubWindow(baseline);baseline->resize(280,170);baseline->show();QStyleOptionTitleBar nativeOption;nativeOption.initFrom(baseline);nativeOption.rect=QRect(0,0,280,baseline->style()->pixelMetric(QStyle::PM_TitleBarHeight,nullptr,baseline));nativeOption.titleBarFlags=baseline->windowFlags();nativeOption.titleBarState=baseline->windowState();click(baseline,baseline->style()->subControlRect(QStyle::CC_TitleBar,&nativeOption,QStyle::SC_TitleBarMaxButton,baseline).center());QTRY_VERIFY(baseline->isMaximized());delete baseline;
        first->showMinimized();QVERIFY(first->isMinimized());first->showNormal();first->showShaded();QVERIFY(first->isShaded());first->showNormal();QVERIFY(!first->isShaded());
        first->setEnabled(false);applyFigmaMdiSubWindowAppearance(first,"Active");QVERIFY(!first->isEnabled());first->setEnabled(true);
        // Actual native move/resize sequence; no custom event handler synthesizes geometry.
        applyFigmaMdiSubWindowAppearance(first);first->setGeometry(40,40,280,170);area.setActiveSubWindow(first);
        const auto drag=[](QMdiSubWindow* sub,const QPoint& from,const QPoint& to){QMouseEvent hover(QEvent::MouseMove,QPointF(from),QPointF(sub->mapToGlobal(from)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(sub,&hover);QTest::mousePress(sub,Qt::LeftButton,Qt::NoModifier,from);QMouseEvent move(QEvent::MouseMove,QPointF(to),QPointF(sub->mapToGlobal(to)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(sub,&move);QTest::mouseRelease(sub,Qt::LeftButton,Qt::NoModifier,to);};
        const auto beforeMove=first->pos();drag(first,QPoint(130,16),QPoint(160,36));QVERIFY(first->pos()!=beforeMove);const auto beforeSize=first->size();drag(first,QPoint(first->width()-1,80),QPoint(first->width()+24,80));QVERIFY(first->width()>beforeSize.width());QCOMPARE(first->widget(),child);
        area.cascadeSubWindows();QVERIFY(first->geometry()!=geom);area.tileSubWindows();QVERIFY(!first->geometry().intersects(second->geometry()));QCOMPARE(area.subWindowList().size(),2);
        area.setViewMode(QMdiArea::TabbedView);area.setTabsClosable(true);area.setTabsMovable(true);applyFigmaMdiAreaAppearance(&area);QTest::qWait(40);auto* bar=area.findChild<QTabBar*>(QString(),Qt::FindDirectChildrenOnly);QVERIFY(bar);QCOMPARE(bar->count(),2);QCOMPARE(bar->tabRect(0).height(),36);area.setActiveSubWindow(first);QCOMPARE(area.activeSubWindow(),first);child->veto=true;
        auto* close=qobject_cast<QAbstractButton*>(bar->tabButton(bar->currentIndex(),QTabBar::RightSide));QVERIFY(close);close->click();QVERIFY(first->isVisible());QCOMPARE(area.subWindowList().size(),2);child->veto=false;close->click();QVERIFY(first->isHidden());QCOMPARE(first->widget(),child);first->show();area.setViewMode(QMdiArea::SubWindowView);
        first->setLayoutDirection(Qt::RightToLeft);QStyleOptionTitleBar option;option.initFrom(first);option.rect=first->rect();option.titleBarFlags=first->windowFlags();option.titleBarState=first->windowState();QCOMPARE(first->style()->subControlRect(QStyle::CC_TitleBar,&option,QStyle::SC_TitleBarCloseButton,first).left(),4);
        QPointer<QWidget> retained=child;area.removeSubWindow(first);QVERIFY(retained);QVERIFY(!first->parentWidget());QCOMPARE(first->widget(),child);QVERIFY(QAccessible::queryAccessibleInterface(first));delete first;QVERIFY(retained.isNull());
        auto* ephemeral=new Z_MdiSubWindow;ephemeral->setWidget(new QWidget);area.addSubWindow(ephemeral);ephemeral->setAttribute(Qt::WA_DeleteOnClose);applyFigmaMdiSubWindowAppearance(ephemeral);ephemeral->show();QPointer<QMdiSubWindow> deleted=ephemeral;ephemeral->close();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QVERIFY(deleted.isNull());
        QVERIFY_EXCEPTION_THROWN(applyFigmaMdiSubWindowAppearance(second,"Invalid"),std::invalid_argument);
    }
    void capturedDockWidgetPreservesPlacementActionsAndOwnership() {
        qRegisterMetaType<Qt::DockWidgetArea>("Qt::DockWidgetArea");
        for(const auto* name:{"Close","Undock"}){QSvgRenderer svg(QString(":/figma/dockwidget/%1.svg").arg(name));QVERIFY(svg.isValid());QCOMPARE(svg.defaultSize(),QSize(16,16));}
        QMainWindow main;main.setDockOptions(QMainWindow::AllowNestedDocks|QMainWindow::AllowTabbedDocks);main.resize(640,360);auto* dock=new Z_DockWidget(&main);dock->setObjectName("testDock");dock->setWindowTitle("Inspector");auto* child=new QLineEdit("Retained content");dock->setWidget(child);main.addDockWidget(Qt::LeftDockWidgetArea,dock);
        auto* close=dock->findChild<QAbstractButton*>("qt_dockwidget_closebutton");auto* floating=dock->findChild<QAbstractButton*>("qt_dockwidget_floatbutton");QVERIFY(close&&floating);auto* layout=dock->layout();auto* toggle=dock->toggleViewAction();const auto features=dock->features();const auto allowed=dock->allowedAreas();QSignalSpy tops(dock,&QDockWidget::topLevelChanged),locations(dock,&QDockWidget::dockLocationChanged),visible(dock,&QDockWidget::visibilityChanged);
        applyFigmaDockWidgetAppearance(dock,"Right","Floating",false,false);QCOMPARE(dock->features(),features);QCOMPARE(dock->allowedAreas(),allowed);QCOMPARE(main.dockWidgetArea(dock),Qt::LeftDockWidgetArea);QVERIFY(!dock->isFloating());QCOMPARE(tops.count(),0);QCOMPARE(locations.count(),0);QCOMPARE(dock->layout(),layout);QCOMPARE(dock->widget(),child);QVERIFY(!dock->titleBarWidget());
        main.show();QTest::qWait(40);dock->grab();QCOMPARE(dock->property("figmaDockEdgeBounds").toRect(),QRect());QVERIFY(close->isVisible());QVERIFY(floating->isVisible());QCOMPARE(close->focusPolicy(),Qt::NoFocus);
        applyFigmaDockWidgetAppearance(dock,"Left","Docked");QTest::qWait(30);dock->grab();QCOMPARE(close->geometry(),QRect(dock->width()-27,10,16,16));QCOMPARE(floating->geometry(),QRect(dock->width()-51,10,16,16));QCOMPARE(dock->property("figmaDockEdgeBounds").toRect(),QRect(0,0,3,dock->height()));close->grab();QCOMPARE(close->property("figmaDockGlyphBounds").toRect(),QRect(0,0,16,16));
        close->click();QVERIFY(dock->isHidden());QCOMPARE(dock->widget(),child);QVERIFY(!toggle->isChecked());toggle->trigger();QVERIFY(!dock->isHidden());QVERIFY(toggle->isChecked());QVERIFY(visible.count()>=2);
        floating->click();QTRY_VERIFY(dock->isFloating());QCOMPARE(dock->widget(),child);QCOMPARE(child->text(),QString("Retained content"));QVERIFY(!dock->titleBarWidget());QCOMPARE(tops.count(),1);dock->setFloating(false);QTRY_VERIFY(!dock->isFloating());QCOMPARE(tops.count(),2);
        QTest::mouseDClick(dock,Qt::LeftButton,Qt::NoModifier,QPoint(80,15));QTRY_VERIFY(dock->isFloating());dock->setFloating(false);QTRY_VERIFY(!dock->isFloating());
        // Qt5 QTest::mouseMove(QWidget*) only moves QCursor; offscreen emits no move event.
        // Deliver the same native input sequence to an unstyled baseline and the adapted dock.
        const auto drag=[](QDockWidget* target){QTest::mousePress(target,Qt::LeftButton,Qt::ControlModifier,QPoint(80,10));QMouseEvent move(QEvent::MouseMove,QPointF(140,75),QPointF(target->mapToGlobal(QPoint(140,75))),Qt::NoButton,Qt::LeftButton,Qt::ControlModifier);QApplication::sendEvent(target,&move);QTest::mouseRelease(target,Qt::LeftButton,Qt::ControlModifier,QPoint(140,75));};
        auto* baseline=new QDockWidget("Baseline",&main);baseline->setWidget(new QLabel("Baseline"));main.addDockWidget(Qt::RightDockWidgetArea,baseline);baseline->show();QTest::qWait(20);drag(baseline);QTRY_VERIFY(baseline->isFloating());baseline->hide();delete baseline;drag(dock);QTRY_VERIFY(dock->isFloating());dock->setFloating(false);QCOMPARE(dock->widget(),child);
        dock->setFeatures(QDockWidget::DockWidgetMovable);QVERIFY(close->isHidden());QVERIFY(floating->isHidden());QVERIFY(!toggle->isEnabled());applyFigmaDockWidgetAppearance(dock,"Bottom","Floating");QCOMPARE(dock->features(),QDockWidget::DockWidgetFeatures(QDockWidget::DockWidgetMovable));QTest::mouseDClick(dock,Qt::LeftButton,Qt::NoModifier,QPoint(80,15));QVERIFY(!dock->isFloating());dock->setFeatures(features);
        dock->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);QVERIFY(!dock->isAreaAllowed(Qt::BottomDockWidgetArea));applyFigmaDockWidgetAppearance(dock,"Bottom","Docked");QCOMPARE(dock->allowedAreas(),Qt::DockWidgetAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea));
        main.addDockWidget(Qt::RightDockWidgetArea,dock);const auto saved=main.saveState(7);main.addDockWidget(Qt::LeftDockWidgetArea,dock);QVERIFY(main.restoreState(saved,7));QCOMPARE(main.dockWidgetArea(dock),Qt::RightDockWidgetArea);QVERIFY(!main.restoreState(saved,8));
        auto* other=new QDockWidget("Other",&main);other->setObjectName("otherDock");other->setWidget(new QLabel("Other"));main.addDockWidget(Qt::RightDockWidgetArea,other);main.tabifyDockWidget(dock,other);QVERIFY(main.tabifiedDockWidgets(dock).contains(other));dock->raise();QCOMPARE(dock->widget(),child);
        dock->setFeatures(features|QDockWidget::DockWidgetVerticalTitleBar);QTest::qWait(30);QCOMPARE(dock->widget(),child);QVERIFY(!dock->titleBarWidget());dock->setFeatures(features);dock->setLayoutDirection(Qt::RightToLeft);QTest::qWait(20);QCOMPARE(close->geometry().left(),11);
        QPointer<QWidget> alive=child;main.removeDockWidget(dock);QVERIFY(alive);QCOMPARE(dock->widget(),child);QCOMPARE(dock->toggleViewAction(),toggle);QCOMPARE(dock->findChild<QAbstractButton*>("qt_dockwidget_closebutton"),close);QVERIFY(QAccessible::queryAccessibleInterface(dock));QVERIFY_EXCEPTION_THROWN(applyFigmaDockWidgetAppearance(dock,"Center"),std::invalid_argument);
    }
    void capturedItemOverrideDoesNotHideOtherItemsNativeFocus() {
        const auto before=FocusIndicator::instance().mode();FocusIndicator::instance().setMode(FocusIndicator::Mode::Always);
        Z_ToolBox box;box.resize(500,300);box.addItem(new QWidget,"One");box.addItem(new QWidget,"Two");box.setCurrentIndex(1);applyFigmaToolBoxAppearance(&box,"Default",0);box.show();box.activateWindow();QTest::qWait(30);auto buttons=box.findChildren<QAbstractButton*>("qt_toolbox_toolboxbutton",Qt::FindDirectChildrenOnly);buttons[1]->setFocusPolicy(Qt::StrongFocus);buttons[1]->setFocus();QTest::qWait(20);QVERIFY(buttons[1]->hasFocus());QCOMPARE(buttons[1]->grab().toImage().pixelColor(1,1),QColor("#6235f7"));box.hide();
        Z_TabWidget tabs;tabs.resize(500,300);tabs.addTab(new QWidget,"One");tabs.addTab(new QWidget,"Two");tabs.setCurrentIndex(1);applyFigmaTabWidgetAppearance(&tabs,"Default",0);tabs.show();tabs.activateWindow();QTest::qWait(30);tabs.tabBar()->setFocus();QTest::qWait(20);QVERIFY(tabs.tabBar()->hasFocus());const auto point=tabs.tabBar()->tabRect(1).topLeft()+QPoint(1,1);QCOMPARE(tabs.tabBar()->grab().toImage().pixelColor(point),QColor("#6235f7"));FocusIndicator::instance().setMode(before);
    }
    void capturedToolBoxKeepsSingleCurrentMnemonicAndOwnership() {
        for(const auto* name:{"ChevronRight","ChevronDown","ChevronDisabled","SettingsComposition","SettingsPrimary","SettingsDisabled"}){QSvgRenderer svg(QString(":/figma/toolbox/%1.svg").arg(name));QVERIFY(svg.isValid());QCOMPARE(svg.defaultSize(),QSize(16,16));}
        Z_ToolBox box;box.resize(800,360);auto* general=new QWidget;auto* rules=new QWidget;auto* notices=new QWidget;box.addItem(general,"&General");box.addItem(rules,figmaToolBoxSettingsIcon(),"&Rules");box.addItem(notices,"Notifications");box.setCurrentIndex(1);
        auto buttons=box.findChildren<QAbstractButton*>("qt_toolbox_toolboxbutton",Qt::FindDirectChildrenOnly);QCOMPARE(buttons.size(),3);auto* first=buttons[0];auto* second=buttons[1];const auto shortcut=first->shortcut();auto* layout=box.layout();QSignalSpy changes(&box,&QToolBox::currentChanged);
        applyFigmaToolBoxAppearance(&box,"Selected",0,true);QCOMPARE(box.currentIndex(),1);QCOMPARE(changes.count(),0);QCOMPARE(box.layout(),layout);QCOMPARE(first->shortcut(),shortcut);QCOMPARE(first->focusPolicy(),Qt::NoFocus);QVERIFY(!first->hasFocus());box.show();QTest::qWait(40);first->grab();QCOMPARE(first->height(),48);QCOMPARE(first->property("figmaToolBoxArrowBounds").toRect(),QRect(19,16,16,16));QCOMPARE(first->property("figmaToolBoxVisualState").toString(),QString("Selected"));QVERIFY(!first->property("figmaToolBoxMnemonicBounds").toRect().isEmpty());QCOMPARE(layout->contentsMargins(),QMargins(16,16,16,16));QCOMPARE(layout->spacing(),0);
        applyFigmaToolBoxAppearance(&box);QTest::mouseClick(first,Qt::LeftButton);QCOMPARE(box.currentWidget(),general);QCOMPARE(changes.count(),1);QVERIFY(general->isVisible());QVERIFY(!rules->isVisible());QTest::mouseClick(first,Qt::LeftButton);QCOMPARE(changes.count(),1);QCOMPARE(box.currentIndex(),0);
        box.setItemEnabled(0,false);QCOMPARE(box.currentIndex(),1);QVERIFY(box.isEnabled());QTest::mouseClick(first,Qt::LeftButton);QCOMPARE(box.currentIndex(),1);box.setItemEnabled(0,true);
        second->setFocusPolicy(Qt::StrongFocus);second->setFocus();QTest::keyClick(second,Qt::Key_Space);QCOMPARE(box.currentWidget(),rules);
        box.activateWindow();QTRY_VERIFY(box.isActiveWindow());QTest::keyClick(&box,Qt::Key_G,Qt::AltModifier);
        // Mnemonics use QAbstractButton::animateClick: await the native signal, not a 130ms deadline.
        QTRY_COMPARE(box.currentWidget(),general);
        box.setItemText(0,"&&Literal &Page");applyFigmaToolBoxAppearance(&box,"Focus",0,true);QCOMPARE(first->text(),QString("&&Literal &Page"));QCOMPARE(first->shortcut(),QKeySequence::mnemonic(first->text()));box.setLayoutDirection(Qt::RightToLeft);first->grab();QCOMPARE(first->property("figmaToolBoxArrowBounds").toRect().left(),first->width()-35);
        auto* inserted=new QWidget;box.insertItem(1,inserted,"Inserted");QTest::qWait(40);QCOMPARE(box.layout()->contentsMargins(),QMargins(16,16,16,16));QCOMPARE(box.layout()->spacing(),0);buttons=box.findChildren<QAbstractButton*>("qt_toolbox_toolboxbutton",Qt::FindDirectChildrenOnly);QCOMPARE(buttons.size(),4);for(auto* button:buttons){QCOMPARE(button->height(),48);QVERIFY(button->findChild<QObject*>("figmaToolBoxItemStyle"));}
        QPointer<QWidget> alive=rules;box.removeItem(box.indexOf(rules));QVERIFY(alive);QCOMPARE(box.indexOf(rules),-1);QCOMPARE(box.count(),3);QCOMPARE(rules->parentWidget(),static_cast<QWidget*>(&box));box.addItem(rules,"Rules restored");QTest::qWait(40);box.setCurrentWidget(rules);QCOMPARE(box.currentWidget(),rules);QVERIFY(QAccessible::queryAccessibleInterface(&box));QVERIFY_EXCEPTION_THROWN(applyFigmaToolBoxAppearance(&box,"Pressed"),std::invalid_argument);
    }
    void capturedTabsKeepSelectionCloseMoveAndPageOwnership() {
        for(const auto* name:{"ChevronRight","ChevronLeft","ClosePrimary","CloseDisabled","DocumentPrimary","DocumentAccent","DocumentDisabled","DocumentCompositionPrimary","DocumentCompositionAccent"}){QSvgRenderer svg(QString(":/figma/tabs/%1.svg").arg(name));QVERIFY(svg.isValid());QCOMPARE(svg.defaultSize(),QSize(16,16));}
        Z_TabWidget tabs;tabs.resize(800,300);auto* first=new QWidget;auto* second=new QWidget;auto* third=new QWidget;tabs.addTab(first,"&One");tabs.addTab(second,"&Two");tabs.addTab(third,"&Three");auto* nativeBar=tabs.tabBar();tabs.setTabsClosable(true);tabs.setMovable(true);tabs.setTabIcon(0,figmaTabDocumentIcon(true));const auto icon=tabs.tabIcon(0).cacheKey();
        QSignalSpy changed(&tabs,&QTabWidget::currentChanged),closed(&tabs,&QTabWidget::tabCloseRequested),moved(nativeBar,&QTabBar::tabMoved);
        applyFigmaTabWidgetAppearance(&tabs,"Selected",1);QCOMPARE(tabs.currentIndex(),0);QCOMPARE(changed.count(),0);QCOMPARE(tabs.tabBar(),nativeBar);QCOMPARE(tabs.tabIcon(0).cacheKey(),icon);QVERIFY(!nativeBar->hasFocus());tabs.show();QTest::qWait(30);tabs.grab();QCOMPARE(nativeBar->tabRect(0).size(),QSize(176,48));
        auto* close=qobject_cast<QAbstractButton*>(nativeBar->tabButton(1,QTabBar::RightSide));QVERIFY(close);QCOMPARE(close->size(),QSize(16,16));close->click();QCOMPARE(closed.count(),1);QCOMPARE(closed.at(0).at(0).toInt(),1);QCOMPARE(tabs.count(),3);QCOMPARE(tabs.widget(1),second);
        applyFigmaTabWidgetAppearance(&tabs);QTest::mouseClick(nativeBar,Qt::LeftButton,Qt::NoModifier,nativeBar->tabRect(1).center());QCOMPARE(tabs.currentWidget(),second);QCOMPARE(changed.count(),1);QVERIFY(second->isVisible());QVERIFY(!first->isVisible());tabs.setTabEnabled(2,false);QTest::mouseClick(nativeBar,Qt::LeftButton,Qt::NoModifier,nativeBar->tabRect(2).center());QCOMPARE(tabs.currentWidget(),second);
        nativeBar->moveTab(1,0);QCOMPARE(moved.count(),1);QCOMPARE(tabs.widget(0),second);QCOMPARE(tabs.currentWidget(),second);QCOMPARE(tabs.currentIndex(),0);QCOMPARE(tabs.widget(1),first);
        tabs.setCurrentIndex(0);nativeBar->setFocus();QTest::keyClick(nativeBar,Qt::Key_Right);QCOMPARE(tabs.currentWidget(),first);QTest::keyClick(&tabs,Qt::Key_Tab,Qt::ControlModifier);QCOMPARE(tabs.currentWidget(),second);
        for(auto position:{QTabWidget::North,QTabWidget::South,QTabWidget::West,QTabWidget::East}){tabs.setTabPosition(position);applyFigmaTabWidgetAppearance(&tabs);QTest::qWait(10);QCOMPARE(nativeBar->tabRect(0).size(),QSize(176,48));QCOMPARE(tabs.count(),3);QCOMPARE(tabs.widget(0),second);QCOMPARE(tabs.currentWidget(),second);QVERIFY(tabs.rect().contains(nativeBar->geometry()));QVERIFY(tabs.rect().contains(QRect(second->mapTo(&tabs,QPoint()),second->size())));}
        tabs.setTabPosition(QTabWidget::North);tabs.setLayoutDirection(Qt::RightToLeft);applyFigmaTabWidgetAppearance(&tabs);QTest::qWait(10);QVERIFY(nativeBar->tabRect(0).x()>nativeBar->tabRect(1).x());tabs.setLayoutDirection(Qt::LeftToRight);
        tabs.resize(370,300);tabs.setElideMode(Qt::ElideNone);tabs.setUsesScrollButtons(true);applyFigmaTabWidgetAppearance(&tabs);QTest::qWait(20);QToolButton* next=nullptr;int visible=0;for(auto* button:nativeBar->findChildren<QToolButton*>()){if(button->isVisible()){++visible;QCOMPARE(button->size(),QSize(22,48));if(button->arrowType()==Qt::RightArrow)next=button;}}QCOMPARE(visible,2);QVERIFY(next);const auto before=nativeBar->tabRect(0).x();QTest::mouseClick(next,Qt::LeftButton);QVERIFY(nativeBar->tabRect(0).x()<before);QCOMPARE(tabs.currentWidget(),second);
        QPointer<QWidget> alive=first;tabs.removeTab(tabs.indexOf(first));QVERIFY(alive);QCOMPARE(tabs.count(),2);QCOMPARE(tabs.indexOf(first),-1);QVERIFY(QAccessible::queryAccessibleInterface(&tabs));QVERIFY_EXCEPTION_THROWN(applyFigmaTabWidgetAppearance(&tabs,"Pressed"),std::invalid_argument);
    }
    void capturedDialogActionsKeepRolesStandardIdentityAndKeys() {
        QWidget host;host.resize(600,300);Z_DialogButtonBox box(&host);box.resize(560,160);
        box.setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Apply|QDialogButtonBox::Reset|QDialogButtonBox::Help);
        auto* ok=box.button(QDialogButtonBox::Ok);const auto text=ok->text();const auto shortcut=ok->shortcut();auto* nativeLayout=box.layout();const auto nativePolicy=box.style()->styleHint(QStyle::SH_DialogButtonLayout,nullptr,&box);
        applyFigmaDialogButtonBoxAppearance(&box);host.show();QTest::qWait(30);
        QCOMPARE(box.button(QDialogButtonBox::Ok),ok);QCOMPARE(box.standardButton(ok),QDialogButtonBox::Ok);QCOMPARE(ok->text(),text);QCOMPARE(ok->shortcut(),shortcut);QCOMPARE(box.layout(),nativeLayout);QVERIFY(nativeLayout->isEnabled());QCOMPARE(nativeLayout->contentsMargins(),QMargins(16,16,16,16));QCOMPARE(nativeLayout->spacing(),8);QCOMPARE(box.style()->styleHint(QStyle::SH_DialogButtonLayout,nullptr,&box),nativePolicy);
        QSignalSpy accepted(&box,&QDialogButtonBox::accepted),rejected(&box,&QDialogButtonBox::rejected),help(&box,&QDialogButtonBox::helpRequested);int clicks=0;QDialogButtonBox::ButtonRole last=QDialogButtonBox::InvalidRole;QObject::connect(&box,&QDialogButtonBox::clicked,&box,[&](QAbstractButton* b){++clicks;last=box.buttonRole(b);});
        ok->click();QCOMPARE(accepted.count(),1);QCOMPARE(last,QDialogButtonBox::AcceptRole);box.button(QDialogButtonBox::Cancel)->click();QCOMPARE(rejected.count(),1);box.button(QDialogButtonBox::Help)->click();QCOMPARE(help.count(),1);
        box.button(QDialogButtonBox::Apply)->click();QCOMPARE(last,QDialogButtonBox::ApplyRole);box.button(QDialogButtonBox::Reset)->click();QCOMPARE(last,QDialogButtonBox::ResetRole);QCOMPARE(accepted.count(),1);QCOMPARE(rejected.count(),1);
        auto* destructive=box.addButton("Delete",QDialogButtonBox::DestructiveRole);applyFigmaDialogButtonBoxAppearance(&box);QCOMPARE(box.standardButton(destructive),QDialogButtonBox::NoButton);destructive->click();QCOMPARE(last,QDialogButtonBox::DestructiveRole);QCOMPARE(clicks,6);QCOMPARE(accepted.count(),1);QCOMPARE(rejected.count(),1);
        box.setEnabled(false);ok->click();destructive->click();QCOMPARE(clicks,6);box.setEnabled(true);
        box.setOrientation(Qt::Vertical);box.setCenterButtons(true);box.setLayoutDirection(Qt::RightToLeft);applyFigmaDialogButtonBoxAppearance(&box);QCOMPARE(box.orientation(),Qt::Vertical);QVERIFY(box.centerButtons());QCOMPARE(box.button(QDialogButtonBox::Ok),ok);QVERIFY(box.layout()->isEnabled());QTest::qWait(20);ok->grab();QCOMPARE(ok->property("figmaDialogButtonRecipe").toString(),QString("PRIMARY"));
        box.setStandardButtons(QDialogButtonBox::Help);QTest::qWait(30);auto* newHelp=box.button(QDialogButtonBox::Help);QVERIFY(newHelp);QVERIFY(newHelp->findChild<QObject*>("figmaDialogNativeButtonPaint"));QCOMPARE(box.buttonRole(destructive),QDialogButtonBox::DestructiveRole);
        box.removeButton(destructive);destructive->setParent(&host);applyFigmaDialogButtonBoxAppearance(&box);destructive->grab();QCOMPARE(box.buttonRole(destructive),QDialogButtonBox::InvalidRole);
        QDialog dialog;auto* layout=new QVBoxLayout(&dialog);auto* actions=new Z_DialogButtonBox(&dialog);actions->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);applyFigmaDialogButtonBoxAppearance(actions);layout->addWidget(actions);QObject::connect(actions,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(actions,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);QSignalSpy finished(&dialog,&QDialog::finished);
        dialog.show();QTest::qWait(30);QVERIFY(actions->button(QDialogButtonBox::Ok)->isDefault());QTest::keyClick(&dialog,Qt::Key_Return);QCOMPARE(finished.count(),1);QCOMPARE(dialog.result(),int(QDialog::Accepted));dialog.show();QTest::qWait(20);QTest::keyClick(&dialog,Qt::Key_Escape);QCOMPARE(finished.count(),2);QCOMPARE(dialog.result(),int(QDialog::Rejected));
    }
    void capturedToolButtonKeepsNativeActionAndPopupModes() {
        qRegisterMetaType<QAction*>("QAction*");
        for(const auto* name:{"ActionPrimary","ActionDisabled","ActionAccent"}){QSvgRenderer svg(QString(":/figma/toolbutton/%1.svg").arg(name));QVERIFY(svg.isValid());QCOMPARE(svg.defaultSize(),QSize(16,16));}
        class Probe:public Z_ToolButton{public:using Z_ToolButton::Z_ToolButton;using Z_ToolButton::initStyleOption;};QWidget host;host.resize(400,200);Probe button(&host);
        button.setText("Format");button.setIcon(figmaToolButtonIcon());button.setIconSize(QSize(16,16));button.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);button.setPopupMode(QToolButton::MenuButtonPopup);
        QMenu menu;auto* menuAction=menu.addAction("Format document");menu.addAction("Format selection");button.setMenu(&menu);
        QSignalSpy clicks(&button,&QAbstractButton::clicked),triggers(&button,&QToolButton::triggered),opened(&menu,&QMenu::aboutToShow);
        applyFigmaToolButtonAppearance(&button,"Default",true);button.resize(119,32);host.show();button.show();QTest::qWait(20);button.grab();
        QCOMPARE(button.property("figmaToolIconBounds").toRect(),QRect(9,7,16,16));QCOMPARE(button.property("figmaToolLabelBounds").toRect(),QRect(33,5,48,22));QCOMPARE(button.property("figmaToolMenuBounds").toRect(),QRect(90,0,28,32));
        applyFigmaToolButtonAppearance(&button,"Focus",true);button.resize(121,32);button.grab();QCOMPARE(button.property("figmaToolIconBounds").toRect(),QRect(10,7,16,16));QCOMPARE(button.property("figmaToolMenuBounds").toRect(),QRect(91,0,28,32));
        applyFigmaToolButtonAppearance(&button,"Open",true);QVERIFY(!menu.isVisible());QVERIFY(!button.isDown());QCOMPARE(opened.count(),0);
        QAction action("Format",&button);action.setIcon(figmaToolButtonIcon());action.setCheckable(true);button.setDefaultAction(&action);applyFigmaToolButtonAppearance(&button);button.resize(button.sizeHint());QSignalSpy actionTriggered(&action,&QAction::triggered);
        QStyleOptionToolButton opt;button.initStyleOption(&opt);auto main=button.style()->subControlRect(QStyle::CC_ToolButton,&opt,QStyle::SC_ToolButton,&button);auto drop=button.style()->subControlRect(QStyle::CC_ToolButton,&opt,QStyle::SC_ToolButtonMenu,&button);
        QVERIFY(!main.intersects(drop));QCOMPARE(main.width()+drop.width(),button.width());QTest::mouseClick(&button,Qt::LeftButton,Qt::NoModifier,main.center());QCOMPARE(clicks.count(),1);QCOMPARE(actionTriggered.count(),1);QVERIFY(action.isChecked());QVERIFY(button.isChecked());
        bool sawOpen=false;QTimer::singleShot(80,&menu,[&]{sawOpen=menu.isVisible();button.grab();QCOMPARE(button.property("figmaToolVisualState").toString(),QString("Open"));QTest::keyClick(&menu,Qt::Key_Escape);});
        QTest::mouseClick(&button,Qt::LeftButton,Qt::NoModifier,drop.center());QVERIFY(sawOpen);QCOMPARE(opened.count(),1);QCOMPARE(clicks.count(),1);QCOMPARE(actionTriggered.count(),1);QVERIFY(!button.isDown());
        QTimer::singleShot(80,&menu,[&]{menu.setActiveAction(menuAction);QTest::keyClick(&menu,Qt::Key_Return);});button.showMenu();QCOMPARE(triggers.count(),2);QCOMPARE(triggers.last().at(0).value<QAction*>(),menuAction);
        action.setText("Updated");QCOMPARE(button.text(),QString("Updated"));action.setChecked(false);QVERIFY(!button.isChecked());action.setEnabled(false);QVERIFY(!button.isEnabled());applyFigmaToolButtonAppearance(&button,"Focus");QVERIFY(!button.isEnabled());QCOMPARE(button.defaultAction(),&action);action.setEnabled(true);
        QFont actionFont("Inter");actionFont.setPixelSize(14);actionFont.setWeight(QFont::Normal);action.setFont(actionFont);const auto beforeFont=button.font();QToolButton nativeFontBaseline;nativeFontBaseline.setDefaultAction(&action);applyFigmaToolButtonAppearance(&button);QCOMPARE(button.font(),beforeFont);QCOMPARE(button.font(),nativeFontBaseline.font());QCOMPARE(button.font().pixelSize(),14);QCOMPARE(button.font().weight(),int(QFont::Normal));
        button.setPopupMode(QToolButton::InstantPopup);applyFigmaToolButtonAppearance(&button);QTimer::singleShot(80,&menu,[&]{QVERIFY(menu.isVisible());QTest::keyClick(&menu,Qt::Key_Escape);});const int before=actionTriggered.count();QTest::mouseClick(&button,Qt::LeftButton);QCOMPARE(actionTriggered.count(),before);QVERIFY(!button.isDown());
        button.setPopupMode(QToolButton::DelayedPopup);applyFigmaToolButtonAppearance(&button);QTest::mouseClick(&button,Qt::LeftButton);QCOMPARE(actionTriggered.count(),before+1);
        const int delay=button.style()->styleHint(QStyle::SH_ToolButton_PopupDelay,nullptr,&button);bool sawDelayed=false;QTimer::singleShot(delay+80,&menu,[&]{sawDelayed=menu.isVisible();menu.close();});QTest::mousePress(&button,Qt::LeftButton);QTest::qWait(delay+130);QTest::mouseRelease(&button,Qt::LeftButton);QVERIFY(sawDelayed);QCOMPARE(actionTriggered.count(),before+1);
        const auto noSplit=button.sizeHint();button.setPopupMode(QToolButton::MenuButtonPopup);applyFigmaToolButtonAppearance(&button);QCOMPARE(button.sizeHint().width(),noSplit.width()+29);
        button.setLayoutDirection(Qt::RightToLeft);button.initStyleOption(&opt);drop=button.style()->subControlRect(QStyle::CC_ToolButton,&opt,QStyle::SC_ToolButtonMenu,&button);QCOMPARE(drop.left(),0);QCOMPARE(button.style()->hitTestComplexControl(QStyle::CC_ToolButton,&opt,drop.center(),&button),QStyle::SC_ToolButtonMenu);
        action.setPriority(QAction::LowPriority);button.grab();QVERIFY(button.property("figmaToolLabelBounds").toRect().isEmpty());action.setPriority(QAction::NormalPriority);button.grab();QVERIFY(!button.property("figmaToolLabelBounds").toRect().isEmpty());
        const auto savedText=button.text();button.setToolButtonStyle(Qt::ToolButtonIconOnly);applyFigmaToolButtonAppearance(&button);QCOMPARE(button.text(),savedText);QVERIFY(button.icon().cacheKey()==action.icon().cacheKey());QVERIFY(QAccessible::queryAccessibleInterface(&button));
        button.setToolButtonStyle(Qt::ToolButtonTextUnderIcon);applyFigmaToolButtonAppearance(&button);button.resize(button.sizeHint());button.grab();QVERIFY(button.property("figmaToolLabelBounds").toRect().top()>button.property("figmaToolIconBounds").toRect().bottom());QCOMPARE(button.property("figmaToolPaintFont").value<QFont>().weight(),int(QFont::Normal));
        button.setToolButtonStyle(Qt::ToolButtonTextOnly);button.grab();QVERIFY(button.property("figmaToolIconBounds").toRect().isEmpty());
        const auto keyCount=actionTriggered.count();QTest::keyClick(&button,Qt::Key_Space);QCOMPARE(actionTriggered.count(),keyCount+1);action.setEnabled(false);QTest::mouseClick(&button,Qt::LeftButton);QCOMPARE(actionTriggered.count(),keyCount+1);action.setEnabled(true);
        button.setPopupMode(QToolButton::DelayedPopup);QVERIFY_EXCEPTION_THROWN(applyFigmaToolButtonAppearance(&button,"Open"),std::invalid_argument);QVERIFY_EXCEPTION_THROWN(applyFigmaToolButtonAppearance(&button,"Loading"),std::invalid_argument);
    }
    void capturedCommandLinkKeepsNativeActionAndDescription() {
        for(const auto* name:{"ArrowPrimary","ArrowDisabled","ArrowAccent"}){QSvgRenderer svg(QString(":/figma/commandlink/%1.svg").arg(name));QVERIFY(svg.isValid());QCOMPARE(svg.defaultSize(),QSize(16,16));}
        Z_CommandLinkButton button;button.resize(360,88);button.setText("Create a new workspace");button.setDescription("Start with an empty workspace and configure it later.");button.setIcon(figmaCommandLinkIcon());button.setIconSize(QSize(16,16));button.setCheckable(true);button.setAutoDefault(false);
        const auto icon=button.icon().cacheKey();const auto description=button.description();QSignalSpy clicks(&button,&QAbstractButton::clicked),toggled(&button,&QAbstractButton::toggled),pressed(&button,&QAbstractButton::pressed);
        applyFigmaCommandLinkAppearance(&button,"Pressed");QVERIFY(!button.isDown());QCOMPARE(pressed.count(),0);QVERIFY(!button.isChecked());
        button.show();QTest::qWait(20);button.grab();QCOMPARE(button.property("figmaCommandVisualState").toString(),QString("Pressed"));QCOMPARE(button.description(),description);
        QTest::mouseClick(&button,Qt::LeftButton);QCOMPARE(clicks.count(),1);QCOMPARE(toggled.count(),1);QVERIFY(button.isChecked());
        applyFigmaCommandLinkAppearance(&button,"Focus",false,false);QCOMPARE(button.description(),description);QCOMPARE(button.icon().cacheKey(),icon);QVERIFY(button.isChecked());QVERIFY(!button.isDown());
        applyFigmaCommandLinkAppearance(&button);QTest::keyClick(&button,Qt::Key_Space);QCOMPARE(clicks.count(),2);QVERIFY(!button.isChecked());
        button.setFlat(true);button.setDefault(true);applyFigmaCommandLinkAppearance(&button,"Default");QVERIFY(button.isFlat());QVERIFY(button.isDefault());QVERIFY(!button.autoDefault());
        button.setDescription(QString("Long description ").repeated(40));QTest::qWait(40);QVERIFY(button.minimumHeight()>88);QCOMPARE(button.description(),QString("Long description ").repeated(40));
        button.setDescription(description);applyFigmaCommandLinkAppearance(&button,"Default",true,true,true);button.setFixedSize(360,78);button.grab();QCOMPARE(button.property("figmaCommandContentBounds").toRect(),QRect(53,13,292,52));QCOMPARE(button.property("figmaCommandIconBounds").toRect(),QRect(17,27,16,16));
        applyFigmaCommandLinkAppearance(&button,"CheckedFocus",true,true,true);button.setFixedSize(360,80);button.grab();QCOMPARE(button.property("figmaCommandContentBounds").toRect(),QRect(54,14,292,52));QCOMPARE(button.property("figmaCommandIconBounds").toRect(),QRect(18,28,16,16));QVERIFY(!button.isChecked());
        button.setEnabled(false);const auto count=clicks.count();QTest::mouseClick(&button,Qt::LeftButton);QCOMPARE(clicks.count(),count);button.grab();QCOMPARE(button.property("figmaCommandVisualState").toString(),QString("Disabled"));QVERIFY(QAccessible::queryAccessibleInterface(&button));
        button.setEnabled(true);Z_CommandLinkButton other;other.setCheckable(true);QButtonGroup group;group.addButton(&button,1);group.addButton(&other,2);button.click();QCOMPARE(group.checkedId(),1);other.click();QCOMPARE(group.checkedId(),2);QVERIFY(!button.isChecked());
        QVERIFY_EXCEPTION_THROWN(applyFigmaCommandLinkAppearance(&button,"Loading"),std::invalid_argument);
    }
    void capturedComboKeepsModelEditorAndFontAuthority() {
        for(const auto* name:{"ChevronDown","Prefix","Clear"}){QSvgRenderer svg(QString(":/figma/combo/%1.svg").arg(name));QVERIFY(svg.isValid());QCOMPARE(svg.defaultSize(),QSize(16,16));}
        Z_ComboBox combo;combo.resize(240,32);combo.setPlaceholderText("Please select");combo.addItems({"Alpha","Beta","Gamma"});combo.setCurrentIndex(-1);
        QStandardItemModel model;model.appendRow(new QStandardItem("Alpha"));model.appendRow(new QStandardItem("Beta"));model.appendRow(new QStandardItem("Gamma"));
        combo.setModel(&model);combo.setCurrentIndex(-1);QStyledItemDelegate delegate;combo.setItemDelegate(&delegate);
        QSignalSpy changed(&combo,qOverload<int>(&QComboBox::currentIndexChanged)),activated(&combo,qOverload<int>(&QComboBox::activated));
        applyFigmaComboBoxAppearance(&combo,"Small","Focus");QCOMPARE(combo.currentIndex(),-1);QVERIFY(combo.currentText().isEmpty());QCOMPARE(changed.count(),0);QCOMPARE(activated.count(),0);
        QCOMPARE(combo.model(),static_cast<QAbstractItemModel*>(&model));QCOMPARE(combo.itemDelegate(),static_cast<QAbstractItemDelegate*>(&delegate));
        combo.show();QTest::qWait(20);combo.grab();QCOMPARE(combo.height(),24);QCOMPARE(combo.property("figmaComboArrowBounds").toRect(),QRect(216,4,16,16));QVERIFY(!combo.view()->isVisible());
        combo.setCurrentIndex(1);changed.clear();applyFigmaComboBoxAppearance(&combo,"Large","Hover");QCOMPARE(combo.height(),40);QCOMPARE(combo.currentText(),QString("Beta"));QCOMPARE(changed.count(),0);
        combo.setFocus();QTest::keyClick(&combo,Qt::Key_Down);QCOMPARE(combo.currentIndex(),2);QCOMPARE(activated.count(),1);
        QTest::keyClick(&combo,Qt::Key_F4);QTRY_VERIFY(combo.view()->isVisible());QTest::keyClick(combo.view(),Qt::Key_Escape);QTRY_VERIFY(!combo.view()->isVisible());
        combo.setEditable(true);auto* editor=combo.lineEdit();QVERIFY(editor);auto* completer=combo.completer();QVERIFY(completer);
        QIntValidator validator(0,9999,&combo);combo.setValidator(&validator);combo.setEditText("1234");editor->setSelection(1,2);changed.clear();
        applyFigmaComboBoxAppearance(&combo,"Middle");QCOMPARE(combo.lineEdit(),editor);QCOMPARE(combo.completer(),completer);QCOMPARE(combo.validator(),static_cast<const QValidator*>(&validator));QCOMPARE(editor->selectedText(),QString("23"));QCOMPARE(combo.currentText(),QString("1234"));QCOMPARE(changed.count(),0);
        editor->selectAll();QTest::keyClicks(editor,"abcd");QCOMPARE(combo.currentText(),QString("1234"));combo.setValidator(nullptr);
        editor->selectAll();QInputMethodEvent commit;commit.setCommitString(QString::fromUtf8("工艺"));QApplication::sendEvent(editor,&commit);QCOMPARE(combo.currentText(),QString::fromUtf8("工艺"));
        combo.setInsertPolicy(QComboBox::InsertAtBottom);combo.setEditText("Delta");QTest::keyClick(editor,Qt::Key_Return);QCOMPARE(combo.count(),4);QCOMPARE(combo.currentText(),QString("Delta"));
        combo.setLayoutDirection(Qt::RightToLeft);applyFigmaComboBoxAppearance(&combo,"Small","Default");combo.grab();QCOMPARE(combo.property("figmaComboArrowBounds").toRect(),QRect(8,4,16,16));QCOMPARE(combo.lineEdit(),editor);
        combo.setEnabled(false);QTest::qWait(20);combo.grab();QCOMPARE(combo.property("figmaComboVisualState").toString(),QString("Disabled"));QVERIFY(QAccessible::queryAccessibleInterface(&combo));
        Z_FontComboBox fonts;fonts.resize(240,32);auto* fontModel=fonts.model();auto* fontDelegate=fonts.itemDelegate();auto* fontEditor=fonts.lineEdit();const auto font=fonts.currentFont();const auto count=fonts.count();
        QSignalSpy fontChanged(&fonts,&QFontComboBox::currentFontChanged);applyFigmaComboBoxAppearance(&fonts,"Large","Focus");
        QCOMPARE(fonts.model(),fontModel);QCOMPARE(fonts.itemDelegate(),fontDelegate);QCOMPARE(fonts.lineEdit(),fontEditor);QCOMPARE(fonts.currentFont(),font);QCOMPARE(fonts.count(),count);QCOMPARE(fontChanged.count(),0);QVERIFY(fonts.isEditable());
        QFontComboBox baseline;baseline.setFontFilters(QFontComboBox::MonospacedFonts);fonts.setFontFilters(QFontComboBox::MonospacedFonts);QCOMPARE(fonts.count(),baseline.count());
        baseline.setWritingSystem(QFontDatabase::Latin);fonts.setWritingSystem(QFontDatabase::Latin);QCOMPARE(fonts.count(),baseline.count());
        for(int i=0;i<fonts.count();++i)QCOMPARE(fonts.itemText(i),baseline.itemText(i));
        fonts.setEditable(false);applyFigmaComboBoxAppearance(&fonts,"Middle","Default",true);fonts.show();QTest::qWait(20);const auto image=fonts.grab().toImage();
        if(fonts.count()>1){fonts.setCurrentIndex(fonts.currentIndex()==0?1:0);QTest::qWait(20);QVERIFY(image==fonts.grab().toImage());}
        QVERIFY_EXCEPTION_THROWN(applyFigmaComboBoxAppearance(&combo,"Medium"),std::invalid_argument);
    }
    void capturedShortcutKeepsNativeCaptureAndCandidateContract() {
        for(const auto* name:{"ClosePrimary","CloseDisabled","RecordingIndicator"}){QSvgRenderer svg(QString(":/figma/shortcut/%1.svg").arg(name));QVERIFY(svg.isValid());QCOMPARE(svg.defaultSize(),QString(name)=="RecordingIndicator"?QSize(8,8):QSize(16,16));}
        Z_KeySequenceEdit edit;edit.resize(280,72);edit.setKeySequence(QKeySequence("Ctrl+Shift+P"));
        auto* line=edit.findChild<QLineEdit*>("qt_keysequenceedit_lineedit");QVERIFY(line);const auto candidate=edit.keySequence();
        QSignalSpy changed(&edit,&QKeySequenceEdit::keySequenceChanged),finished(&edit,&QKeySequenceEdit::editingFinished);
        applyFigmaShortcutAppearance(&edit,"Recording");QCOMPARE(edit.keySequence(),candidate);QCOMPARE(changed.count(),0);QCOMPARE(finished.count(),0);QVERIFY(!edit.property("figmaShortcutRecording").toBool());
        edit.show();QTest::qWait(20);QCOMPARE(line->geometry(),QRect(0,0,280,40));
        auto* clear=edit.findChild<QAbstractButton*>("figmaShortcutClear");QVERIFY(clear);QCOMPARE(clear->geometry(),QRect(248,8,24,24));
        applyFigmaShortcutAppearance(&edit);edit.setFocus();QTest::keyClick(&edit,Qt::Key_K,Qt::ControlModifier);QTest::qWait(200);
        QCOMPARE(finished.count(),0);QVERIFY(edit.property("figmaShortcutRecording").toBool());
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(),1,2000);QVERIFY(!edit.property("figmaShortcutRecording").toBool());QCOMPARE(edit.keySequence().toString(QKeySequence::PortableText),QString("Ctrl+K"));
        QCOMPARE(edit.findChild<QLineEdit*>("qt_keysequenceedit_lineedit"),line);QCOMPARE(line->text(),edit.keySequence().toString(QKeySequence::NativeText));
        const auto portable=edit.keySequence().toString(QKeySequence::PortableText);QCOMPARE(QKeySequence::fromString(portable,QKeySequence::PortableText),edit.keySequence());
        setFigmaShortcutConflict(&edit,"Conflict: Save (demo registry)");QCOMPARE(edit.property("figmaShortcutVisualState").toString(),QString("Conflict"));QCOMPARE(edit.keySequence().toString(QKeySequence::PortableText),portable);
        changed.clear();finished.clear();QTest::mouseClick(clear,Qt::LeftButton);QVERIFY(edit.keySequence().isEmpty());QVERIFY(line->text().isEmpty());QCOMPARE(changed.count(),1);QCOMPARE(finished.count(),0);QVERIFY(!clear->isVisible());
        setFigmaShortcutConflict(&edit);QKeySequenceEdit baseline;baseline.show();
        for(auto* target:QList<QKeySequenceEdit*>{&baseline,&edit}) {
            target->clear();QTest::keyPress(target,Qt::Key_A);
            for(const auto key:{Qt::Key_B,Qt::Key_C,Qt::Key_D,Qt::Key_E})QTest::keyClick(target,key);
            QTest::keyRelease(target,Qt::Key_A);
        }
        QCOMPARE(edit.keySequence(),baseline.keySequence());QCOMPARE(edit.keySequence().count(),4);QCOMPARE(finished.count(),1);
        QTest::keyClick(&edit,Qt::Key_N,Qt::ControlModifier);QTest::qWait(20);QVERIFY(edit.property("figmaShortcutRecording").toBool());
        const auto pending=edit.keySequence();edit.setKeySequence(pending);QTest::qWait(20);QVERIFY(!edit.property("figmaShortcutRecording").toBool());QCOMPARE(edit.keySequence(),pending);
        applyFigmaShortcutAppearance(&edit,"Focus",false,false);QVERIFY(!clear->isVisible());QVERIFY(!edit.findChild<QWidget*>("figmaShortcutHint")->isVisible());QCOMPARE(edit.keySequence(),pending);
        edit.setEnabled(false);QTest::qWait(20);QCOMPARE(edit.property("figmaShortcutVisualState").toString(),QString("Disabled"));
        QVERIFY(QAccessible::queryAccessibleInterface(&edit));QVERIFY(QAccessible::queryAccessibleInterface(clear));
        edit.setEnabled(true);edit.clearFocus();applyFigmaShortcutAppearance(&edit,"Default",true,true,true);QTest::qWait(20);
        const auto snapshotA=line->grab().toImage();edit.setKeySequence(QKeySequence("Alt+X"));QTest::qWait(20);
        const auto snapshotB=line->grab().toImage();QVERIFY(snapshotA==snapshotB);QCOMPARE(edit.keySequence(),QKeySequence("Alt+X"));
        QVERIFY_EXCEPTION_THROWN(applyFigmaShortcutAppearance(&edit,"Pressed"),std::invalid_argument);
    }
    void capturedLineEditRetainsNativeEditingAndActions() {
        for(const auto* name:{"Prefix","Clear","Suffix"}){QSvgRenderer svg(QString(":/figma/lineedit/%1.svg").arg(name));QVERIFY(svg.isValid());QCOMPARE(svg.defaultSize(),QSize(16,16));}
        Z_LineEdit edit;edit.resize(320,32);edit.setPlaceholderText("Input text");edit.setText("sample");edit.setSelection(1,3);
        QSignalSpy changed(&edit,&QLineEdit::textChanged),edited(&edit,&QLineEdit::textEdited);
        applyFigmaLineEditAppearance(&edit,"Small","Error");QCOMPARE(edit.text(),QString("sample"));QCOMPARE(edit.selectedText(),QString("amp"));QCOMPARE(changed.count(),0);
        edit.show();edit.setFocus();QTest::qWait(20);edit.grab();QCOMPARE(edit.property("figmaLineEditVisualState").toString(),QString("Error"));
        QStyleOptionFrame option;option.initFrom(&edit);const QRect content=edit.style()->subElementRect(QStyle::SE_LineEditContents,&option,&edit);QCOMPARE(content.x(),7);
        applyFigmaLineEditAppearance(&edit,"Large");QCOMPARE(edit.height(),40);QCOMPARE(edit.selectedText(),QString("amp"));
        QTest::keyClicks(&edit,"X");QCOMPARE(edit.text(),QString("sXle"));QVERIFY(edited.count()>0);edit.undo();QCOMPARE(edit.text(),QString("sample"));edit.redo();QCOMPARE(edit.text(),QString("sXle"));
        edit.selectAll();QInputMethodEvent commit;commit.setCommitString(QString::fromUtf8("批次"));QApplication::sendEvent(&edit,&commit);QCOMPARE(edit.text(),QString::fromUtf8("批次"));
        edit.setReadOnly(true);QTest::keyClicks(&edit,"123");QCOMPARE(edit.text(),QString::fromUtf8("批次"));edit.setReadOnly(false);
        edit.setEchoMode(QLineEdit::Password);QVERIFY(edit.displayText()!=edit.text());applyFigmaLineEditAppearance(&edit,"Medium","Focus");QCOMPARE(edit.echoMode(),QLineEdit::Password);QCOMPARE(edit.text(),QString::fromUtf8("批次"));
        edit.setEchoMode(QLineEdit::Normal);edit.clear();edit.setMaxLength(4);QTest::keyClicks(&edit,"123456");QCOMPARE(edit.text(),QString("1234"));
        QIntValidator validator(0,9999,&edit);edit.setValidator(&validator);edit.selectAll();QTest::keyClicks(&edit,"abcd");QCOMPARE(edit.text(),QString("1234"));QVERIFY(edit.hasAcceptableInput());
        applyFigmaLineEditActions(&edit,true,true);const auto actions=edit.findChildren<QAction*>(QRegularExpression("figmaLineEdit.*Action"));QCOMPARE(actions.size(),2);
        auto* prefix=edit.findChild<QAction*>("figmaLineEditPrefixAction");auto* suffix=edit.findChild<QAction*>("figmaLineEditSuffixAction");QVERIFY(prefix&&suffix);
        QSignalSpy prefixClicks(prefix,&QAction::triggered);QToolButton* prefixButton=nullptr;for(auto* button:edit.findChildren<QToolButton*>())if(button->defaultAction()==prefix)prefixButton=button;
        QVERIFY(prefixButton);QTest::mouseClick(prefixButton,Qt::LeftButton);QCOMPARE(prefixClicks.count(),1);QCOMPARE(edit.text(),QString("1234"));
        edit.setClearButtonEnabled(true);QTest::qWait(200);QToolButton* clearButton=nullptr;
        for(auto* button:edit.findChildren<QToolButton*>())if(button->defaultAction()!=prefix&&button->defaultAction()!=suffix)clearButton=button;
        QVERIFY(clearButton);QVERIFY(clearButton->isVisible());QTest::mouseClick(clearButton,Qt::LeftButton);QVERIFY(edit.text().isEmpty());QTest::qWait(200);QVERIFY(!clearButton->isVisible());
        edit.setText("123");edit.setReadOnly(true);QTest::qWait(200);
        QLineEdit baseline("123");baseline.setClearButtonEnabled(true);baseline.setReadOnly(true);baseline.show();QTest::qWait(200);
        auto* nativeClear=baseline.findChild<QToolButton*>();QVERIFY(nativeClear);
        QCOMPARE(clearButton->isVisible(),nativeClear->isVisible());QCOMPARE(clearButton->isEnabled(),nativeClear->isEnabled());QVERIFY(!clearButton->isEnabled());
        QTest::mouseClick(clearButton,Qt::LeftButton);QCOMPARE(edit.text(),QString("123"));QCOMPARE(edit.validator(),static_cast<const QValidator*>(&validator));
        applyFigmaLineEditActions(&edit,false,false);QVERIFY(!prefix->isVisible());QVERIFY(!suffix->isVisible());QCOMPARE(edit.text(),QString("123"));
        edit.setLayoutDirection(Qt::RightToLeft);applyFigmaLineEditAppearance(&edit,"Small","Hover");QCOMPARE(edit.layoutDirection(),Qt::RightToLeft);QVERIFY(edit.isReadOnly());
        edit.setEnabled(false);edit.grab();QCOMPARE(edit.property("figmaLineEditVisualState").toString(),QString("Disabled"));
        edit.setValidator(nullptr);QVERIFY(QAccessible::queryAccessibleInterface(&edit));QVERIFY_EXCEPTION_THROWN(applyFigmaLineEditAppearance(&edit,"Small","Loading"),std::invalid_argument);
    }
    void capturedDialKeepsNativeAnglesAndSignals() {
        QFile evidence(":/figma/dial-visuals.json");QVERIFY(evidence.open(QIODevice::ReadOnly));
        const auto source=QJsonDocument::fromJson(evidence.readAll()).object();QCOMPARE(source["variants"].toArray().size(),8);
        for(const auto& entry:source["assets"].toArray()) {
            const auto name=entry.toObject()["name"].toString();QFile file(":/figma/dial/"+name+".svg");QVERIFY(file.open(QIODevice::ReadOnly));QVERIFY(file.size()>0);
            QXmlStreamReader xml(&file);QVERIFY(xml.readNextStartElement());const auto attrs=xml.attributes();
            QSvgRenderer svg(file.fileName());QVERIFY(svg.isValid());
            QVERIFY(qAbs(svg.viewBoxF().width()-attrs.value("width").toDouble())<0.002);
            QVERIFY(qAbs(svg.viewBoxF().height()-attrs.value("height").toDouble())<0.002);
        }
        Z_Dial dial;dial.resize(180,180);dial.setRange(0,100);dial.setValue(40);dial.setNotchesVisible(true);
        const auto policy=dial.focusPolicy();QSignalSpy changes(&dial,&QAbstractSlider::valueChanged);
        applyFigmaDialAppearance(&dial,"Dragging",true,true,"40");QCOMPARE(changes.count(),0);QVERIFY(!dial.isSliderDown());QCOMPARE(dial.focusPolicy(),policy);
        dial.show();QTest::qWait(20);dial.grab();QCOMPARE(dial.property("figmaDialCenter").toPointF(),QPointF(90,86));
        QCOMPARE(dial.property("figmaDialPointerAngle").toDouble(),-27.0);QCOMPARE(dial.property("figmaDialNotchCount").toInt(),12);
        dial.setWrapping(true);dial.grab();QCOMPARE(dial.property("figmaDialPointerAngle").toDouble(),54.0);
        applyFigmaDialAppearance(&dial);dial.setWrapping(false);dial.grab();QCOMPARE(dial.value(),40);
        QCOMPARE(dial.property("figmaDialCenter").toPointF(),QPointF(90,90));QCOMPARE(dial.property("figmaDialPointerAngle").toDouble(),-120.0);
        QTest::mouseClick(&dial,Qt::LeftButton,Qt::NoModifier,QPoint(160,90));QCOMPARE(dial.value(),80);
        dial.grab();QCOMPARE(dial.property("figmaDialPointerAngle").toDouble(),0.0);
        dial.setValue(40);dial.setTracking(false);QSignalSpy pressed(&dial,&QAbstractSlider::sliderPressed),released(&dial,&QAbstractSlider::sliderReleased),moved(&dial,&QAbstractSlider::sliderMoved);
        QTest::mousePress(&dial,Qt::LeftButton,Qt::NoModifier,QPoint(160,90));QCOMPARE(dial.value(),40);QCOMPARE(dial.sliderPosition(),80);QVERIFY(dial.isSliderDown());
        QMouseEvent move(QEvent::MouseMove,QPointF(90,20),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&dial,&move);
        QCOMPARE(dial.sliderPosition(),50);QCOMPARE(dial.value(),40);QVERIFY(moved.count()>0);
        QTest::mouseRelease(&dial,Qt::LeftButton,Qt::NoModifier,QPoint(90,20));QCOMPARE(dial.value(),50);QCOMPARE(pressed.count(),1);QCOMPARE(released.count(),1);
        dial.setTracking(true);dial.setWrapping(true);QTest::mouseClick(&dial,Qt::LeftButton,Qt::NoModifier,QPoint(160,90));QCOMPARE(dial.value(),75);
        dial.grab();QCOMPARE(dial.property("figmaDialPointerAngle").toDouble(),0.0);
        QDial baseline;baseline.resize(180,180);baseline.setRange(10,110);baseline.setInvertedAppearance(true);baseline.setValue(40);baseline.show();
        dial.setWrapping(false);dial.setRange(10,110);dial.setInvertedAppearance(true);dial.setValue(40);
        for(const QPoint point:{QPoint(160,90),QPoint(90,20),QPoint(20,90)}) {
            QTest::mouseClick(&baseline,Qt::LeftButton,Qt::NoModifier,point);QTest::mouseClick(&dial,Qt::LeftButton,Qt::NoModifier,point);QCOMPARE(dial.value(),baseline.value());
        }
        dial.setSingleStep(3);baseline.setSingleStep(3);dial.setPageStep(12);baseline.setPageStep(12);
        for(const auto key:{Qt::Key_Right,Qt::Key_Left,Qt::Key_PageUp,Qt::Key_End,Qt::Key_Home}) {
            QTest::keyClick(&baseline,key);QTest::keyClick(&dial,key);QCOMPARE(dial.value(),baseline.value());
        }
        dial.setNotchTarget(8.5);baseline.setNotchTarget(8.5);QCOMPARE(dial.notchSize(),baseline.notchSize());
        dial.setNotchesVisible(false);dial.grab();QCOMPARE(dial.property("figmaDialNotchCount").toInt(),0);
        dial.setNotchesVisible(true);dial.setRange(-1000000,1000000);dial.setSingleStep(1);dial.setNotchTarget(0.1);dial.grab();QVERIFY(dial.property("figmaDialNotchCount").toInt()<=2049);
        const int value=dial.value();applyFigmaDialAppearance(&dial,"Focus",false,true,"independent");QCOMPARE(dial.value(),value);QVERIFY(!dial.property("figmaDialAutomaticLabel").toBool());
        dial.setEnabled(false);dial.grab();QCOMPARE(dial.property("figmaDialVisualState").toString(),QString("Disabled"));
        dial.setEnabled(true);dial.setInvertedAppearance(false);dial.setRange(5,5);dial.grab();QCOMPARE(dial.value(),5);
        QVERIFY(QAccessible::queryAccessibleInterface(&dial));QVERIFY_EXCEPTION_THROWN(applyFigmaDialAppearance(&dial,"Hover"),std::invalid_argument);
    }
    void capturedScrollBarRetainsNativeSubcontrolsAndViewportSizing() {
        for(const auto* direction:{"Left","Right","Up","Down"})for(const auto* tone:{"Primary","Disabled"}) {
            QSvgRenderer svg(QString(":/figma/scrollbar/%1%2.svg").arg(direction,tone));QVERIFY(svg.isValid());QCOMPARE(svg.defaultSize(),QSize(16,16));
        }
        Z_ScrollBar bar;QCOMPARE(bar.focusPolicy(),Qt::NoFocus);bar.setOrientation(Qt::Horizontal);bar.resize(280,56);
        bar.setRange(0,100);bar.setPageStep(20);bar.setSingleStep(5);bar.setValue(40);
        QSignalSpy changes(&bar,&QAbstractSlider::valueChanged),pressed(&bar,&QAbstractSlider::sliderPressed),released(&bar,&QAbstractSlider::sliderReleased);
        applyFigmaScrollBarAppearance(&bar,"Dragging",true);QVERIFY(!bar.isSliderDown());QCOMPARE(changes.count(),0);bar.show();QTest::qWait(20);bar.grab();
        QCOMPARE(bar.property("figmaScrollBarHandleBounds").toRect(),QRect(86,20,72,16));
        QCOMPARE(bar.property("figmaScrollBarSubIconBounds").toRect(),QRect(10,20,16,16));QCOMPARE(bar.property("figmaScrollBarAddIconBounds").toRect(),QRect(254,20,16,16));
        applyFigmaScrollBarAppearance(&bar);
        const auto option=[&]{QStyleOptionSlider o;o.initFrom(&bar);o.orientation=bar.orientation();o.minimum=bar.minimum();o.maximum=bar.maximum();o.pageStep=bar.pageStep();o.sliderPosition=bar.sliderPosition();o.sliderValue=bar.value();o.upsideDown=bar.invertedAppearance();return o;};
        const auto rect=[&](QStyle::SubControl sub){const auto o=option();return bar.style()->subControlRect(QStyle::CC_ScrollBar,&o,sub,&bar);};
        QCOMPARE(rect(QStyle::SC_ScrollBarGroove),QRect(32,19,216,18));QCOMPARE(rect(QStyle::SC_ScrollBarSlider),QRect(104,20,36,16));
        QTest::mouseClick(&bar,Qt::LeftButton,Qt::NoModifier,rect(QStyle::SC_ScrollBarAddLine).center());QCOMPARE(bar.value(),45);
        QTest::mouseClick(&bar,Qt::LeftButton,Qt::NoModifier,rect(QStyle::SC_ScrollBarSubLine).center());QCOMPARE(bar.value(),40);
        QTest::mouseClick(&bar,Qt::LeftButton,Qt::NoModifier,rect(QStyle::SC_ScrollBarAddPage).center());QCOMPARE(bar.value(),60);
        bar.setValue(40);bar.setTracking(false);const auto handle=rect(QStyle::SC_ScrollBarSlider);
        QTest::mousePress(&bar,Qt::LeftButton,Qt::NoModifier,handle.center());QVERIFY(bar.isSliderDown());
        QMouseEvent move(QEvent::MouseMove,QPointF(handle.center()+QPoint(72,0)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(&bar,&move);
        QCOMPARE(bar.sliderPosition(),80);QCOMPARE(bar.value(),40);QTest::mouseRelease(&bar,Qt::LeftButton,Qt::NoModifier,handle.center()+QPoint(72,0));
        QCOMPARE(bar.value(),80);QCOMPARE(pressed.count(),1);QCOMPARE(released.count(),1);
        bar.setValue(40);applyFigmaScrollBarAppearance(&bar,{},false,false);QCOMPARE(bar.value(),40);QCOMPARE(bar.pageStep(),20);
        QVERIFY(rect(QStyle::SC_ScrollBarSubLine).isEmpty());QCOMPARE(rect(QStyle::SC_ScrollBarGroove),QRect(8,19,264,18));QCOMPARE(rect(QStyle::SC_ScrollBarSlider).width(),44);
        applyFigmaScrollBarAppearance(&bar);bar.setTracking(true);bar.setLayoutDirection(Qt::RightToLeft);
        QCOMPARE(rect(QStyle::SC_ScrollBarSubLine).x(),252);QCOMPARE(rect(QStyle::SC_ScrollBarSlider).x(),140);
        QTest::mouseClick(&bar,Qt::LeftButton,Qt::NoModifier,rect(QStyle::SC_ScrollBarSubLine).center());QCOMPARE(bar.value(),35);
        bar.setInvertedAppearance(true);QCOMPARE(rect(QStyle::SC_ScrollBarSubLine).x(),8);
        bar.setLayoutDirection(Qt::LeftToRight);bar.setInvertedAppearance(false);bar.setValue(10);
        const auto add=rect(QStyle::SC_ScrollBarAddLine).center();QTest::mousePress(&bar,Qt::LeftButton,Qt::NoModifier,add);QTest::qWait(650);QTest::mouseRelease(&bar,Qt::LeftButton,Qt::NoModifier,add);QVERIFY(bar.value()>15);
        bar.setOrientation(Qt::Vertical);bar.resize(100,260);applyFigmaScrollBarAppearance(&bar,"Default",true);bar.grab();
        QCOMPARE(bar.property("figmaScrollBarHandleBounds").toRect(),QRect(42,86,16,72));QCOMPARE(bar.property("figmaScrollBarSubIconBounds").toRect(),QRect(42,10,16,16));
        QCOMPARE(bar.property("figmaScrollBarAddIconBounds").toRect(),QRect(42,234,16,16));
        applyFigmaScrollBarAppearance(&bar);bar.setValue(40);bar.setFocusPolicy(Qt::StrongFocus);bar.setFocus();QTest::keyClick(&bar,Qt::Key_Down);QCOMPARE(bar.value(),45);
        bar.setEnabled(false);QTest::keyClick(&bar,Qt::Key_Down);QCOMPARE(bar.value(),45);
        bar.setEnabled(true);bar.setRange(-2147483647,2147483647);bar.setPageStep(2147483647);QVERIFY(!bar.grab().isNull());
        bar.setRange(7,7);QCOMPARE(rect(QStyle::SC_ScrollBarSlider).height(),rect(QStyle::SC_ScrollBarGroove).height());
        QScrollArea area;area.resize(220,180);auto* body=new QWidget;body->setFixedSize(600,700);area.setWidget(body);
        auto* hosted=new Z_ScrollBar;applyFigmaScrollBarAppearance(hosted);area.setVerticalScrollBar(hosted);area.show();QTest::qWait(30);
        QCOMPARE(hosted->minimumWidth(),0);QVERIFY(hosted->width()<=24);QCOMPARE(hosted->pageStep(),area.viewport()->height());QCOMPARE(hosted->maximum(),700-area.viewport()->height());
        auto* accessible=QAccessible::queryAccessibleInterface(&bar);QVERIFY(accessible);QCOMPARE(accessible->valueInterface()->currentValue().toInt(),7);
        QVERIFY_EXCEPTION_THROWN(applyFigmaScrollBarAppearance(&bar,"Focus"),std::invalid_argument);
    }
    void capturedSliderKeepsNativeTrackingAndGeometry() {
        for(const auto& item:QList<QPair<QString,int>>{{"Handle",18},{"HandleDisabled",18},{"HandleDragging",20},{"FocusRing",24}}) {
            QSvgRenderer svg(":/figma/slider/"+item.first+".svg");QVERIFY(svg.isValid());QCOMPARE(svg.defaultSize(),QSize(item.second,item.second));
        }
        Z_Slider slider;slider.setOrientation(Qt::Horizontal);slider.setRange(0,100);slider.setValue(40);slider.setPageStep(10);
        slider.setTickPosition(QSlider::TicksBelow);slider.setTickInterval(10);slider.resize(280,80);
        QSignalSpy changed(&slider,&QAbstractSlider::valueChanged),pressed(&slider,&QAbstractSlider::sliderPressed),released(&slider,&QAbstractSlider::sliderReleased);
        applyFigmaSliderAppearance(&slider,"Dragging",true,true,"40");
        QVERIFY(!slider.isSliderDown());QVERIFY(!slider.hasFocus());QCOMPARE(changed.count(),0);
        slider.show();QTest::qWait(20);slider.grab();
        QCOMPARE(slider.property("figmaSliderHandleAssetBounds").toRectF(),QRectF(108,28,20,20));
        QCOMPARE(slider.property("figmaSliderFocusAssetBounds").toRectF(),QRectF(105,25,24,24));
        QCOMPARE(slider.property("figmaSliderActiveTrackBounds").toRectF(),QRectF(10,34,104,6));
        QCOMPARE(slider.property("figmaSliderBubbleBounds").toRectF(),QRectF(95,0,19,24));
        applyFigmaSliderAppearance(&slider,{},false,true,"independent label");QCOMPARE(slider.value(),40);
        const auto option=[&] {
            QStyleOptionSlider opt;opt.initFrom(&slider);opt.orientation=slider.orientation();opt.minimum=slider.minimum();opt.maximum=slider.maximum();
            opt.sliderPosition=slider.sliderPosition();opt.sliderValue=slider.value();opt.upsideDown=slider.orientation()==Qt::Horizontal?
                slider.invertedAppearance()!=(slider.layoutDirection()==Qt::RightToLeft):!slider.invertedAppearance();return opt;
        };
        auto opt=option();auto handle=slider.style()->subControlRect(QStyle::CC_Slider,&opt,QStyle::SC_SliderHandle,&slider);
        QCOMPARE(handle,QRect(105,28,18,18));
        slider.setTracking(false);QTest::mousePress(&slider,Qt::LeftButton,Qt::NoModifier,handle.center());QVERIFY(slider.isSliderDown());
        QMouseEvent move(QEvent::MouseMove,QPointF(handle.center()+QPoint(104,0)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(&slider,&move);QCOMPARE(slider.sliderPosition(),80);QCOMPARE(slider.value(),40);
        QTest::mouseRelease(&slider,Qt::LeftButton,Qt::NoModifier,handle.center()+QPoint(104,0));
        QCOMPARE(slider.value(),80);QVERIFY(!slider.isSliderDown());QCOMPARE(pressed.count(),1);QCOMPARE(released.count(),1);
        slider.setValue(40);slider.setLayoutDirection(Qt::RightToLeft);opt=option();
        handle=slider.style()->subControlRect(QStyle::CC_Slider,&opt,QStyle::SC_SliderHandle,&slider);QCOMPARE(handle.x(),157);
        slider.setInvertedAppearance(true);opt=option();
        handle=slider.style()->subControlRect(QStyle::CC_Slider,&opt,QStyle::SC_SliderHandle,&slider);QCOMPARE(handle.x(),105);
        slider.setEnabled(false);QTest::keyClick(&slider,Qt::Key_Right);QCOMPARE(slider.value(),40);
        slider.setEnabled(true);slider.setOrientation(Qt::Vertical);slider.setLayoutDirection(Qt::LeftToRight);slider.setInvertedAppearance(false);
        applyFigmaSliderAppearance(&slider,"Focus",true,true,"40");slider.resize(100,260);slider.grab();
        QCOMPARE(slider.property("figmaSliderHandleAssetBounds").toRectF(),QRectF(41,144,18,18));
        QCOMPARE(slider.property("figmaSliderBubbleBounds").toRectF(),QRectF(0,141,19,24));
        applyFigmaSliderAppearance(&slider);slider.setTracking(true);QTest::keyClick(&slider,Qt::Key_Up);QCOMPARE(slider.value(),41);
        slider.setInvertedControls(true);QTest::keyClick(&slider,Qt::Key_Up);QCOMPARE(slider.value(),40);
        slider.setRange(-2147483647,2147483647);slider.setValue(0);slider.setTickInterval(1);QVERIFY(!slider.grab().isNull());
        slider.setRange(7,7);QVERIFY(!slider.grab().isNull());QCOMPARE(slider.value(),7);
        auto* accessible=QAccessible::queryAccessibleInterface(&slider);QVERIFY(accessible);QVERIFY(accessible->valueInterface());
        QCOMPARE(accessible->valueInterface()->currentValue().toInt(),7);
        QVERIFY_EXCEPTION_THROWN(applyFigmaSliderAppearance(&slider,"Invented"),std::invalid_argument);
    }
    void capturedDatePickerRetainsSectionsAndCalendarPopup() {
        QFile source(":/figma/datetime-visuals.json");QVERIFY(source.open(QIODevice::ReadOnly));
        const auto metadata=QJsonDocument::fromJson(source.readAll()).object();
        QCOMPARE(metadata["status"].toString(),QString("CAPTURED_LIGHT_NOT_DESIGN_APPROVED"));
        QCOMPARE(metadata["sets"].toArray().size(),3);
        for(const auto& set:metadata["sets"].toArray())QCOMPARE(set.toObject()["variants"].toArray().size(),8);
        for(const auto* tone:{"Primary","Disabled"}) {
            QSvgRenderer svg(QString(":/figma/datetime/Calendar%1.svg").arg(tone));
            QVERIFY(svg.isValid());QCOMPARE(svg.defaultSize(),QSize(16,16));
        }
        Z_DateEdit date;QCOMPARE(date.property("designSystemStyleOwner").toString(),QString("tokens"));
        date.setLocale(QLocale::c());date.setDisplayFormat("yyyy-MM-dd");
        date.setDateRange(QDate(2026,1,1),QDate(2026,12,31));date.setDate(QDate(2026,10,7));
        auto* editor=date.findChild<QLineEdit*>();date.resize(280,32);date.setCalendarPopup(true);
        applyFigmaDateTimeAppearance(&date,"Default");date.show();QTest::qWait(30);
        QCOMPARE(date.property("designSystemStyleOwner").toString(),QString("figma-date-captured"));
        QCOMPARE(date.height(),32);QCOMPARE(editor->geometry(),QRect(11,1,220,30));
        QCOMPARE(date.property("figmaDateCalendarAssetBounds").toRectF(),QRectF(252,8,16,16));
        QStyleOptionComboBox combo;combo.initFrom(&date);combo.editable=true;
        const auto arrow=date.style()->subControlRect(QStyle::CC_ComboBox,&combo,QStyle::SC_ComboBoxArrow,&date);
        QCOMPARE(arrow,QRect(239,0,42,32));
        QTest::mouseClick(&date,Qt::LeftButton,Qt::NoModifier,arrow.center());QTest::qWait(20);
        auto* calendar=date.calendarWidget();QVERIFY(calendar);QVERIFY(calendar->isVisible());
        calendar->setSelectedDate(QDate(2026,10,8));
        QMetaObject::invokeMethod(calendar,"activated",Qt::DirectConnection,Q_ARG(QDate,QDate(2026,10,8)));
        QCoreApplication::processEvents();QCOMPARE(date.date(),QDate(2026,10,8));
        calendar->window()->hide();date.setReadOnly(true);
        QTest::mouseClick(&date,Qt::LeftButton,Qt::NoModifier,arrow.center());QVERIFY(!calendar->isVisible());
        date.setReadOnly(false);date.setCalendarPopup(false);applyFigmaDateTimeAppearance(&date,"Default");
        date.setCurrentSection(QDateTimeEdit::DaySection);date.setDate(QDate(2026,10,7));
        QStyleOptionSpinBox spin;spin.initFrom(&date);
        auto up=date.style()->subControlRect(QStyle::CC_SpinBox,&spin,QStyle::SC_SpinBoxUp,&date);
        QCOMPARE(up,QRect(239,0,42,16));
        QCOMPARE(date.property("figmaDateUpAssetBounds").toRectF(),QRectF(254,1.75,16,16));
        QTest::mouseClick(&date,Qt::LeftButton,Qt::NoModifier,up.center());QCOMPARE(date.date(),QDate(2026,10,8));
        date.setReadOnly(true);QTest::keyClick(&date,Qt::Key_Up);QCOMPARE(date.date(),QDate(2026,10,8));
        date.setReadOnly(false);date.setDate(QDate(2026,12,31));date.stepUp();QCOMPARE(date.date(),QDate(2026,12,31));
        date.setLayoutDirection(Qt::RightToLeft);QCoreApplication::processEvents();
        up=date.style()->subControlRect(QStyle::CC_SpinBox,&spin,QStyle::SC_SpinBoxUp,&date);
        QCOMPARE(up,QRect(-1,0,42,16));QCOMPARE(date.findChild<QLineEdit*>(),editor);
    }
    void capturedDateTimeAndTimeKeepNativeValueAuthority() {
        Z_DateTimeEdit date;date.setLocale(QLocale::c());date.setDisplayFormat(QString::fromUtf8("yyyy-MM-dd '·' HH:mm"));
        date.setDateTime(QDateTime(QDate(2026,10,7),QTime(14,30)));date.resize(336,32);
        const auto original=date.dateTime();const auto spec=date.timeSpec();auto* editor=date.findChild<QLineEdit*>();
        QSignalSpy values(&date,&QDateTimeEdit::dateTimeChanged);
        applyFigmaDateTimeAppearance(&date,"Focus"," CST");
        QCOMPARE(date.dateTime(),original);QCOMPARE(date.timeSpec(),spec);QCOMPARE(values.count(),0);QVERIFY(!date.hasFocus());
        QCOMPARE(date.property("figmaDatePreviewTimeSpec").toString(),QString(" CST"));
        setDateTimeDisplayFormatPreservingValue(&date,"yyyy-MM-dd");applyFigmaDateTimeAppearance(&date,"Default"," CST");
        QCOMPARE(date.dateTime(),original);QCOMPARE(date.findChild<QLineEdit*>(),editor);
        QCOMPARE(values.count(),0);date.setCurrentSection(QDateTimeEdit::DaySection);date.stepUp();
        QCOMPARE(date.date(),QDate(2026,10,8));QCOMPARE(date.time(),QTime(14,30));date.setDateTime(original);
        date.setDisplayFormat("dd.MM.yyyy HH:mm");applyFigmaDateTimeAppearance(&date);
        QVERIFY(!date.property("figmaDateAuthoredFormat").toBool());QVERIFY(date.text().contains("07.10.2026"));
        Z_TimeEdit time;time.setLocale(QLocale::c());time.setDisplayFormat("HH:mm:ss");time.setTime(QTime(14,30,45));time.resize(280,32);
        auto* timeEditor=time.findChild<QLineEdit*>();applyFigmaDateTimeAppearance(&time,"Default");
        time.setDisplayFormat("hh:mm:ss AP");QCOMPARE(time.time(),QTime(14,30,45));QVERIFY(time.text().contains("02:30:45 PM"));
        time.setDisplayFormat("hh:mm AP");QCOMPARE(time.time(),QTime(14,30,45));QVERIFY(!(time.displayedSections()&QDateTimeEdit::SecondSection));
        time.setCurrentSection(QDateTimeEdit::HourSection);time.stepUp();QCOMPARE(time.time(),QTime(15,30,45));
        time.setTimeRange(QTime(8,0),QTime(18,0));time.setTime(QTime(19,0));QCOMPARE(time.time(),QTime(18,0));
        time.setReadOnly(true);QTest::keyClick(&time,Qt::Key_Down);QCOMPARE(time.time(),QTime(18,0));
        QCOMPARE(time.findChild<QLineEdit*>(),timeEditor);QCOMPARE(QFontInfo(timeEditor->font()).family(),QString("HarmonyOS Sans"));
        QVERIFY_EXCEPTION_THROWN(applyFigmaDateTimeAppearance(&time,"Invented"),std::invalid_argument);
    }
    void capturedSpinBoxRetainsNativeEditingAndStepping() {
        QFile source(":/figma/spinbox-visuals.json");QVERIFY(source.open(QIODevice::ReadOnly));
        const auto metadata=QJsonDocument::fromJson(source.readAll()).object();
        QCOMPARE(metadata["status"].toString(),QString("CAPTURED_LIGHT_NOT_DESIGN_APPROVED"));
        QCOMPARE(metadata["sets"].toArray().size(),2);
        for(const auto& set:metadata["sets"].toArray())QCOMPARE(set.toObject()["variants"].toArray().size(),16);
        for(const auto& name:{"PlusPrimary","PlusDisabled","MinusPrimary","MinusDisabled","UpPrimary","UpDisabled","DownPrimary","DownDisabled"}) {
            QSvgRenderer svg(QString(":/figma/spinbox/%1.svg").arg(name));QVERIFY(svg.isValid());QCOMPARE(svg.defaultSize(),QSize(16,16));
        }
        Z_SpinBox spin;spin.setRange(-200,200);spin.setValue(128);spin.setPrefix(QString::fromUtf8("¥"));spin.setSuffix(" units");
        auto* editor=spin.findChild<QLineEdit*>();QVERIFY(editor);spin.resize(244,32);
        QSignalSpy values(&spin,qOverload<int>(&QSpinBox::valueChanged));
        for(const auto& mode:{"UpDown","PlusMinus","None","SplitPlusMinus"}) {
            applyFigmaSpinBoxAppearance(&spin,mode,"Default");QCOMPARE(spin.findChild<QLineEdit*>(),editor);QCOMPARE(spin.value(),128);
        }
        QCOMPARE(values.count(),0);QCOMPARE(spin.height(),32);QCOMPARE(spin.buttonSymbols(),QAbstractSpinBox::PlusMinus);
        spin.show();QTest::qWait(30);
        QCOMPARE(editor->geometry(),QRect(46,1,154,30));
        QStyleOptionSpinBox option;option.initFrom(&spin);option.buttonSymbols=spin.buttonSymbols();
        const auto step=[&](bool up){return spin.style()->subControlRect(QStyle::CC_SpinBox,&option,up?QStyle::SC_SpinBoxUp:QStyle::SC_SpinBoxDown,&spin);};
        QCOMPARE(step(false),QRect(1,0,32,32));QCOMPARE(step(true),QRect(211,0,32,32));
        QTest::mouseClick(&spin,Qt::LeftButton,Qt::NoModifier,step(true).center());QCOMPARE(spin.value(),129);
        QTest::mouseClick(&spin,Qt::LeftButton,Qt::NoModifier,step(false).center());QCOMPARE(spin.value(),128);
        spin.setReadOnly(true);QTest::mouseClick(&spin,Qt::LeftButton,Qt::NoModifier,step(true).center());QCOMPARE(spin.value(),128);
        QTest::keyClick(&spin,Qt::Key_Up);QCOMPARE(spin.value(),128);spin.setReadOnly(false);
        spin.setValue(200);QTest::mouseClick(&spin,Qt::LeftButton,Qt::NoModifier,step(true).center());QCOMPARE(spin.value(),200);
        spin.setWrapping(true);QTest::mouseClick(&spin,Qt::LeftButton,Qt::NoModifier,step(true).center());QCOMPARE(spin.value(),-200);
        spin.setWrapping(false);spin.setValue(128);spin.setLayoutDirection(Qt::RightToLeft);QCoreApplication::processEvents();
        QCOMPARE(step(true),QRect(1,0,32,32));QCOMPARE(step(false),QRect(211,0,32,32));
        QTest::mouseClick(&spin,Qt::LeftButton,Qt::NoModifier,step(true).center());QCOMPARE(spin.value(),129);
        spin.setLayoutDirection(Qt::LeftToRight);spin.setFocus();spin.selectAll();QTest::keyClicks(&spin,"42");QTest::keyClick(&spin,Qt::Key_Return);
        QCOMPARE(spin.value(),42);QVERIFY(spin.hasAcceptableInput());QCOMPARE(spin.prefix(),QString::fromUtf8("¥"));
        QCOMPARE(QFontInfo(editor->font()).family(),QString("HarmonyOS Sans"));QCOMPARE(editor->font().weight(),int(QFont::Medium));
        auto* accessible=QAccessible::queryAccessibleInterface(&spin);QVERIFY(accessible);QVERIFY(accessible->valueInterface());
        QCOMPARE(accessible->valueInterface()->currentValue().toInt(),42);
        QVERIFY_EXCEPTION_THROWN(applyFigmaSpinBoxAppearance(&spin,"Invented"),std::invalid_argument);
    }
    void capturedDoubleSpinBoxKeepsDecimalAndLocaleAuthority() {
        Z_DoubleSpinBox spin;spin.setRange(0,1000);spin.setDecimals(2);spin.setSingleStep(.25);spin.setValue(72.5);
        spin.setPrefix("$");spin.setSuffix(" %");spin.resize(260,32);applyFigmaSpinBoxAppearance(&spin,"PlusMinus","Focus");
        QCOMPARE(spin.value(),72.5);QVERIFY(!spin.hasFocus());
        spin.show();QTest::qWait(20);auto* editor=spin.findChild<QLineEdit*>();
        QCOMPARE(spin.property("figmaSpinUpAssetBounds").toRectF(),QRectF(230.5,-.25,16,16));
        spin.setLocale(QLocale("de_DE"));QVERIFY(spin.text().contains("72,50"));
        spin.setFocus();spin.selectAll();QTest::keyClicks(&spin,"10,25");QTest::keyClick(&spin,Qt::Key_Return);QCOMPARE(spin.value(),10.25);
        QTest::keyClick(&spin,Qt::Key_Up);QCOMPARE(spin.value(),10.5);
        spin.setDecimals(1);spin.setValue(2.36);QCOMPARE(spin.value(),2.4);
        applyFigmaSpinBoxAppearance(&spin,"SplitPlusMinus");QCOMPARE(spin.findChild<QLineEdit*>(),editor);
        spin.setReadOnly(true);QTest::keyClick(&spin,Qt::Key_Up);QCOMPARE(spin.value(),2.4);
        spin.setReadOnly(false);spin.setEnabled(false);QTest::keyClick(&spin,Qt::Key_Up);QCOMPARE(spin.value(),2.4);
    }
    void capturedRadioSourceRemainsUnapprovedAndNative() {
        QFile source(":/figma/radio-visuals.json");QVERIFY(source.open(QIODevice::ReadOnly));
        const auto document=QJsonDocument::fromJson(source.readAll()).object();
        QCOMPARE(document["status"].toString(),QString("CAPTURED_LIGHT_NOT_DESIGN_APPROVED"));
        const auto sets=document["sets"].toArray();QCOMPARE(sets.size(),2);
        QCOMPARE(sets[0].toObject()["variants"].toArray().size(),10);
        QCOMPARE(sets[1].toObject()["variants"].toArray().size(),4);
        for(const auto& item:document["assets"].toArray()) {
            const auto asset=item.toObject();QSvgRenderer svg(":/figma/radio/"+asset["name"].toString()+".svg");
            QVERIFY(svg.isValid());const auto size=asset["rootSize"].toArray();
            QCOMPARE(svg.defaultSize(),QSize(size[0].toInt(),size[1].toInt()));
        }
        Z_RadioButton radio;radio.setText("Native option");radio.setChecked(true);
        applyFigmaRadioAppearance(&radio,"Pressed");QVERIFY(!radio.isDown());
        QCOMPARE(radio.text(),QString("Native option"));QVERIFY(radio.isChecked());
        QCOMPARE(radio.property("nativeQtClass").toString(),QString("QRadioButton"));
    }
    void currentFigmaCatalog() {
        QWidget page;
        new QVBoxLayout(&page);
        populateCatalog(&page);
        const auto descriptors = commonWidget::designComponentDescriptors();
        const auto figmaPages = commonWidget::figmaComponentPages();
        const auto figmaCategories = commonWidget::figmaComponentCategories();
        int expectedInstantiable = 0;
        int expectedSetCount = 0;
        int expectedVariantCount = 0;
        for (const auto& descriptor : descriptors)
            expectedInstantiable += descriptor.instantiable ? 1 : 0;
        for (const auto& figmaPage : figmaPages) {
            expectedSetCount += figmaPage.componentSets.size();
            for (const auto& componentSet : figmaPage.componentSets)
                expectedVariantCount += componentSet.variants.size();
        }
        QCOMPARE(page.property("componentCount").toInt(), descriptors.size());
        QCOMPARE(page.property("instantiableCount").toInt(), expectedInstantiable);
        QCOMPARE(page.property("contractOnlyCount").toInt(),
                 descriptors.size() - expectedInstantiable);
        QCOMPARE(page.property("figmaFileKey").toString(), QString("w92BKW1EcutKmTXkwzNHbf"));
        QCOMPARE(page.property("figmaPageCount").toInt(), figmaPages.size());
        QCOMPARE(page.property("figmaComponentSetCount").toInt(), expectedSetCount);
        QCOMPARE(page.property("figmaVariantCount").toInt(), expectedVariantCount);
        auto* tabs = page.findChild<QTabWidget*>("designCatalogTabs");
        auto* tree = page.findChild<QTreeWidget*>("figmaComponentTree");
        auto* setTable = page.findChild<QTableWidget*>("figmaComponentSetTable");
        QVERIFY(tabs && tree && setTable);
        QCOMPARE(tabs->count(), 2);
        QCOMPARE(tabs->tabText(0), QString::fromUtf8("Figma 组件结构"));
        QCOMPARE(tabs->tabText(1), QString::fromUtf8("开发映射"));
        QCOMPARE(tree->topLevelItemCount(), figmaCategories.size());
        QVERIFY(tree->topLevelItem(0)->text(0).startsWith(
            QString::fromUtf8("20.1 · Navigation / 导航 (")));
        int figmaPageCount = 0;
        QTreeWidgetItem* checkboxPage = nullptr;
        for (int group = 0; group < tree->topLevelItemCount(); ++group) {
            auto* branch = tree->topLevelItem(group);
            figmaPageCount += branch->childCount();
            for (int row = 0; row < branch->childCount(); ++row) {
                if (branch->child(row)->data(0, Qt::UserRole).toString() == "234:2717")
                    checkboxPage = branch->child(row);
            }
        }
        QCOMPARE(figmaPageCount, figmaPages.size());
        QVERIFY(checkboxPage);
        tree->setCurrentItem(checkboxPage);
        QCoreApplication::processEvents();
        QCOMPARE(setTable->property("figmaPageId").toString(), QString("234:2717"));
        QCOMPARE(setTable->rowCount(), 2);
        QCOMPARE(setTable->item(0, 0)->text(), QString("DSM/Checkbox"));
        QCOMPARE(setTable->item(0, 2)->text(),
                 QString("Size=LG/SM · Value=Unchecked/Checked/Indeterminate · "
                         "State=Default/Hover/Focus-visible/Disabled/Pressed"));
        QCOMPARE(setTable->item(0, 3)->text(), QString("30"));
        auto* layouts=page.findChild<QTreeWidget*>("figmaGalleryLayoutTree");
        QVERIFY(layouts);QVERIFY(layouts->property("sourceCaptured").toBool());
        QCOMPARE(layouts->topLevelItemCount(),1);
        auto* gallery=layouts->topLevelItem(0);
        QCOMPARE(gallery->data(0,Qt::UserRole).toString(),QString("2015:20254"));
        QCOMPARE(gallery->text(3),QString("64, 1876 / 1312 × 991"));
        QCOMPARE(gallery->childCount(),3);
        QCOMPARE(gallery->child(1)->text(0),QString("DSM/Checkbox"));
        QCOMPARE(gallery->child(1)->childCount(),30);
        QCOMPARE(gallery->child(2)->childCount(),4);
        auto* table = page.findChild<QTableWidget*>("designComponentCatalogTable");
        QVERIFY(table);
        QCOMPARE(table->rowCount(), descriptors.size());
        QSet<QString> identities;
        QSet<QString> sdkClasses;
        for (int row = 0; row < table->rowCount(); ++row) {
            const auto identity = table->item(row, 0)->text();
            const auto sdkClass = table->item(row, 1)->text();
            QVERIFY(identity.startsWith("ComponentLab/"));
            QVERIFY(!identities.contains(identity));
            identities.insert(identity);
            if (!sdkClass.isEmpty()) {
                QVERIFY(sdkClass.startsWith("commonWidget::Z_"));
                QVERIFY(!sdkClasses.contains(sdkClass));
                sdkClasses.insert(sdkClass);
            }
        }
    }
    void compositeRuntimeVariantsAreObservable() {
        Z_Switch toggle;
        toggle.resize(44, 24);
        toggle.setVariantTuple("Size=Medium, Value=On, State=Default");
        const auto normal = toggle.grab().toImage();
        toggle.setVariantTuple("Size=Medium, Value=On, State=Hover");
        const auto hover = toggle.grab().toImage();
        toggle.setVariantTuple("Size=Medium, Value=On, State=Pressed");
        QVERIFY(normal != hover);
        QVERIFY(hover != toggle.grab().toImage());

        Z_Form form;
        form.setVariantTuple("Layout=Vertical, Density=Compact, State=Error");
        const auto fields = form.findChildren<QLineEdit*>();
        QCOMPARE(fields.size(), 2);
        for (auto* field : fields) {
            QCOMPARE(field->property("validationState").toString(), QString("Error"));
            QVERIFY(field->styleSheet().contains("#D14343"));
        }

        Z_DataTable table;
        table.setVariantTuple("Size=Large, State=Hover");
        QCOMPARE(table.verticalHeader()->defaultSectionSize(), 44);
        QVERIFY(table.item(0, 0)->background().style() != Qt::NoBrush);
        table.setVariantTuple("Size=Small, State=Selected-hover");
        QCOMPARE(table.verticalHeader()->defaultSectionSize(), 28);
        QVERIFY(table.selectionModel()->isRowSelected(0, QModelIndex()));
        QVERIFY(table.item(0, 0)->background().style() != Qt::NoBrush);

        Z_Collapse collapse;
        collapse.resize(584, 400);
        collapse.setVariantTuple("Mode=Multi, Border=Bordered, State=Default");
        const auto image = collapse.grab().toImage();
        if (profile.startsWith("light")) {
            const auto bodyPixel = image.pixelColor(300, 260);
            const auto diagnostic = QStringLiteral(
                "Light profile Collapse body must remain legible: pixel=%1 window=%2 text=%3 base=%4")
                .arg(bodyPixel.name(QColor::HexArgb),
                     collapse.palette().color(QPalette::Window).name(QColor::HexArgb),
                     collapse.palette().color(QPalette::Text).name(QColor::HexArgb),
                     collapse.palette().color(QPalette::Base).name(QColor::HexArgb));
            QVERIFY2(bodyPixel.lightness() > 180, qPrintable(diagnostic));
        }
    }
    void externalPixsoButton() {
        const auto input = qEnvironmentVariable("PIXSO_BUTTON_CONTRACT");
        if (input.isEmpty())
            QSKIP("Set PIXSO_BUTTON_CONTRACT for the captured Pixso pilot");
        const auto contract = parse(read(input));
        QVERIFY2(validate(contract, profile).isEmpty(), "Pixso contract validation failed");
        std::unique_ptr<QWidget> page(preview(contract, profile));
        auto* button = page->findChild<Z_PushButton*>("pixsoProcessButton");
        QVERIFY(button);
        QCOMPARE(button->text(), QString::fromUtf8("执行工艺"));
        QVERIFY(button->isEnabled());
        QVERIFY(!button->isIconOnly());
        page->resize(360, 120);
        page->show();
        QTest::qWait(50);
        QSignalSpy clicked(button, &QPushButton::clicked);
        QTest::mouseClick(button, Qt::LeftButton);
        QCOMPARE(clicked.count(), 1);
        const auto capture = qEnvironmentVariable("PIXSO_BUTTON_CAPTURE");
        if (!capture.isEmpty())
            QVERIFY(page->grab().save(capture));
    }
    void canonicalFilesAndCompiledUi() {
        const auto contract = example();
        QVERIFY2(validate(contract, profile).isEmpty(),
                 qPrintable(validate(contract, profile).join('\n')));
        const auto files = generate(contract, profile);
        QCOMPARE(files.size(), 7);
        QXmlStreamReader xml(files["MappingGenerated.ui"]);
        while (!xml.atEnd())
            xml.readNext();
        QVERIFY(!xml.hasError());
        QVERIFY(files["MappingGenerated.cpp"].contains("design_mapping::mount"));
        QVERIFY(!files["MappingGenerated.qss"].contains("#6235"));
        QCOMPARE(generate(contract, profile), files);
        MappingGenerated compiled(profile);
        std::unique_ptr<QWidget> loaded(preview(contract, profile));
        for (auto* root : {static_cast<QWidget*>(&compiled), loaded.get()}) {
            QVERIFY(root->findChild<Z_PushButton*>("submitButton"));
            QVERIFY(root->findChild<Z_PushButton*>("searchButton")->isIconOnly());
            QVERIFY(root->findChild<Z_CheckBox*>("qualityCheck")->isChecked());
            QCOMPARE(root->findChild<Z_InputField*>("lotInput")->value(), QString("LOT-2026-0917"));
            QCOMPARE(root->findChild<Z_Select*>("chamberSelect")->values(), QStringList{"etch-01"});
            const field::Tokens values(profile);
            auto* layout = root->findChild<QBoxLayout*>("layout_MappingExample");
            QCOMPARE(layout->spacing(), values.metric("labelGap"));
            QCOMPARE(layout->contentsMargins().left(), values.metric("tagPadding"));
            QCOMPARE(root->findChild<Z_InputField*>("lotInput")->control()->controlSize(),
                     QString("md"));
        }
    }
    void capturedCheckboxAppearanceExportsAndRuns() {
        auto contract=mutate("qualityCheck",[](QJsonObject& node) {
            auto properties=node["properties"].toObject();
            properties["appearance"]="figma-light";
            properties["checked"]=false;
            properties["indeterminate"]=true;
            node["properties"]=properties;
        });
        QVERIFY(validate(contract,profile).isEmpty());
        const auto generated=generate(contract,profile);
        QVERIFY(generated["MappingGenerated.cpp"].contains("figma-light"));
        auto restored=parse(generated["ui-contract.json"]);
        std::unique_ptr<QWidget> page(preview(restored,profile));
        auto* checkbox=page->findChild<Z_CheckBox*>("qualityCheck");
        QVERIFY(checkbox);QCOMPARE(checkbox->appearance(),QString("figma-light"));
        QVERIFY(checkbox->isIndeterminate());QVERIFY(!checkbox->isChecked());
        QVERIFY(!checkbox->isTristate());
        // Unauthored Input combinations must not silently normalize to captured appearance.
        const auto invalid=mutate("lotInput",[](QJsonObject& node) {
            auto properties=node["properties"].toObject();properties["appearance"]="figma-light";
            properties["variant"]="underlined";node["properties"]=properties;
        });
        QVERIFY(!validate(invalid,profile).isEmpty());
    }
    void capturedButtonAppearanceExportsAndRuns() {
        auto contract=mutate("submitButton",[](QJsonObject& node) {
            auto properties=node["properties"].toObject();
            properties["appearance"]="figma-light";properties["ghost"]=false;
            properties["danger"]=false;node["properties"]=properties;
        });
        QVERIFY(validate(contract,profile).isEmpty());
        const auto generated=generate(contract,profile);
        QVERIFY(generated["MappingGenerated.cpp"].contains("figma-light"));
        std::unique_ptr<QWidget> page(preview(parse(generated["ui-contract.json"]),profile));
        auto* button=page->findChild<Z_PushButton*>("submitButton");QVERIFY(button);
        QVERIFY(button->property("figmaAppearance").toBool());
        const auto unsupported=mutate("submitButton",[](QJsonObject& node) {
            auto p=node["properties"].toObject();p["appearance"]="figma-light";
            p["type"]="Default";p["danger"]=true;node["properties"]=p;
        });
        QVERIFY(!validate(unsupported,profile).isEmpty());
    }
    void capturedInputExportsAndRunsWithOriginalAssets() {
        QFile source(":/figma/input-visuals.json");QVERIFY(source.open(QIODevice::ReadOnly));
        const auto evidence=QJsonDocument::fromJson(source.readAll()).object();
        QCOMPARE(evidence["variantCount"].toInt(),108);
        QCOMPARE(evidence["inputCount"].toInt(),96);
        QCOMPARE(evidence["designApproval"].toString(),QString("NOT_DESIGN_APPROVED"));
        for(const auto& item:evidence["assets"].toArray()) {
            QSvgRenderer svg(item.toObject()["resource"].toString());
            QVERIFY(svg.isValid());QCOMPARE(svg.defaultSize(),QSize(16,16));
        }
        auto contract=mutate("lotInput",[](QJsonObject& node) {
            auto p=node["properties"].toObject();p["appearance"]="figma-light";
            p["kind"]="password";p["passwordVisible"]=true;p["value"]="secret42";node["properties"]=p;
        });
        QVERIFY2(validate(contract,profile).isEmpty(),qPrintable(validate(contract,profile).join('\n')));
        const auto generated=generate(contract,profile);
        // Export embeds the contract and invokes mount(), rather than emitting each setter.
        QVERIFY(generated["MappingGenerated.cpp"].contains("passwordVisible"));
        std::unique_ptr<QWidget> page(preview(parse(generated["ui-contract.json"]),profile));
        auto* input=page->findChild<Z_InputField*>("lotInput");QVERIFY(input);
        QCOMPARE(input->appearance(),QString("figma-light"));QVERIFY(input->control()->isFigmaInputAppearance());
        QVERIFY(input->isPasswordVisible());QCOMPARE(input->lineEdit()->echoMode(),QLineEdit::Normal);
        QCOMPARE(input->value(),QString("secret42"));
    }
    void actualInteractions() {
        std::unique_ptr<QWidget> page(preview(example(), profile));
        page->resize(600, 500);
        page->show();
        QTest::qWait(40);
        auto* b = page->findChild<Z_PushButton*>("submitButton");
        auto* icon = page->findChild<Z_PushButton*>("searchButton");
        const field::Tokens metrics(profile);
        QCOMPARE(icon->mapTo(page.get(), QPoint()).x() - b->mapTo(page.get(), QPoint()).x() -
                     b->width(),
                 metrics.metric("labelGap"));
        QSignalSpy clicked(b, &QPushButton::clicked);
        QTest::mouseClick(b, Qt::LeftButton);
        QCOMPARE(clicked.count(), 1);
        auto* c = page->findChild<Z_CheckBox*>("qualityCheck");
        QTest::mouseClick(c, Qt::LeftButton);
        QVERIFY(!c->isChecked());
        auto* input = page->findChild<Z_InputField*>("lotInput");
        input->lineEdit()->selectAll();
        QTest::keyClicks(input->lineEdit(), "LOT-002");
        QCOMPARE(input->value(), QString("LOT-002"));
        auto* select = page->findChild<Z_Select*>("chamberSelect");
        select->setValues({"etch-02"});
        QCOMPARE(select->values(), QStringList{"etch-02"});
    }
    void unsupportedInputsFailClosed() {
        const QList<QJsonObject> invalid{
            mutate("submitButton",
                   [](QJsonObject& n) {
                       auto p = n["properties"].toObject();
                       p["fill"] = "#ff0000";
                       n["properties"] = p;
                   }),
            mutate("submitButton",
                   [](QJsonObject& n) {
                       auto p = n["properties"].toObject();
                       p["disabled"] = "false";
                       n["properties"] = p;
                   }),
            mutate("submitButton",
                   [](QJsonObject& n) {
                       auto p = n["properties"].toObject();
                       p["leadingIcon"] = "https://example.org/icon.svg";
                       n["properties"] = p;
                   }),
            mutate("submitButton",
                   [](QJsonObject& n) { n["tokenRefs"] = QJsonArray{"arbitrary.color"}; }),
            mutate("submitButton", [](QJsonObject& n) { n["component"] = "Table"; }),
            mutate("qualityCheck", [](QJsonObject& n) { n["id"] = "submitButton"; }),
            mutate("qualityCheck",
                   [](QJsonObject& n) {
                       auto p = n["properties"].toObject();
                       p["controlSize"] = "md";
                       n["properties"] = p;
                   }),
            mutate("searchButton",
                   [](QJsonObject& n) {
                       auto p = n["properties"].toObject();
                       p["accessibleName"] = "";
                       n["properties"] = p;
                   }),
            mutate("MappingExample",
                   [](QJsonObject& n) {
                       auto p = n["layout"].toObject();
                       p["margin"] = 24;
                       n["layout"] = p;
                   }),
            mutate("chamberSelect", [](QJsonObject& n) {
                auto p = n["properties"].toObject();
                p["values"] = QJsonArray{"missing"};
                n["properties"] = p;
            })};
        QTemporaryDir directory;
        for (const auto& contract : invalid) {
            QVERIFY(!validate(contract, profile).isEmpty());
            QVERIFY_EXCEPTION_THROWN(exportFiles(contract, profile, directory.path()),
                                     std::invalid_argument);
        }
        QVERIFY(
            QDir(directory.path()).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty());
        QVERIFY_EXCEPTION_THROWN(parse("not JSON"), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(parse("[]"), std::invalid_argument);
    }
    void stateAndContentModes() {
        auto contract = mutate("searchButton", [](QJsonObject& n) {
            auto p = n["properties"].toObject();
            p["loading"] = true;
            n["properties"] = p;
        });
        std::unique_ptr<QWidget> page(preview(contract, profile));
        auto* icon = page->findChild<Z_PushButton*>("searchButton");
        QVERIFY(icon->isIconOnly());
        QVERIFY(!icon->isEnabled());
        icon->setLoading(false);
        QVERIFY(icon->isEnabled());
        auto disabled = mutate("lotInput", [](QJsonObject& n) {
            auto p = n["properties"].toObject();
            p["disabled"] = true;
            n["properties"] = p;
        });
        std::unique_ptr<QWidget> disabledPage(preview(disabled, profile));
        QVERIFY(!disabledPage->findChild<Z_InputField*>("lotInput")->isEnabled());
        for (auto material : surface::Catalog(profile).buttonPresetIds()) {
            auto input = mutate("submitButton", [material](QJsonObject& n) {
                auto p = n["properties"].toObject();
                p["appearance"] = material;
                n["properties"] = p;
            });
            if (validate(input, profile).isEmpty()) {
                std::unique_ptr<QWidget> materialPage(preview(input, profile));
                QVERIFY(materialPage);
            }
        }
    }
    void exportProvenanceAndNonOverwrite() {
        QTemporaryDir directory;
        const auto path = exportFiles(example(), profile, directory.path());
        const auto report = parse(read(path + "/mapping-report.json"));
        QCOMPARE(report["source"].toObject()["kind"].toString(), QString("localFixture"));
        QCOMPARE(report["tokenPipelineModified"].toBool(), false);
        const auto files = generate(example(), profile);
        for (auto it = files.begin(); it != files.end(); ++it)
            QCOMPARE(read(path + '/' + it.key()), it.value());
        auto fake = example();
        auto source = fake["source"].toObject();
        source["kind"] = "pixsoNormalized";
        fake["source"] = source;
        QVERIFY(!validate(fake, profile).isEmpty());
    }
    void canonicalButtonEvidence() {
        // Hash-bound review is simulated only inside this test's temporary directory.
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString sourceDir = QString::fromUtf8(TEST_SOURCE_DIR);
        const QString script = sourceDir + "/mapping/design/pixso_button.py";
        const QString index = sourceDir + "/../../DesignEvidence/packages/DesignTest-2-145/canonical/index.json";
        const QString rule = sourceDir + "/mapping/design/pixso-button/rule.json";
        const QString reviewFile = temp.path() + "/test-review.json";
        QProcess python;
        python.start(QString::fromUtf8(TEST_PYTHON), {"-B", script, index, rule, reviewFile, "--inspect"});
        QVERIFY(python.waitForFinished(30000));
        QVERIFY2(python.exitCode() == 0, python.readAllStandardError().constData());
        QFile review(reviewFile);
        QVERIFY(review.open(QIODevice::ReadOnly));
        const auto bytes = review.readAll();
        QVERIFY(bytes.size() < 200000);
        const auto hash = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
        const QString contractFile = temp.path() + "/test-contract.json";
        python.start(QString::fromUtf8(TEST_PYTHON), {"-B", script, index, rule, contractFile, "--reviewed-sha256", hash});
        QVERIFY(python.waitForFinished(30000));
        QVERIFY2(python.exitCode() == 0, python.readAllStandardError().constData());
        QFile file(contractFile);
        QVERIFY(file.open(QIODevice::ReadOnly));
        auto contract = parse(file.readAll());
        QVERIFY2(validate(contract, profile).isEmpty(), qPrintable(validate(contract, profile).join('\n')));
        const auto originalPage = contract["page"].toObject();
        auto child = originalPage["children"].toArray()[0].toObject();
        const auto identity = child["design"].toObject();
        QVERIFY(!identity.contains("instanceNodeId"));
        QCOMPARE(identity["variantNodeId"].toString(), QString("3:5686"));
        QCOMPARE(identity["nativeComponentKey"].toString(), QString("5db2a212943f439833dd3b8d6f8957d687a6edb1"));
        std::unique_ptr<QWidget> widget(preview(contract, profile));
        auto button = widget->findChild<QPushButton*>("pixsoProcessButton");
        QVERIFY(button);
        QCOMPARE(button->text(), QString::fromUtf8("执行工艺"));
        auto changedIdentity = identity;
        changedIdentity["nativeComponentKey"] = QString(40, '0');
        child["design"] = changedIdentity;
        auto changedPage = originalPage;
        changedPage["children"] = QJsonArray{child};
        contract["page"] = changedPage;
        QVERIFY(!validate(contract, profile).isEmpty());
        contract["page"] = originalPage;
        auto source = contract["source"].toObject();
        source["canonicalIndexSha256"] = QString(64, '0');
        contract["source"] = source;
        QVERIFY(!validate(contract, profile).isEmpty());
        source["kind"] = "pixsoNormalized";
        contract["source"] = source;
        QVERIFY(!validate(contract, profile).isEmpty());
    }
    void designIdentityAndSizedReferences() {
        auto live = example();
        live["source"] = QJsonObject{{"kind", "figmaNormalized"},
                                     {"documentUrl", "https://www.figma.com/design/test-fixture"},
                                     {"nodeId", "synthetic-page"},
                                     {"rawSha256", QString(64, '0')},
                                     {"reviewed", true}};
        QVERIFY(!validate(live, profile).isEmpty());
        // Synthetic identities exercise validation only; they are never design evidence.
        std::function<QJsonObject(QJsonObject)> identify = [&](QJsonObject node) {
            if (node.contains("children")) {
                QJsonArray children;
                for (const auto& child : node["children"].toArray())
                    children.append(identify(child.toObject()));
                node["children"] = children;
            } else {
                node["design"] =
                    QJsonObject{{"instanceNodeId", "synthetic-" + node["id"].toString()},
                                {"componentKey", "synthetic-component"},
                                {"libraryVersion", "test-only"},
                                {"alias", node["component"]}};
            }
            return node;
        };
        live["page"] = identify(live["page"].toObject());
        auto attachSnapshot = [](QJsonObject source, const QJsonObject& snapshot) {
            const auto bytes = QJsonDocument(snapshot).toJson(QJsonDocument::Compact);
            const auto digest = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
            source["rawSha256"] = digest;
            source["reviewedSha256"] = digest;
            source["snapshotBase64"] = QString::fromLatin1(bytes.toBase64());
            return source;
        };
        live["source"] = attachSnapshot(live["source"].toObject(),
            {{"provider", "figma"}, {"fileKey", "test-fixture"},
             {"root", QJsonObject{{"id", "synthetic-page"}}}});
        QVERIFY2(validate(live, profile).isEmpty(), qPrintable(validate(live, profile).join('\n')));
        for (const auto* key : {"snapshotBase64", "reviewedSha256", "rawSha256"}) {
            auto broken = live; auto src = broken["source"].toObject();
            src[key] = "changed"; broken["source"] = src;
            QVERIFY(!validate(broken, profile).isEmpty());
        }
        auto figmaSource = live["source"].toObject();
        figmaSource["kind"] = "figmaNormalized";
        figmaSource["documentUrl"] = "https://www.figma.com/design/synthetic-test";
        figmaSource = attachSnapshot(figmaSource,
            {{"provider", "figma"}, {"fileKey", "synthetic-test"},
             {"root", QJsonObject{{"id", "synthetic-page"}}}});
        auto figmaContract = live;
        figmaContract["source"] = figmaSource;
        QVERIFY2(validate(figmaContract, profile).isEmpty(), qPrintable(validate(figmaContract, profile).join('\n')));
        for (const auto* invalidUrl : {"https://www.figma.com.evil.example/design/test",
                                      "https://user@www.figma.com/design/test",
                                      "http://www.figma.com/design/test",
                                      "https://pixso.cn/design/test"}) {
            figmaSource["documentUrl"] = invalidUrl;
            figmaContract["source"] = figmaSource;
            QVERIFY(!validate(figmaContract, profile).isEmpty());
        }
        std::unique_ptr<QWidget> page(preview(live, profile));
        QCOMPARE(page->findChild<Z_InputField*>("lotInput")->property("designNodeId").toString(),
                 QString("synthetic-lotInput"));
        auto mismatched = mutate("submitButton", [](QJsonObject& n) {
            auto p = n["properties"].toObject();
            p["controlSize"] = "lg";
            n["properties"] = p;
        });
        QVERIFY(!validate(mismatched, profile).isEmpty());
        auto matched = mutate("submitButton", [](QJsonObject& n) {
            auto p = n["properties"].toObject();
            p["controlSize"] = "lg";
            n["properties"] = p;
            n["tokenRefs"] = QJsonArray{"component.basic.button.size.lg.height"};
        });
        QVERIFY(validate(matched, profile).isEmpty());
        auto smallest = mutate("submitButton", [](QJsonObject& n) {
            auto p = n["properties"].toObject();
            p["controlSize"] = "xs";
            n["properties"] = p;
            n["tokenRefs"] = QJsonArray{"semantic.dimension.control.height-xs",
                                        "base.stoken.typography.font-size.label.default"};
        });
        QVERIFY(validate(smallest, profile).isEmpty());
        auto quoted = mutate("submitButton", [](QJsonObject& n) {
            auto p = n["properties"].toObject();
            p["text"] = "quotes \", unicode, )QTM0\" & <tag>";
            n["properties"] = p;
        });
        const auto code = generate(quoted, profile)["MappingGenerated.cpp"];
        QVERIFY(code.contains("QByteArray(R\"QTM"));
        QVERIFY(!code.contains("fromBase64"));
        QVERIFY(code.contains("quotes"));
        auto literal = mutate("lotInput", [](QJsonObject& n) {
            auto p = n["properties"].toObject();
            p["label"] = "<b>Literal label</b>";
            n["properties"] = p;
        });
        std::unique_ptr<QWidget> literalPage(preview(literal, profile));
        auto* field = literalPage->findChild<Z_InputField*>("lotInput");
        bool found = false;
        for (auto* label : field->findChildren<QLabel*>()) {
            if (label->text() == "<b>Literal label</b>") {
                found = true;
                QCOMPARE(label->textFormat(), Qt::PlainText);
            }
        }
        QVERIFY(found);
    }
    void staleContractCannotExport() {
        QWidget flow, filePage;
        new QVBoxLayout(&flow);
        new QVBoxLayout(&filePage);
        MappingPage controller(profile, &flow, &filePage, &flow);
        auto* button = flow.findChild<QPushButton*>("mappingExport");
        QVERIFY(button->isEnabled());
        auto* editor = filePage.findChild<QPlainTextEdit*>("mappingContractEditor");
        editor->setPlainText("{}");
        QVERIFY(!button->isEnabled());
        QVERIFY(!controller.applyContract());
        QVERIFY(!button->isEnabled());
        QVERIFY(flow.findChild<Z_InputField*>("lotInput"));
    }
};
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    component_theme::install(app, qEnvironmentVariable("DEMO_PROFILE", "light-comfortable"));
    MappingTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "MappingTests.moc"
