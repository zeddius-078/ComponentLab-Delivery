#include "BaselineGallery.h"

#include <MyDesignSystem/MyDesignSystem.h>

#include <QApplication>
#include <QFrame>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>
#include <QSet>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QStyle>
#include <QStyleOptionButton>
#include <QTabWidget>
#include <QTreeWidget>
#include <QToolButton>
#include <QtTest>

class BaselineConsumerTests final : public QObject {
    Q_OBJECT

  private slots:
    void allPublishedWrappersUseThePublicSdkContract() {
        QWidget owner;
#define VERIFY_NATIVE_WRAPPER(QT_CLASS, SDK_CLASS) \
    { \
        commonWidget::SDK_CLASS component(&owner); \
        QCOMPARE(component.property("nativeQtClass").toString(), QStringLiteral(#QT_CLASS)); \
        QCOMPARE(component.property("designSystemClass").toString(), \
                 QStringLiteral("commonWidget::" #SDK_CLASS)); \
    }
        Z_NATIVE_COMPONENT_TYPES(VERIFY_NATIVE_WRAPPER)
#undef VERIFY_NATIVE_WRAPPER
    }

    void qssAndIndeterminateStateAreOwnedByTheSdk() {
        commonWidget::initializeNativeComponentResources();
        const QString qss = commonWidget::nativeComponentStyleSheet();
        QVERIFY(qss.contains(QStringLiteral("HarmonyOS Sans")));
        QVERIFY(qss.contains(QStringLiteral("indicator:indeterminate")));
        qApp->setStyleSheet(qss);
        commonWidget::Z_CheckBox checkbox;
        checkbox.setTristate(true);
        checkbox.setCheckState(Qt::PartiallyChecked);
        QCOMPARE(checkbox.checkState(), Qt::PartiallyChecked);
        QVERIFY(!qApp->styleSheet().isEmpty());
        checkbox.resize(160, 32);
        checkbox.show();
        QTest::qWait(20);
        QStyleOptionButton option;
        option.initFrom(&checkbox);
        option.state |= QStyle::State_NoChange;
        const QRect indicator = checkbox.style()->subElementRect(
            QStyle::SE_CheckBoxIndicator, &option, &checkbox);
        const QImage rendered = checkbox.grab().toImage();
        const QColor center = rendered.pixelColor(indicator.center());
        const QColor fill = rendered.pixelColor(indicator.left() + 2, indicator.center().y());
        // The middle of the indeterminate indicator is the SDK-owned white
        // horizontal dash, not an invisible white square or the purple fill.
        QVERIFY2(center.red() > 220 && center.green() > 220 && center.blue() > 220,
                 "Indeterminate checkbox must render its white horizontal dash");
        QVERIFY2(fill.red() > 70 && fill.red() < 130 && fill.green() < 90 && fill.blue() > 220,
                 "Indeterminate checkbox must use the SDK purple fill");
    }

    void inheritedQtConstructorsReceiveIdentityBeforePainting() {
        commonWidget::initializeNativeComponentResources();
        qApp->setStyleSheet(commonWidget::nativeComponentStyleSheet());
        commonWidget::Z_CheckBox checkbox(QStringLiteral("Inherited Qt constructor"));
        // The identity is established by the forwarding constructor itself,
        // not deferred to show(), polish(), or an event-loop side effect.
        QCOMPARE(checkbox.property("designSystemClass").toString(),
                 QStringLiteral("commonWidget::Z_CheckBox"));
        checkbox.setTristate(true);
        checkbox.setCheckState(Qt::PartiallyChecked);
        checkbox.resize(220, 32);
        checkbox.show();
        QTest::qWait(20);
        QStyleOptionButton option;
        option.initFrom(&checkbox);
        option.state |= QStyle::State_NoChange;
        const QRect indicator = checkbox.style()->subElementRect(
            QStyle::SE_CheckBoxIndicator, &option, &checkbox);
        const QColor center = checkbox.grab().toImage().pixelColor(indicator.center());
        const QColor fill = checkbox.grab().toImage().pixelColor(
            indicator.left() + 2, indicator.center().y());
        QVERIFY2(center.red() > 220 && center.green() > 220 && center.blue() > 220,
                 "Inherited constructor must receive the QSS indeterminate dash");
        QVERIFY2(fill.red() > 70 && fill.red() < 130 && fill.green() < 90 && fill.blue() > 220,
                 "Inherited constructor must receive the SDK purple fill");
    }

    void tableViewUsesSdkQssAndAnExternalModel() {
        commonWidget::initializeNativeComponentResources();
        const QString qss = commonWidget::nativeComponentStyleSheet();
        QVERIFY(qss.contains(QStringLiteral("QTableView[designSystemClass=\"commonWidget::Z_TableView\"]")));
        QVERIFY(qss.contains(QStringLiteral("#F1ECFF")));
        QVERIFY(qss.contains(QStringLiteral("QHeaderView::section")));
        QVERIFY(qss.contains(QStringLiteral("Table/Header")));
        QVERIFY(qss.contains(QStringLiteral("Table/Body")));
        QVERIFY(qss.contains(QStringLiteral("tableLifecycle")));
        qApp->setStyleSheet(qss + baselineGalleryCanvasStyleSheet());

        commonWidget::Z_TableView table;
        auto* model = new QStandardItemModel(3, 3, &table);
        model->setHorizontalHeaderLabels({QStringLiteral("Name"), QStringLiteral("Status"),
                                           QStringLiteral("Value")});
        table.setModel(model);
        QCOMPARE(table.model(), static_cast<QAbstractItemModel*>(model));
        QCOMPARE(table.selectionBehavior(), QAbstractItemView::SelectRows);
        QVERIFY(!table.verticalHeader()->isVisible());
        table.resize(540, 260);
        table.show();
        QCoreApplication::processEvents();
        const int smallRowHeight = table.rowHeight(0);
        table.setTableSize(QStringLiteral("Medium"));
        QCOMPARE(table.tableSize(), QStringLiteral("Medium"));
        QCoreApplication::processEvents();
        const int mediumRowHeight = table.rowHeight(0);
        QCOMPARE(table.horizontalHeader()->height(), table.tableHeaderHeightMedium());
        table.selectRow(1);
        QVERIFY(table.selectionModel()->isRowSelected(1, QModelIndex()));
        table.setTableSize(QStringLiteral("Large"));
        QCoreApplication::processEvents();
        const int largeRowHeight = table.rowHeight(0);
        QCOMPARE(table.horizontalHeader()->height(), table.tableHeaderHeightLarge());
        QVERIFY2(smallRowHeight < mediumRowHeight && mediumRowHeight < largeRowHeight,
                 "The SDK QSS must make Small / Medium / Large row geometry distinct");
        QCOMPARE(table.model(), static_cast<QAbstractItemModel*>(model));
        QCOMPARE(table.tableSize(), QStringLiteral("Large"));
        table.setTableStriped(true);
        QVERIFY(table.tableStriped());
        table.setLifecycleState(QStringLiteral("loading"));
        QCOMPARE(table.lifecycleState(), QStringLiteral("loading"));
        auto* loading = table.findChild<QLabel*>(QStringLiteral("loading"));
        QVERIFY(loading);
        table.setLifecycleState(QStringLiteral("ready"));
        QVERIFY(loading->isHidden());

        BaselineWindow window;
        auto* tabs = window.findChild<QTabWidget*>(QStringLiteral("baselineTableViewSizeTabs"));
        QVERIFY(tabs);
        QCOMPARE(tabs->count(), 3);
        QCOMPARE(window.findChildren<commonWidget::Z_TableView*>(
                     QRegularExpression(QStringLiteral("^baselineTableView_"))).size(), 15);
        auto* disabled = window.findChild<commonWidget::Z_TableView*>(
            QStringLiteral("baselineTableView_Small_Disabled"));
        QVERIFY(disabled && !disabled->isEnabled());
        auto* selected = window.findChild<commonWidget::Z_TableView*>(
            QStringLiteral("baselineTableView_Small_Selected"));
        QVERIFY(selected && selected->selectionModel()->isRowSelected(0, QModelIndex()));
        auto* striped = window.findChild<commonWidget::Z_CheckBox*>(
            QStringLiteral("baselineTableViewStriped"));
        auto* lifecycle = window.findChild<commonWidget::Z_ComboBox*>(
            QStringLiteral("baselineTableViewLifecycle"));
        QVERIFY(striped && lifecycle);
        auto* appearance = window.findChild<commonWidget::Z_TableView*>(
            QStringLiteral("componentRoot_QTableView"));
        QVERIFY(appearance);
        auto* headerSpecimen = window.findChild<commonWidget::Z_TableView*>(
            QStringLiteral("tableHeaderSpecimen"));
        auto* bodySpecimen = window.findChild<commonWidget::Z_TableView*>(
            QStringLiteral("tableBodySpecimen"));
        QVERIFY(headerSpecimen && bodySpecimen);
        QCOMPARE(headerSpecimen->model()->rowCount(), 0);
        QVERIFY(!headerSpecimen->horizontalHeader()->isHidden());
        QCOMPARE(headerSpecimen->horizontalHeader()->height(),
                 headerSpecimen->tableHeaderHeightMedium());
        QCOMPARE(bodySpecimen->model()->rowCount(), 3);
        QVERIFY(bodySpecimen->horizontalHeader()->isHidden());
        auto* headerAction = appearance->findChild<commonWidget::Z_ToolButton*>(
            QStringLiteral("tableHeaderAction"));
        auto* cellAction = appearance->findChild<commonWidget::Z_PushButton*>(
            QStringLiteral("tableCellAction"));
        auto* cellIconAction = appearance->findChild<commonWidget::Z_ToolButton*>(
            QStringLiteral("tableCellIconAction"));
        QVERIFY(headerAction && cellAction && cellIconAction);
        window.show();
        auto* pages = window.findChild<QStackedWidget*>(QStringLiteral("baselineGalleryPages"));
        QVERIFY(pages);
        QWidget* page = appearance->parentWidget();
        while (page && page->objectName() != QStringLiteral("baselineContentHost"))
            page = page->parentWidget();
        QVERIFY(page);
        pages->setCurrentWidget(page);
        QTest::qWait(20);
        auto* header = appearance->horizontalHeader();
        QCOMPARE(header->sortIndicatorSection(), 0);
        QTest::mouseClick(header->viewport(), Qt::LeftButton, Qt::NoModifier,
                          QPoint(header->sectionViewportPosition(3) + 12,
                                 header->viewport()->height() / 2));
        QCOMPARE(header->sortIndicatorSection(), 0);
        headerAction->click();
        QCOMPARE(appearance->property("lastTableAction").toString(), QStringLiteral("header"));
        cellAction->click();
        QCOMPARE(appearance->property("lastTableAction").toString(), QStringLiteral("view"));
        cellIconAction->click();
        QCOMPARE(appearance->property("lastTableAction").toString(), QStringLiteral("details"));
        striped->setChecked(true);
        QVERIFY(appearance->tableStriped());
        lifecycle->setCurrentIndex(1);
        QCOMPARE(appearance->lifecycleState(), QStringLiteral("loading"));
    }

    void selectUsesSdkOptionAndTagBehavior() {
        commonWidget::initializeNativeComponentResources();
        const QString qss = commonWidget::nativeComponentStyleSheet();
        QVERIFY(qss.contains(QStringLiteral("select-chevron-down.svg")));
        QVERIFY(qss.contains(QStringLiteral("selectTag")));
        qApp->setStyleSheet(qss);
        commonWidget::Z_ComboBox select;
        select.addItem(QStringLiteral("Automatic"), QStringLiteral("auto"));
        select.addItem(QStringLiteral("Manual"), QStringLiteral("manual"));
        select.addItem(QStringLiteral("Maintenance"), QStringLiteral("maintenance"));
        select.setCurrentIndex(-1);
        select.setSelectionMode(commonWidget::Z_ComboBox::SelectionMode::Tags);
        select.setSelectedValues({QStringLiteral("auto"), QStringLiteral("manual")});
        QCOMPARE(select.selectedValues(),
                 (QStringList{QStringLiteral("auto"), QStringLiteral("manual")}));
        QCOMPARE(select.findChildren<QFrame*>(QStringLiteral("selectTag")).size(), 2);
        select.setMaxSelectionCount(2);
        QVERIFY(!select.toggleOption(2));
        QCOMPARE(select.selectedValues().size(), 2);
        QVERIFY(select.toggleOption(0));
        QCOMPARE(select.selectedValues(), QStringList{QStringLiteral("manual")});
        select.setSelectionMode(commonWidget::Z_ComboBox::SelectionMode::Single);
        QCOMPARE(select.currentData().toString(), QStringLiteral("manual"));
    }

    void inputCompositeUsesSdkQssAndRealActions() {
        commonWidget::initializeNativeComponentResources();
        qApp->setStyleSheet(commonWidget::nativeComponentStyleSheet() +
                            baselineGalleryCanvasStyleSheet());
        BaselineWindow window;
        window.show();
        auto* input = window.findChild<commonWidget::Z_InputField*>(
            QStringLiteral("componentRoot_Input_0"));
        QVERIFY(input);
        QCOMPARE(input->property("designSystemClass").toString(),
                 QStringLiteral("commonWidget::Z_InputField"));
        QCOMPARE(input->contentInset(), 3);
        QCOMPARE(input->contentGap(), 4);
        QCOMPARE(input->value(), QStringLiteral("ComponentLab"));
        auto* pages = window.findChild<QStackedWidget*>(QStringLiteral("baselineGalleryPages"));
        QVERIFY(pages);
        auto* page = input->parentWidget();
        while (page && page->objectName() != QStringLiteral("baselineContentHost"))
            page = page->parentWidget();
        QVERIFY(page);
        pages->setCurrentWidget(page);
        QTest::qWait(20);
        auto* prefix = input->findChild<QLabel*>(QStringLiteral("inputPrefix"));
        QVERIFY(prefix);
        QVERIFY(!prefix->isVisible());
        const int noIconEditorX = input->lineEdit()->x();
        input->setPrefixIcon(QIcon(QStringLiteral(":/z-native/icons/input-search.svg")));
        QCoreApplication::processEvents();
        QVERIFY(prefix->isVisible());
        QVERIFY(input->lineEdit()->x() > noIconEditorX);
        input->setPrefixIcon(QIcon{});
        QCoreApplication::processEvents();
        QVERIFY(!prefix->isVisible());
        QCOMPARE(input->lineEdit()->x(), noIconEditorX);
        QVERIFY(input->clearAction()->isVisible());
        QTest::mouseClick(input->clearAction(), Qt::LeftButton);
        QCOMPARE(input->value(), QString{});
        QVERIFY(!input->clearAction()->isVisible());
        QCOMPARE(page->findChildren<commonWidget::Z_InputField*>(
                     QRegularExpression(QStringLiteral("^inputMatrix_"))).size(), 18);
        auto* password = window.findChild<commonWidget::Z_InputField*>(
            QStringLiteral("componentRoot_Input_1"));
        QVERIFY(password);
        QCOMPARE(password->lineEdit()->echoMode(), QLineEdit::Password);
        QTest::mouseClick(password->passwordAction(), Qt::LeftButton);
        QCOMPARE(password->lineEdit()->echoMode(), QLineEdit::Normal);
        auto* loading = window.findChild<commonWidget::Z_InputField*>(
            QStringLiteral("componentRoot_Input_3"));
        QVERIFY(loading && loading->isLoading());
        QVERIFY(loading->spinner()->isVisible());
        auto* tabs = page->findChild<QTabWidget*>(QStringLiteral("baselineComponentTabs"));
        QVERIFY(tabs);
        tabs->setCurrentIndex(1);
        window.resize(1040, 700);
        QTest::qWait(30);
        auto* stateScroll = qobject_cast<QScrollArea*>(tabs->currentWidget());
        QVERIFY(stateScroll);
        QCOMPARE(stateScroll->horizontalScrollBar()->maximum(), 0);
    }

    void inputKeepsOneValueAcrossNativeEditors() {
        qApp->setStyleSheet(commonWidget::componentStyleSheet());
        commonWidget::Z_InputField input;
        input.setKind(commonWidget::Z_InputField::Kind::Text);
        input.show();
        input.lineEdit()->setFocus();
        QTest::keyClicks(input.lineEdit(), QStringLiteral("Batch-42"));
        QCOMPARE(input.value(), QStringLiteral("Batch-42"));
        input.setKind(commonWidget::Z_InputField::Kind::TextArea);
        QCOMPARE(input.textArea()->toPlainText(), QStringLiteral("Batch-42"));
        input.textArea()->setFocus();
        QTest::keyClicks(input.textArea(), QStringLiteral(" new"));
        QVERIFY(input.value().contains(QStringLiteral("new")));
        input.setKind(commonWidget::Z_InputField::Kind::Text);
        QCOMPARE(input.lineEdit()->text(), input.value());
    }

    void selectAppearanceMatrixDoesNotBleedIntoAdjacentColumns() {
        commonWidget::initializeNativeComponentResources();
        qApp->setStyleSheet(commonWidget::nativeComponentStyleSheet() +
                            baselineGalleryCanvasStyleSheet());
        BaselineWindow window;
        window.resize(1360, 880);
        window.show();
        auto* select = window.findChild<commonWidget::Z_ComboBox*>(
            QStringLiteral("componentRoot_QComboBox"));
        QVERIFY(select);
        auto* page = select->parentWidget();
        while (page && page->objectName() != QStringLiteral("baselineContentHost"))
            page = page->parentWidget();
        QVERIFY(page);
        auto* pages = window.findChild<QStackedWidget*>(QStringLiteral("baselineGalleryPages"));
        QVERIFY(pages);
        pages->setCurrentWidget(page);
        auto* tabs = page->findChild<QTabWidget*>(QStringLiteral("baselineComponentTabs"));
        QVERIFY(tabs);
        tabs->setCurrentIndex(1);
        QTest::qWait(30);
        auto* stateScroll = qobject_cast<QScrollArea*>(tabs->currentWidget());
        QVERIFY(stateScroll);
        QCOMPARE(stateScroll->horizontalScrollBar()->maximum(), 0);
        window.resize(1040, 700);
        QTest::qWait(30);
        QCOMPARE(stateScroll->horizontalScrollBar()->maximum(), 0);
        auto* inspector = window.findChild<QTreeWidget*>(
            QStringLiteral("baselineObjectInspector"));
        QVERIFY(inspector);
        QVERIFY(!inspector->isVisible());
        QCOMPARE(stateScroll->findChildren<commonWidget::Z_ComboBox*>().size(), 37);
    }

    void galleryDisplaysOnePublishedComponentPerPage() {
        commonWidget::initializeNativeComponentResources();
        qApp->setStyleSheet(commonWidget::nativeComponentStyleSheet() +
                            baselineGalleryCanvasStyleSheet());
        BaselineWindow window;
        QCOMPARE(window.componentSectionCount(), 9);
        QCOMPARE(window.componentPageCount(), 59);
        QVERIFY(window.hasObjectInspector());
        QVERIFY(!baselineGalleryCanvasStyleSheet().isEmpty());
        QVERIFY(window.findChild<QTabWidget*>(QStringLiteral("baselineSdkTabs")));
        QVERIFY(window.findChild<QTabWidget*>(QStringLiteral("baselineGalleryTabs")));
        auto* navigation = window.findChild<QTreeWidget*>(QStringLiteral("baselineNavigation"));
        QVERIFY(navigation);
        QCOMPARE(navigation->topLevelItemCount(), 1);
        auto* published = navigation->topLevelItem(0);
        QCOMPARE(published->childCount(), 9);
        auto* pages = window.findChild<QStackedWidget*>(QStringLiteral("baselineGalleryPages"));
        QVERIFY(pages);
        QCOMPARE(pages->count(), 59);
        int leafCount = 0;
        int inputEntryCount = 0;
        QSet<int> pageIndices;
        QTreeWidgetItem* checkboxLeaf = nullptr;
        for (int categoryIndex = 0; categoryIndex < published->childCount(); ++categoryIndex) {
            auto* category = published->child(categoryIndex);
            for (int componentIndex = 0; componentIndex < category->childCount(); ++componentIndex) {
                auto* leaf = category->child(componentIndex);
                const int pageIndex = leaf->data(0, Qt::UserRole).toInt();
                QVERIFY(pageIndex >= 0 && pageIndex < pages->count());
                QVERIFY(!pageIndices.contains(pageIndex));
                pageIndices.insert(pageIndex);
                const QString qtClass = leaf->data(0, Qt::UserRole + 1).toString();
                QVERIFY(qtClass != QStringLiteral("QLineEdit"));
                if (qtClass == QStringLiteral("ComponentLab/Input")) {
                    QCOMPARE(leaf->text(0), QStringLiteral("Input / 输入框"));
                    ++inputEntryCount;
                } else {
                    QVERIFY(leaf->text(0).startsWith(QStringLiteral("Z_")));
                }
                if (qtClass == QStringLiteral("QCheckBox"))
                    checkboxLeaf = leaf;
                const auto roots = pages->widget(pageIndex)->findChildren<QWidget*>();
                int matching = 0;
                for (auto* root : roots) {
                    if (!root->objectName().startsWith(QStringLiteral("componentRoot_")))
                        continue;
                    if (qtClass == QStringLiteral("ComponentLab/Input")) {
                        QVERIFY(root->objectName().startsWith(QStringLiteral("componentRoot_Input_")));
                        QVERIFY(qobject_cast<commonWidget::Z_InputField*>(root));
                    } else {
                        QCOMPARE(root->objectName(), QStringLiteral("componentRoot_%1").arg(qtClass));
                    }
                    ++matching;
                }
                if (qtClass != QStringLiteral("QOpenGLWidget"))
                    QVERIFY2(matching > 0, "Page must contain its own SDK component instance");
                ++leafCount;
            }
        }
        QCOMPARE(leafCount, 59);
        QCOMPARE(inputEntryCount, 1);
        QVERIFY(checkboxLeaf);
        navigation->setCurrentItem(checkboxLeaf);
        QCOMPARE(pages->currentIndex(), checkboxLeaf->data(0, Qt::UserRole).toInt());
        auto* selectedTabs = pages->currentWidget()->findChild<QTabWidget*>(
            QStringLiteral("baselineComponentTabs"));
        QVERIFY(selectedTabs);
        QCOMPARE(selectedTabs->count(), 2);
        selectedTabs->setCurrentIndex(1);
        QVERIFY(pages->currentWidget()->findChild<commonWidget::Z_CheckBox*>(
            QStringLiteral("baselineIndeterminateCheckbox")));
        auto* inspector = window.findChild<QTreeWidget*>(QStringLiteral("baselineObjectInspector"));
        QVERIFY(inspector);
        QCOMPARE(inspector->columnCount(), 2);
        QCOMPARE(inspector->topLevelItem(0)->text(1),
                 QStringLiteral("commonWidget::Z_CheckBox"));
        QVERIFY(window.findChild<commonWidget::Z_PushButton*>(
            QStringLiteral("componentRoot_QPushButton")));
        QVERIFY(window.findChild<commonWidget::Z_CheckBox*>(
            QStringLiteral("baselineIndeterminateCheckbox")));
    }

    void galleryExamplesPreserveNativeInteractionContracts() {
        commonWidget::initializeNativeComponentResources();
        qApp->setStyleSheet(commonWidget::nativeComponentStyleSheet() +
                            baselineGalleryCanvasStyleSheet());
        BaselineWindow window;
        auto* menu = window.findChild<commonWidget::Z_Menu*>(
            QStringLiteral("componentRoot_QMenu"));
        QVERIFY(menu);
        QCOMPARE(menu->actions().count(), 4);
        QVERIFY(menu->windowFlags() & Qt::Popup);

        auto* wizard = window.findChild<commonWidget::Z_Wizard*>(
            QStringLiteral("componentRoot_QWizard"));
        QVERIFY(wizard);
        QCOMPARE(wizard->pageIds().count(), 2);

        auto* dock = window.findChild<commonWidget::Z_DockWidget*>(
            QStringLiteral("componentRoot_QDockWidget"));
        QVERIFY(dock);
        QVERIFY(dock->widget());
        QVERIFY(!dock->windowTitle().isEmpty());

        auto* scrollArea = window.findChild<commonWidget::Z_ScrollArea*>(
            QStringLiteral("componentRoot_QScrollArea"));
        QVERIFY(scrollArea);
        QVERIFY(scrollArea->widget());
        QVERIFY(scrollArea->widget()->minimumHeight() > scrollArea->height());
        auto* navigation = window.findChild<QTreeWidget*>(QStringLiteral("baselineNavigation"));
        QVERIFY(navigation);
        auto* published = navigation->topLevelItem(0);
        QTreeWidgetItem* scrollLeaf = nullptr;
        for (int section = 0; section < published->childCount(); ++section) {
            auto* category = published->child(section);
            for (int item = 0; item < category->childCount(); ++item) {
                auto* leaf = category->child(item);
                if (leaf->data(0, Qt::UserRole + 1).toString() == QStringLiteral("QScrollArea"))
                    scrollLeaf = leaf;
            }
        }
        QVERIFY(scrollLeaf);
        navigation->setCurrentItem(scrollLeaf);
        window.show();
        QTest::qWait(20);
        auto* verticalBar = scrollArea->verticalScrollBar();
        QVERIFY(verticalBar->maximum() > 0);
        verticalBar->setValue(verticalBar->maximum());
        QCOMPARE(verticalBar->value(), verticalBar->maximum());

        auto* undo = window.findChild<commonWidget::Z_UndoView*>(
            QStringLiteral("componentRoot_QUndoView"));
        QVERIFY(undo);
        QVERIFY(undo->stack());
        QCOMPARE(undo->stack()->count(), 2);

        auto* font = window.findChild<commonWidget::Z_FontComboBox*>(
            QStringLiteral("componentRoot_QFontComboBox"));
        QVERIFY(font);
        QCOMPARE(font->findText(QStringLiteral("自动")), -1);
        QVERIFY(font->count() > 0);

        auto* plainText = window.findChild<commonWidget::Z_PlainTextEdit*>(
            QStringLiteral("componentRoot_QPlainTextEdit"));
        QVERIFY(plainText);
        QVERIFY(!plainText->toPlainText().isEmpty());
        auto* lcd = window.findChild<commonWidget::Z_LCDNumber*>(
            QStringLiteral("componentRoot_QLCDNumber"));
        QVERIFY(lcd);
        QCOMPARE(lcd->value(), 42.0);
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    BaselineConsumerTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "BaselineConsumerTests.moc"
