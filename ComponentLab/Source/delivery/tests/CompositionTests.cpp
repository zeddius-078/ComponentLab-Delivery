#include "DemoWindow.h"
#include "DemoRuntime.h"
#include "ButtonAppearanceAdapter.h"
#include "Z_InputField.h"
#include "Z_Select.h"
#include "Z_CheckBox.h"
#include "Z_DesignComponents.h"
#include "Z_FoundationComponents.h"
#include "native/Z_ScrollArea.h"
#include "native/Z_Splitter.h"
#include "SolidInnerShadow.h"
#include "FocusIndicator.h"
#include "Z_OtpInput.h"
#include "NavigationGallery.h"
#include "FigmaLayoutEvidence.h"
#include "DesignComponentCatalog.h"
#include "ComponentFactory.h"
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QToolButton>
#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QCommandLinkButton>
#include <QFontComboBox>
#include <QAbstractItemView>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QColorDialog>
#include <QFontDialog>
#include <QDial>
#include <QLabel>
#include <QFontInfo>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QRawFont>
#include <QLineEdit>
#include <QKeySequenceEdit>
#include <QKeyEvent>
#include <QListView>
#include <QListWidget>
#include <QPainter>
#include <QRadioButton>
#include <QRegularExpression>
#include <QButtonGroup>
#include <QSet>
#include <QPushButton>
#include <QProgressBar>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateTimeEdit>
#include <QSlider>
#include <QSignalSpy>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QWizard>
#include <QWizardPage>
#include <QColumnView>
#include <QUndoView>
#include <QStyleFactory>
#include <QStyledItemDelegate>
#include <QTabWidget>
#include <QTabBar>
#include <QToolBox>
#include <QDockWidget>
#include <QMainWindow>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTableWidget>
#include <QTextBrowser>
#include <QLCDNumber>
#include <QCalendarWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QOpenGLWidget>
#include <QTreeWidget>
#include <QTest>
#include <QVBoxLayout>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

using namespace commonWidget;
class CompositionTests : public QObject {
    Q_OBJECT
    QString profile, evidence;
    std::shared_ptr<const field::Tokens> tokens;
    std::unique_ptr<DemoWindow> window;
    template <class T> T* part(const char* name) {
        return window->findChild<T*>(name);
    }
    QTreeWidgetItem* pageItem(QTreeWidget* tree, const QString& pageId) {
        for (QTreeWidgetItemIterator it(tree); *it; ++it)
            if ((*it)->data(0, Qt::UserRole).toString() == pageId)
                return *it;
        return nullptr;
    }
    void edit(const char* name, const QString& value) {
        auto* editor = part<QLineEdit>(name);
        editor->setText(value);
        QMetaObject::invokeMethod(editor, "editingFinished");
    }
    QWidget* variantCanvas(const QString& pageId) {
        auto* tree = part<QTreeWidget>("componentTree");
        for (QTreeWidgetItemIterator it(tree); *it; ++it) {
            if ((*it)->data(0, Qt::UserRole).toString() == pageId) {
                tree->setCurrentItem(*it);
                break;
            }
        }
        QCoreApplication::processEvents();
        auto* page = part<QStackedWidget>("componentPages")->currentWidget();
        auto* sections = page->findChild<QTabWidget*>("componentSections");
        if (!sections || sections->count() < 2)
            return nullptr;
        sections->setCurrentIndex(1);
        QCoreApplication::processEvents();
        auto* view = page->findChild<QGraphicsView*>("figmaVariantGallery");
        if (!view)
            return nullptr;
        for (auto* item : view->scene()->items())
            if (auto* proxy = qgraphicsitem_cast<QGraphicsProxyWidget*>(item))
                return proxy->widget();
        return nullptr;
    }
    QSet<QString> expectedVariantNames(const QString& pageId) const {
        QSet<QString> result;
        for (const auto& page : commonWidget::figmaComponentPages()) {
            if (page.id != pageId)
                continue;
            for (const auto& set : page.componentSets)
                for (const auto& variant : set.variants)
                    result.insert(variant);
        }
        return result;
    }
    QSet<QString> actualVariantNames(QWidget* canvas) const {
        QSet<QString> result;
        for (auto* widget : canvas->findChildren<QWidget*>()) {
            if (widget->property("lifecycleBoundary").toBool())
                continue;
            const QString variant = widget->property("figmaVariant").toString();
            if (!variant.isEmpty())
                result.insert(variant);
        }
        return result;
    }
  private slots:
    void mdiGalleryKeepsAuthoredCompositionAndNativeEditor() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)=="511:11")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();auto* sections=page->findChild<QTabWidget*>("componentSections");sections->setCurrentIndex(1);QTest::qWait(80);auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);QCOMPARE(view->property("variantSpecimenCount").toInt(),10);
        QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(1312,1520));
        const auto areas=canvas->findChildren<QMdiArea*>();QCOMPARE(areas.size(),4);QCOMPARE(canvas->findChildren<QMdiSubWindow*>().size(),18);
        for(auto* l:canvas->findChildren<QLabel*>())if(l->property("figmaNodeId")=="511:282"){QCOMPARE(l->font().pixelSize(),14);QCOMPARE(QRawFont::fromFont(l->font()).weight(),int(QFont::DemiBold));}
for(auto* area:areas){QCOMPARE(area->geometry(),area->property("figmaDesignBounds").toRect());if(area->viewMode()==QMdiArea::TabbedView){auto* bar=area->findChild<QTabBar*>(QString(),Qt::FindDirectChildrenOnly);QVERIFY(bar);QCOMPARE(area->viewport()->geometry(),QRect(12,92,580,250));QCOMPARE(bar->geometry(),QRect(12,48,580,36));QCOMPARE(bar->count(),3);QCOMPARE(area->subWindowList().first()->geometry(),QRect(0,0,580,250));}}
        sections->setCurrentIndex(0);auto* editors=page->findChild<QStackedWidget*>("appearanceComponentSetEditors");auto* firstEditor=editors->widget(0);auto* sub=firstEditor->findChild<QMdiSubWindow*>("componentInteractivePreview");QVERIFY(sub);auto* workspace=firstEditor->findChild<QMdiArea*>("mdiEditorWorkspace");QVERIFY(workspace);auto* payload=sub->widget();
        auto* variant=firstEditor->findChild<QComboBox*>("componentVariantControl");variant->setCurrentText("State=Maximized");QCoreApplication::processEvents();QVERIFY(!sub->isMaximized());QCOMPARE(sub->property("figmaMdiSnapshotState").toString(),QString("Maximized"));
        firstEditor->findChild<QPushButton*>("mdiMaximizeControl")->click();QVERIFY(sub->isMaximized());firstEditor->findChild<QPushButton*>("mdiRestoreControl")->click();QVERIFY(!sub->isMaximized());
        firstEditor->findChild<QCheckBox*>("mdiVetoCloseControl")->setChecked(true);firstEditor->findChild<QPushButton*>("mdiCloseControl")->click();QVERIFY(sub->isVisible());firstEditor->findChild<QCheckBox*>("mdiVetoCloseControl")->setChecked(false);firstEditor->findChild<QPushButton*>("mdiCloseControl")->click();QVERIFY(sub->isHidden());QCOMPARE(sub->widget(),payload);firstEditor->findChild<QPushButton*>("mdiShowAllControl")->click();
        page->findChild<QComboBox*>("appearanceComponentSetControl")->setCurrentIndex(1);QTest::qWait(60);
auto* secondEditor=editors->widget(1);auto* area=secondEditor->findChild<QMdiArea*>("componentInteractivePreview");QVERIFY(area);QCOMPARE(area->subWindowList().size(),2);QCOMPARE(area->property("figmaMdiPresentationPreview").toString(),QString("Free"));const auto geometry=area->subWindowList().first()->geometry();secondEditor->findChild<QComboBox*>("componentVariantControl")->setCurrentText("Presentation=Tile");QCoreApplication::processEvents();QCOMPARE(area->viewMode(),QMdiArea::SubWindowView);QCOMPARE(area->subWindowList().first()->geometry(),geometry);
        secondEditor->findChild<QPushButton*>("mdiApplyPresentationControl")->click();QCOMPARE(area->viewMode(),QMdiArea::SubWindowView);QVERIFY(!area->subWindowList()[0]->geometry().intersects(area->subWindowList()[1]->geometry()));
        secondEditor->findChild<QComboBox*>("mdiViewModeControl")->setCurrentIndex(1);QCOMPARE(area->viewMode(),QMdiArea::TabbedView);QCOMPARE(area->subWindowList().size(),2);
        QTest::qWait(80);QCOMPARE(firstEditor->findChild<QScrollArea*>("componentSettingsScroll")->horizontalScrollBar()->maximum(),0);
    }
    void initTestCase() {
        profile = qEnvironmentVariable("DEMO_PROFILE", "light-comfortable");
        evidence = qEnvironmentVariable("DEMO_EVIDENCE");
        if (!evidence.isEmpty())
            QVERIFY(QDir().mkpath(evidence));
        tokens = std::make_shared<field::Tokens>(profile);
        window = std::make_unique<DemoWindow>(profile);
        window->show();
        QTest::qWait(60);
    }
    void navigationContract() {
        auto* tabs = part<QTabWidget>("tabs");
        QVERIFY(tabs);
        QCOMPARE(tabs->count(), 1);
        QCOMPARE(tabs->tabText(0), QString("Z-DesignSystem"));

        auto* nativeSections = tabs->widget(0)->findChild<QTabWidget*>("nativeSections");
        QVERIFY(nativeSections);
        QCOMPARE(nativeSections->count(), 2);
        QCOMPARE(nativeSections->tabText(0), QString::fromUtf8("设计系统图库"));
        QCOMPARE(nativeSections->tabText(1), QString::fromUtf8("组件映射"));

        auto* gallery = part<QWidget>("nativeGalleryPage");
        auto* tree = part<QTreeWidget>("componentTree");
        auto* pages = part<QStackedWidget>("componentPages");
        auto* registry = part<QTableWidget>("nativeRegistryTable");
        QVERIFY(gallery && tree && pages && registry);
        const auto descriptors = commonWidget::designComponentDescriptors();
        const auto libraryEntries = commonWidget::designLibraryEntries();
        const auto figmaPages = commonWidget::figmaComponentPages();
        const auto figmaCategories = commonWidget::figmaComponentCategories();
        int expectedInstantiable = 0;
        int expectedSetCount = 0;
        int expectedVariantCount = 0;
        int expectedComponentCount = 0;
        int expectedLayoutCount = 0;
        int expectedAssetCount = 0;
        QMap<QString, QString> expectedPageSection;
        for (const auto& entry : libraryEntries) {
            const QString section = entry.kind == QStringLiteral("LayoutHelper")
                ? QStringLiteral("layout")
                : entry.kind == QStringLiteral("Asset")
                ? QStringLiteral("assets")
                : QStringLiteral("components");
            expectedPageSection.insert(entry.pageId, section);
            if (section == QStringLiteral("components")) ++expectedComponentCount;
            if (section == QStringLiteral("layout")) ++expectedLayoutCount;
            if (section == QStringLiteral("assets")) ++expectedAssetCount;
        }
        for (const auto& descriptor : descriptors)
            expectedInstantiable += descriptor.instantiable ? 1 : 0;
        for (const auto& figmaPage : figmaPages) {
            expectedSetCount += figmaPage.componentSets.size();
            for (const auto& componentSet : figmaPage.componentSets)
                expectedVariantCount += componentSet.variants.size();
        }
        QCOMPARE(gallery->property("descriptorCount").toInt(), descriptors.size());
        QCOMPARE(gallery->property("libraryEntryCount").toInt(), libraryEntries.size());
        QCOMPARE(gallery->property("componentCount").toInt(), expectedComponentCount);
        QCOMPARE(gallery->property("pageCount").toInt(), figmaPages.size());
        QCOMPARE(gallery->property("layoutHelperCount").toInt(), expectedLayoutCount);
        QCOMPARE(gallery->property("assetCount").toInt(), expectedAssetCount);
        QCOMPARE(gallery->property("sectionCount").toInt(), 3);
        QCOMPARE(gallery->property("componentSetCount").toInt(), expectedSetCount);
        QCOMPARE(gallery->property("variantCount").toInt(), expectedVariantCount);
        QCOMPARE(gallery->property("instantiableCount").toInt(), expectedInstantiable);
        QCOMPARE(gallery->property("contractOnlyCount").toInt(),
                 descriptors.size() - expectedInstantiable);
        QCOMPARE(gallery->property("figmaFileKey").toString(),
                 QString("w92BKW1EcutKmTXkwzNHbf"));
        QCOMPARE(tree->topLevelItemCount(), 3);
        QCOMPARE(tree->topLevelItem(0)->data(0, Qt::UserRole + 3).toString(),
                 QStringLiteral("components"));
        QCOMPARE(tree->topLevelItem(1)->data(0, Qt::UserRole + 3).toString(),
                 QStringLiteral("layout"));
        QCOMPARE(tree->topLevelItem(2)->data(0, Qt::UserRole + 3).toString(),
                 QStringLiteral("assets"));
        QVERIFY(tree->topLevelItem(0)->text(0).startsWith(QString::fromUtf8("组件 (")));
        QVERIFY(tree->topLevelItem(1)->text(0).startsWith(QString::fromUtf8("布局辅助 (")));
        QVERIFY(tree->topLevelItem(2)->text(0).startsWith(QString::fromUtf8("资产 (")));
        QCOMPARE(registry->rowCount(), descriptors.size());

        int pageCount = 0;
        QSet<QString> pageIds;
        QSet<QString> validCategoryIds;
        for (const auto& category : figmaCategories)
            validCategoryIds.insert(category.id);
        for (int section = 0; section < tree->topLevelItemCount(); ++section) {
            auto* sectionItem = tree->topLevelItem(section);
            QVERIFY(sectionItem->childCount() > 0);
            const QString sectionId = sectionItem->data(0, Qt::UserRole + 3).toString();
            for (int group = 0; group < sectionItem->childCount(); ++group) {
                auto* branch = sectionItem->child(group);
                QVERIFY(validCategoryIds.contains(branch->data(0, Qt::UserRole).toString()));
                QVERIFY(branch->childCount() > 0);
                for (int row = 0; row < branch->childCount(); ++row) {
                    auto* leaf = branch->child(row);
                    const QString pageId = leaf->data(0, Qt::UserRole).toString();
                    QVERIFY(!pageId.isEmpty());
                    QVERIFY(!pageIds.contains(pageId));
                    pageIds.insert(pageId);
                    QCOMPARE(leaf->data(0, Qt::UserRole + 3).toString(), sectionId);
                    QCOMPARE(expectedPageSection.value(pageId), sectionId);
                    QVERIFY(leaf->text(0).startsWith("20."));
                    ++pageCount;
                }
            }
        }
        QCOMPARE(pageCount, figmaPages.size());
        QCOMPARE(pageIds.size(), figmaPages.size());

        QTreeWidgetItem* mdiPage = pageItem(tree, QStringLiteral("511:11"));
        QVERIFY(mdiPage);
        tree->setCurrentItem(mdiPage);
        QCoreApplication::processEvents();
        QCOMPARE(gallery->property("activePageId").toString(), QString("511:11"));
        auto* mdiChoices = pages->currentWidget()->findChild<QComboBox*>("appearanceComponentSetControl");
        QVERIFY(mdiChoices);
        QCOMPARE(mdiChoices->count(), 2);
        QCOMPARE(mdiChoices->itemText(0), QString("ComponentLab/MdiSubWindow"));
        QCOMPARE(mdiChoices->itemText(1), QString("ComponentLab/MdiArea"));
        QVERIFY(pages->currentWidget()->findChild<QGraphicsView*>("figmaVariantGallery"));
        QVERIFY(!window->findChild<QWidget*>("mappingTab"));
        QVERIFY(!window->findChild<QWidget*>("mappingImplementationGuide"));

        QTreeWidgetItem* iconAssetPage = pageItem(tree, QStringLiteral("2127:3"));
        QVERIFY(iconAssetPage);
        QCOMPARE(iconAssetPage->data(0, Qt::UserRole + 3).toString(), QStringLiteral("assets"));
        tree->setCurrentItem(iconAssetPage);
        QCoreApplication::processEvents();
        auto* iconGallery = pages->currentWidget()->findChild<QWidget*>("iconAssetGallery");
        auto* iconTable = pages->currentWidget()->findChild<QTableWidget*>("iconAssetCatalogTable");
        QVERIFY(iconGallery && iconTable);
        QVERIFY(iconGallery->property("iconAssetUsesSdkCatalog").toBool());
        QCOMPARE(iconGallery->property("iconAssetVariantCount").toInt(), 6);
        QCOMPARE(iconTable->rowCount(), commonWidget::IconAssetCatalog::descriptors().size());
        QCOMPARE(iconTable->item(0, 1)->text(), QStringLiteral("icon.action.previous"));
        QVERIFY(!commonWidget::ComponentFactory::isRegistered(
            QStringLiteral("commonWidget::IconAsset")));

        QTreeWidgetItem* imageAssetPage = pageItem(tree, QStringLiteral("2127:4"));
        QVERIFY(imageAssetPage);
        QCOMPARE(imageAssetPage->data(0, Qt::UserRole + 3).toString(), QStringLiteral("assets"));
        tree->setCurrentItem(imageAssetPage);
        QCoreApplication::processEvents();
        auto* imageGallery = pages->currentWidget()->findChild<QWidget*>("imageAssetGallery");
        auto* imageTable = pages->currentWidget()->findChild<QTableWidget*>(
            "visualAssetCatalogTable");
        QVERIFY(imageGallery && imageTable);
        QVERIFY(imageGallery->property("imageAssetUsesSdkCatalog").toBool());
        QCOMPARE(imageGallery->property("imageAssetVariantCount").toInt(), 6);
        QCOMPARE(imageTable->rowCount(),
                 commonWidget::VisualAssetCatalog::imageDescriptors().size());
        QCOMPARE(imageTable->item(0, 0)->text(),
                 QStringLiteral("image.foundation.geometric-preview"));
        QVERIFY(!commonWidget::ComponentFactory::isRegistered(
            QStringLiteral("commonWidget::ImageAsset")));

        QTreeWidgetItem* modelAssetPage = pageItem(tree, QStringLiteral("2127:5"));
        QVERIFY(modelAssetPage);
        QCOMPARE(modelAssetPage->data(0, Qt::UserRole + 3).toString(), QStringLiteral("assets"));
        tree->setCurrentItem(modelAssetPage);
        QCoreApplication::processEvents();
        auto* modelGallery = pages->currentWidget()->findChild<QWidget*>("modelAssetGallery");
        auto* modelTable = pages->currentWidget()->findChild<QTableWidget*>(
            "visualAssetCatalogTable");
        QVERIFY(modelGallery && modelTable);
        QVERIFY(modelGallery->property("modelAssetUsesSdkCatalog").toBool());
        QVERIFY(modelGallery->property("modelAssetIs2D").toBool());
        QCOMPARE(modelGallery->property("modelAssetVariantCount").toInt(), 3);
        QCOMPARE(modelTable->rowCount(),
                 commonWidget::VisualAssetCatalog::modelDescriptors().size());
        QCOMPARE(modelTable->item(0, 0)->text(),
                 QStringLiteral("model.foundation.schematic"));
        QVERIFY(!commonWidget::ComponentFactory::isRegistered(
            QStringLiteral("commonWidget::ModelAsset")));
    }
    void figmaPageAndVariantContract() {
        auto* tree = part<QTreeWidget>("componentTree");
        QVERIFY(tree);
        QCOMPARE(tree->topLevelItemCount(), 3);
        QVERIFY(tree->topLevelItem(0)->child(0)->text(0).startsWith(
            QString::fromUtf8("20.1 · Navigation / 导航 (")));

        QTreeWidgetItem* checkboxPage = pageItem(tree, QStringLiteral("234:2717"));
        QVERIFY(checkboxPage);
        tree->setCurrentItem(checkboxPage);
        QCoreApplication::processEvents();

        auto* pages = part<QStackedWidget>("componentPages");
        QVERIFY(pages && pages->currentWidget());
        QVERIFY(!pages->currentWidget()->findChild<QTabWidget*>("componentSetTabs"));
        QVERIFY(!pages->currentWidget()->findChild<QTableWidget*>("variantMatrix_235_2710"));
        auto* view=pages->currentWidget()->findChild<QGraphicsView*>("figmaVariantGallery");
        QVERIFY(view);QCOMPARE(view->property("galleryNodeId").toString(),QString("2015:20254"));
        QWidget* canvas=nullptr;
        for(auto* item:view->scene()->items())
            if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
        QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(1312,991));
        auto* single=canvas->findChild<QWidget*>("figmaSet_235_2710");
        auto* group=canvas->findChild<QWidget*>("figmaSet_235_2783");
        QVERIFY(single&&group);
        QCOMPARE(single->geometry(),QRect(206,85,900,540));
        QCOMPARE(group->geometry(),QRect(280,643,753,320));
        QCOMPARE(single->findChildren<Z_CheckBox*>().size(),30);
        QCOMPARE(group->findChildren<Z_CheckBox*>().size(),12);
        QSet<QString> identities;
        for(auto* checkbox:single->findChildren<Z_CheckBox*>()) {
            identities.insert(checkbox->property("figmaVariant").toString());
            QCOMPARE(checkbox->appearance(),QString("figma-light"));
            QCOMPARE(checkbox->designContentRect().translated(checkbox->pos()),checkbox->property("figmaDesignBounds").toRect());
            QCOMPARE(checkbox->isTristate(),false);
            const QString value=checkbox->property("figmaValue").toString();
            QCOMPARE(checkbox->isChecked(),value=="Checked");
            QCOMPARE(checkbox->isIndeterminate(),value=="Indeterminate");
        }
        QCOMPARE(identities.size(),30);
        auto* value=pages->currentWidget()->findChild<QComboBox*>("checkboxFigmaValue");
        auto* state=pages->currentWidget()->findChild<QComboBox*>("checkboxFigmaState");
        QVERIFY(value&&state);value->setCurrentText("Indeterminate");state->setCurrentText("Hover");
        auto* editor=pages->currentWidget()->findChild<QWidget*>("checkboxAppearanceTab");
        QVERIFY(editor);
        Z_CheckBox* interactive=nullptr;
        for(auto* candidate:editor->findChildren<Z_CheckBox*>())
            if(candidate->property("figmaPreviewState").toString()=="Hover")interactive=candidate;
        QVERIFY(interactive);QVERIFY(interactive->isIndeterminate());QVERIFY(!interactive->isChecked());
        QVERIFY(!interactive->isTristate());
        state->setCurrentIndex(0);value->setCurrentText("Checked");
    }
    void buttonGalleryMatchesAuthoredComposition() {
        auto* tree=part<QTreeWidget>("componentTree");
        for(QTreeWidgetItemIterator it(tree);*it;++it)
            if((*it)->data(0,Qt::UserRole)=="172:2710")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();
        auto* page=part<QStackedWidget>("componentPages")->currentWidget();
        QVERIFY(!page->findChild<QTabWidget*>("componentSetTabs"));
        auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);
        QCOMPARE(view->property("galleryNodeId").toString(),QString("2015:17140"));
        QWidget* canvas=nullptr;
        for(auto* item:view->scene()->items())
            if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
        QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(1336,2867));
        const auto buttons=canvas->findChildren<Z_PushButton*>();QCOMPARE(buttons.size(),132);
        QSet<QString> ids;int loading=0,disabled=0,iconOnly=0;
        for(auto* b:buttons) {
            ids.insert(b->property("figmaNodeId").toString());
            QCOMPARE(b->geometry(),b->property("figmaDesignBounds").toRect());
            QVERIFY(b->property("figmaAppearance").toBool());
            QCOMPARE(b->findChild<ButtonAppearanceAdapter*>()->appearance(),QString("figma-light"));
            const auto state=b->property("figmaPreviewState").toString();
            loading+=b->property("loading").toBool();disabled+=state=="disabled";iconOnly+=b->isIconOnly();
            QCOMPARE(b->isEnabled(),state!="disabled" && state!="loading");
            QVERIFY(!b->isDown());
        }
        QCOMPARE(ids.size(),132);QCOMPARE(loading,22);QCOMPARE(disabled,22);QCOMPARE(iconOnly,24);
        QCOMPARE(canvas->findChild<QWidget*>("figmaSet_173_2710")->geometry(),QRect(40,98,1200,290));
        auto* material=page->findChild<QComboBox*>("buttonAppearance");QVERIFY(material);
        QCOMPARE(material->currentData().toString(),QString("figma-light"));
    }
    void labelGalleryUsesTwelveNativeVariants() {
        auto* tree=part<QTreeWidget>("componentTree");
        for(QTreeWidgetItemIterator it(tree);*it;++it)
            if((*it)->data(0,Qt::UserRole)=="414:7")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();
        QVERIFY(!page->findChild<QTabWidget*>("componentSetTabs"));
        auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);
        QCOMPARE(view->property("galleryNodeId").toString(),QString("2014:2818"));
        QWidget* canvas=nullptr;
        for(auto* item:view->scene()->items())
            if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
        QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(1596,603));
        int count=0,wrapped=0,disabled=0;QSet<QString> ids;
        for(auto* label:canvas->findChildren<QLabel*>()) {
            if(label->property("nativeQtClass")!="QLabel")continue;
            ++count;wrapped+=label->wordWrap();disabled+=!label->isEnabled();
            ids.insert(label->property("figmaNodeId").toString());
            QCOMPARE(label->geometry(),label->property("figmaDesignBounds").toRect());
            QCOMPARE(label->text(),QString::fromUtf8("设备状态：运行中"));
            QVERIFY(label->alignment()&Qt::AlignAbsolute);
            QVERIFY((QStringList{"Start","Center","End"}.contains(label->property("figmaLogicalAlignment").toString())));
            QCOMPARE(label->font().pixelSize(),14);
            QCOMPARE(QFontInfo(label->font()).family(),QString("Inter"));
        }
        QCOMPARE(count,12);QCOMPARE(ids.size(),12);QCOMPARE(wrapped,6);QCOMPARE(disabled,6);
    }
    void progressGalleryAndEditorUseActualFigmaVariants() {
        auto* tree=part<QTreeWidget>("componentTree");
        for(QTreeWidgetItemIterator it(tree);*it;++it)
            if((*it)->data(0,Qt::UserRole)=="410:13")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();
        QVERIFY(!page->findChild<QTabWidget*>("componentSetTabs"));
        auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);
        QCOMPARE(view->property("galleryNodeId").toString(),QString("2014:2291"));
        QWidget* canvas=nullptr;
        for(auto* item:view->scene()->items())
            if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
        QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(1556,599));
        const auto bars=canvas->findChildren<QProgressBar*>();QCOMPARE(bars.size(),20);
        QSet<QString> ids;int vertical=0,inverted=0,busy=0,disabled=0;
        for(auto* bar:bars) {
            ids.insert(bar->property("figmaNodeId").toString());
            QCOMPARE(bar->geometry(),bar->property("figmaDesignBounds").toRect());
            QCOMPARE(bar->property("nativeQtClass").toString(),QString("QProgressBar"));
            vertical+=bar->orientation()==Qt::Vertical;inverted+=bar->invertedAppearance();
            disabled+=!bar->isEnabled();
            if(bar->maximum()==0) {++busy;QVERIFY(bar->text().isEmpty());}
            else QCOMPARE(bar->value(),68);
            QVERIFY(bar->property("figmaProgressSnapshot").toBool());
            QCOMPARE(QFontInfo(bar->font()).family(),QString("Inter"));
        }
        QCOMPARE(ids.size(),20);QCOMPARE(vertical,6);QCOMPARE(inverted,10);QCOMPARE(busy,4);QCOMPARE(disabled,4);
        auto* preview=page->findChild<QProgressBar*>("componentInteractivePreview");QVERIFY(preview);
        auto* viewport=page->findChild<QScrollArea*>("componentDetailViewport");QVERIFY(viewport);
        QCOMPARE(viewport->widget(),page->findChild<QSplitter*>("componentDetailSplitter"));
        QVERIFY(page->findChild<QScrollArea*>("componentSettingsScroll"));
        auto* value=page->findChild<QSpinBox*>("progressValueControl");QVERIFY(value);
        value->setValue(42);QCOMPARE(preview->value(),42);
        auto* variant=page->findChild<QComboBox*>("componentVariantControl");QVERIFY(variant);QCOMPARE(variant->count(),20);
        variant->setCurrentIndex(7);QCoreApplication::processEvents();
        QCOMPARE(preview->orientation(),Qt::Vertical);QVERIFY(preview->invertedAppearance());
        variant->setCurrentIndex(2);QCoreApplication::processEvents();
        QCOMPARE(preview->maximum(),0);QVERIFY(preview->text().isEmpty());
        variant->setCurrentIndex(18);QCoreApplication::processEvents();
        QCOMPARE(preview->value(),42);QCOMPARE(preview->format(),QString("Completed"));
        page->findChild<QCheckBox*>("progressTextVisibleControl")->setChecked(false);
        QVERIFY(!preview->isTextVisible());
    }
    void radioGalleryAndEditorsPreserveCompositionAndInteraction() {
        auto* tree=part<QTreeWidget>("componentTree");
        for(QTreeWidgetItemIterator it(tree);*it;++it)
            if((*it)->data(0,Qt::UserRole)=="341:2")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();
        QVERIFY(!page->findChild<QTabWidget*>("componentSetTabs"));
        auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);
        QCOMPARE(view->property("galleryNodeId").toString(),QString("2015:21292"));
        QWidget* canvas=nullptr;
        for(auto* item:view->scene()->items())
            if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
        QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(1312,671));
        const auto radios=canvas->findChildren<QRadioButton*>();QCOMPARE(radios.size(),22);
        const auto groups=canvas->findChildren<QButtonGroup*>();QCOMPARE(groups.size(),4);
        QSet<QString> ids;int checked=0,disabled=0;
        for(auto* radio:radios) {
            ids.insert(radio->property("figmaNodeId").toString());
            if(radio->parentWidget()->layout())radio->parentWidget()->layout()->activate();
            QCOMPARE(radio->geometry(),radio->property("figmaDesignBounds").toRect());
            QCOMPARE(radio->property("designSystemClass").toString(),QString("commonWidget::Z_RadioButton"));
            checked+=radio->isChecked();disabled+=!radio->isEnabled();QVERIFY(!radio->isDown());
        }
        QCOMPARE(ids.size(),22);QVERIFY(!ids.contains(""));QCOMPARE(checked,9);QCOMPARE(disabled,8);
        for(auto* group:groups) {
            QCOMPARE(group->checkedId(),1);QVERIFY(group->exclusive());
            QCOMPARE(group->buttons().size(),3);
        }
        auto* preview=page->findChild<QRadioButton*>("componentInteractivePreview");QVERIFY(preview);
        auto* variant=page->findChild<QComboBox*>("componentVariantControl");QVERIFY(variant);QCOMPARE(variant->count(),10);
        auto* control=page->findChild<QCheckBox*>("radioCheckedControl");QVERIFY(control);
        control->setChecked(true);QVERIFY(preview->isChecked());control->setChecked(false);QVERIFY(!preview->isChecked());
        variant->setCurrentIndex(7);QVERIFY(preview->isChecked());QVERIFY(!preview->isDown());
        auto* snapshot=page->findChild<QCheckBox*>("radioSnapshotControl");QVERIFY(snapshot);
        snapshot->setChecked(true);QCOMPARE(preview->property("figmaRadioSnapshotState").toString(),QString("Pressed"));
        snapshot->setChecked(false);QCOMPARE(preview->property("figmaRadioSnapshotState").toString(),QString());
        variant->setCurrentIndex(0);QVERIFY(!preview->isChecked());
        page->findChild<QComboBox*>("appearanceComponentSetControl")->setCurrentIndex(1);
        auto* groupHost=page->findChild<QWidget*>("radioGroupInteractivePreview");QVERIFY(groupHost);
        auto* group=groupHost->findChild<QButtonGroup*>("radioExclusiveGroup");QVERIFY(group);
        auto* selected=page->findChild<QSpinBox*>("radioGroupSelectedIdControl");selected->setValue(3);
        QCOMPARE(group->checkedId(),3);QVERIFY(!groupHost->property("publicAdapterImplemented").toBool());
        auto* groupVariant=page->findChild<QComboBox*>("radioGroupVariantControl");QCOMPARE(groupVariant->count(),4);
        groupVariant->setCurrentIndex(2);QCOMPARE(groupHost->minimumSize(),QSize(196,136));QCOMPARE(group->checkedId(),3);
        groupVariant->setCurrentIndex(3);QVERIFY(!groupHost->isEnabled());
        group->button(1)->click();QCOMPARE(group->checkedId(),3);
    }
    void dockGallerySeparatesSnapshotFromNativeWorkspace() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)=="507:2")tree->setCurrentItem(*it);QCoreApplication::processEvents();
        auto* page=part<QStackedWidget>("componentPages")->currentWidget();auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);QCOMPARE(view->property("variantSpecimenCount").toInt(),8);QWidget* canvas=nullptr;for(auto* i:view->scene()->items())if(auto* p=qgraphicsitem_cast<QGraphicsProxyWidget*>(i))canvas=p->widget();QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(1312,783));
        const auto specimens=canvas->findChildren<QDockWidget*>();QCOMPARE(specimens.size(),10);QSet<QString> ids;int floatingPreviews=0;
        for(auto* d:specimens){ids.insert(d->property("figmaNodeId").toString());QCOMPARE(d->geometry(),d->property("figmaDesignBounds").toRect());QVERIFY(!d->isFloating());QVERIFY(!d->titleBarWidget());QVERIFY(!d->layout()->isEnabled());d->grab();QCOMPARE(d->findChild<QAbstractButton*>("qt_dockwidget_closebutton")->geometry(),QRect(233,10,16,16));QCOMPARE(d->findChild<QAbstractButton*>("qt_dockwidget_floatbutton")->geometry(),QRect(209,10,16,16));QCOMPARE(d->findChild<QWidget*>("dockDemoCard")->mapTo(d,QPoint()),QPoint(11,47));QCOMPARE(d->findChild<QWidget*>("dockDemoCard")->size(),QSize(238,102));QCOMPARE(QRawFont::fromFont(d->property("figmaDockPaintFont").value<QFont>()).weight(),int(QFont::Medium));if(d->property("figmaDockPlacementPreview")=="Floating"){++floatingPreviews;QCOMPARE(d->property("figmaDockEdgeBounds").toRect(),QRect());}}
        QCOMPARE(ids.size(),10);QCOMPARE(floatingPreviews,5);auto* dock=page->findChild<QDockWidget*>("componentInteractivePreview");auto* main=page->findChild<QMainWindow*>("dockWorkspace");QVERIFY(dock&&main);QCOMPARE(main->dockWidgetArea(dock),Qt::LeftDockWidgetArea);auto* child=dock->widget();const auto allowed=dock->allowedAreas();const auto features=dock->features();
        auto* variant=page->findChild<QComboBox*>("componentVariantControl");QCOMPARE(variant->count(),8);variant->setCurrentIndex(7);QCoreApplication::processEvents();QVERIFY(!dock->isFloating());QCOMPARE(dock->allowedAreas(),allowed);QCOMPARE(dock->features(),features);QCOMPARE(dock->widget(),child);page->findChild<QLineEdit*>("componentTextControl")->setText("Changed inspector");QCOMPARE(dock->windowTitle(),QString("Changed inspector"));
        page->findChild<QCheckBox*>("dockCloseGlyphControl")->setChecked(false);QCOMPARE(dock->features(),features);page->findChild<QCheckBox*>("dockClosableControl")->setChecked(false);QVERIFY(!dock->features().testFlag(QDockWidget::DockWidgetClosable));page->findChild<QCheckBox*>("dockClosableControl")->setChecked(true);
        page->findChild<QComboBox*>("dockAreaControl")->setCurrentText("Right");page->findChild<QPushButton*>("dockMoveControl")->click();QCOMPARE(main->dockWidgetArea(dock),Qt::RightDockWidgetArea);page->findChild<QPushButton*>("dockSaveControl")->click();page->findChild<QComboBox*>("dockAreaControl")->setCurrentText("Top");page->findChild<QCheckBox*>("dockAllowTop")->setChecked(false);page->findChild<QPushButton*>("dockMoveControl")->click();QCOMPARE(main->dockWidgetArea(dock),Qt::RightDockWidgetArea);
        dock->close();QVERIFY(dock->isHidden());page->findChild<QPushButton*>("dockShowControl")->click();QVERIFY(!dock->isHidden());page->findChild<QPushButton*>("dockRestoreControl")->click();QVERIFY(page->findChild<QLabel*>("dockNativeReadout")->text().contains("restoreState=1"));QCOMPARE(dock->widget(),child);
        page->findChild<QCheckBox*>("dockLiveAppearanceControl")->setChecked(true);QVERIFY(dock->property("figmaDockAreaPreview").toString().isEmpty());page->findChild<QCheckBox*>("dockRtlControl")->setChecked(true);QCOMPARE(dock->layoutDirection(),Qt::RightToLeft);QTest::qWait(50);QCOMPARE(page->findChild<QScrollArea*>("componentSettingsScroll")->horizontalScrollBar()->maximum(),0);
    }
    void toolBoxGalleryKeepsSingleCurrentCompositionAndNativeEditor() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)=="505:2")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);QCOMPARE(view->property("variantSpecimenCount").toInt(),5);QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(1312,783));
        const auto specimens=canvas->findChildren<QToolBox*>();QCOMPARE(specimens.size(),6);QSet<QString> ids;QToolBox* composition=nullptr;int anatomy=0,disabled=0,headerCount=0;
        for(auto* box:specimens){ids.insert(box->property("figmaNodeId").toString());QCOMPARE(box->geometry(),box->property("figmaDesignBounds").toRect());QVERIFY(!box->layout()->isEnabled());if(box->property("figmaToolBoxAnatomyOnly").toBool()){++anatomy;QCOMPARE(box->size(),QSize(760,48));QCOMPARE(box->count(),1);disabled+=!box->isItemEnabled(0);}else composition=box;for(auto* button:box->findChildren<QAbstractButton*>("qt_toolbox_toolboxbutton",Qt::FindDirectChildrenOnly)){++headerCount;button->grab();QCOMPARE(button->size(),QSize(760,48));QCOMPARE(button->property("figmaToolBoxArrowBounds").toRect(),QRect(19,16,16,16));QCOMPARE(QRawFont::fromFont(button->property("figmaToolBoxPaintFont").value<QFont>()).weight(),int(QFont::DemiBold));QVERIFY(!button->hasFocus());}}
        QCOMPARE(ids.size(),6);QCOMPARE(anatomy,5);QCOMPARE(disabled,1);QCOMPARE(headerCount,8);QVERIFY(composition);QCOMPARE(composition->currentIndex(),1);QCOMPARE(composition->count(),3);QCOMPARE(composition->currentWidget()->mapTo(composition,QPoint()),QPoint(17,113));QCOMPARE(composition->currentWidget()->size(),QSize(766,133));
        auto* box=page->findChild<QToolBox*>("componentInteractivePreview");QVERIFY(box);QVERIFY(box->layout()->isEnabled());QCOMPARE(box->currentIndex(),1);auto* target=page->findChild<QSpinBox*>("toolBoxTargetControl");target->setValue(0);page->findChild<QLineEdit*>("componentTextControl")->setText("&Changed");QCOMPARE(box->itemText(0),QString("&Changed"));page->findChild<QCheckBox*>("toolBoxIconControl")->setChecked(true);QVERIFY(!box->itemIcon(0).isNull());
        page->findChild<QComboBox*>("toolBoxSnapshotControl")->setCurrentText("Selected");QCOMPARE(box->currentIndex(),1);auto* variant=page->findChild<QComboBox*>("componentVariantControl");QCOMPARE(variant->count(),5);variant->setCurrentIndex(4);QCoreApplication::processEvents();QVERIFY(box->isEnabled());QVERIFY(!box->isItemEnabled(0));QCOMPARE(box->currentIndex(),1);variant->setCurrentIndex(0);QCoreApplication::processEvents();QVERIFY(box->isItemEnabled(0));
        page->findChild<QSpinBox*>("toolBoxCurrentControl")->setValue(0);QCOMPARE(box->currentIndex(),0);page->findChild<QCheckBox*>("toolBoxEnabledControl")->setChecked(false);QCOMPARE(box->currentIndex(),1);QVERIFY(box->isEnabled());page->findChild<QCheckBox*>("toolBoxRtlControl")->setChecked(true);QCOMPARE(box->layoutDirection(),Qt::RightToLeft);QTest::qWait(60);QCOMPARE(page->findChild<QScrollArea*>("componentSettingsScroll")->horizontalScrollBar()->maximum(),0);
    }
    void tabsGalleryKeepsAnatomyCompositionAndNativePages() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)=="500:2")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);QCOMPARE(view->property("variantSpecimenCount").toInt(),20);
        QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(1312,783));
        const auto specimens=canvas->findChildren<QTabWidget*>();QCOMPARE(specimens.size(),21);QSet<QString> ids;int anatomy=0,disabled=0;QTabWidget* composition=nullptr;
        for(auto* tabs:specimens){ids.insert(tabs->property("figmaNodeId").toString());QCOMPARE(tabs->geometry(),tabs->property("figmaDesignBounds").toRect());QVERIFY(tabs->property("figmaTabSourceSnapshot").toBool());QVERIFY(!tabs->hasFocus());if(tabs->property("figmaTabAnatomyOnly").toBool()){++anatomy;QCOMPARE(tabs->count(),1);QCOMPARE(tabs->size(),QSize(176,48));tabs->grab();QCOMPARE(tabs->tabBar()->geometry(),QRect(0,0,176,48));QCOMPARE(tabs->tabBar()->tabRect(0),QRect(0,0,176,48));disabled+=!tabs->isTabEnabled(0);}else composition=tabs;}
        QCOMPARE(ids.size(),21);QCOMPARE(anatomy,20);QCOMPARE(disabled,4);QVERIFY(composition);QCOMPARE(composition->count(),4);composition->grab();QCOMPARE(composition->tabBar()->geometry(),QRect(16,16,704,48));QCOMPARE(composition->currentWidget()->geometry().size(),QSize(766,164));QCOMPARE(composition->currentWidget()->mapTo(composition,QPoint()),QPoint(17,65));const auto paintedFont=composition->tabBar()->property("figmaTabPaintFont").value<QFont>();QCOMPARE(QRawFont::fromFont(paintedFont).weight(),int(QFont::DemiBold));QCOMPARE(paintedFont.pixelSize(),14);
        for(auto* label:canvas->findChildren<QLabel*>())if(label->property("figmaNodeId")=="500:898"){QCOMPARE(label->font().pixelSize(),11);QCOMPARE(QRawFont::fromFont(label->font()).weight(),int(QFont::Medium));}
        auto* closeButton=qobject_cast<QAbstractButton*>(composition->tabBar()->tabButton(2,QTabBar::RightSide));QVERIFY(closeButton);QCOMPARE(closeButton->geometry(),QRect(500,15,16,16));
        QTabBar* overflow=nullptr;for(auto* bar:canvas->findChildren<QTabBar*>())if(bar->property("figmaNodeId")=="500:878")overflow=bar;QVERIFY(overflow);overflow->grab();QCOMPARE(overflow->size(),QSize(356,48));int scrollers=0;for(auto* button:overflow->findChildren<QToolButton*>()){if(button->isVisibleTo(overflow)){++scrollers;QCOMPARE(button->size(),QSize(22,48));}}QCOMPARE(scrollers,2);
        auto* tabs=page->findChild<QTabWidget*>("componentInteractivePreview");QVERIFY(tabs);QCOMPARE(tabs->count(),4);auto* target=page->findChild<QSpinBox*>("tabTargetControl");QVERIFY(target);target->setValue(2);page->findChild<QLineEdit*>("componentTextControl")->setText("Changed");QCOMPARE(tabs->tabText(2),QString("Changed"));
        QSignalSpy requests(tabs,&QTabWidget::tabCloseRequested);auto* nativeClose=qobject_cast<QAbstractButton*>(tabs->tabBar()->tabButton(2,QTabBar::RightSide));QVERIFY(nativeClose);auto* behavior=tabs->widget(2);nativeClose->click();QCOMPARE(requests.count(),1);QCOMPARE(tabs->count(),4);QCOMPARE(tabs->widget(2),behavior);QCOMPARE(tabs->property("tabCloseRequests").toInt(),1);
        page->findChild<QCheckBox*>("tabMovableControl")->setChecked(true);tabs->tabBar()->moveTab(2,1);QCOMPARE(tabs->widget(1),behavior);QCOMPARE(tabs->property("tabMoveCount").toInt(),1);
        target->setValue(1);auto* state=page->findChild<QComboBox*>("tabSnapshotControl");const auto current=tabs->currentIndex();state->setCurrentText("Selected");QCOMPARE(tabs->currentIndex(),current);state->setCurrentText("Focus");QVERIFY(!tabs->tabBar()->hasFocus());
        auto* variant=page->findChild<QComboBox*>("componentVariantControl");QCOMPARE(variant->count(),20);variant->setCurrentIndex(4);QCoreApplication::processEvents();QVERIFY(tabs->isEnabled());QVERIFY(!tabs->isTabEnabled(1));QVERIFY(tabs->isTabEnabled(0));variant->setCurrentIndex(0);QCoreApplication::processEvents();QVERIFY(tabs->isTabEnabled(1));
        page->findChild<QComboBox*>("tabPositionControl")->setCurrentText("Left");QCOMPARE(tabs->tabPosition(),QTabWidget::West);page->findChild<QCheckBox*>("tabRtlControl")->setChecked(true);QCOMPARE(tabs->layoutDirection(),Qt::RightToLeft);
        page->findChild<QComboBox*>("tabClosePolicyControl")->setCurrentIndex(2);QVERIFY(!tabs->tabsClosable());QCOMPARE(tabs->count(),4);QTest::qWait(60);QCOMPARE(page->findChild<QScrollArea*>("componentSettingsScroll")->horizontalScrollBar()->maximum(),0);
    }
    void dialogActionsGalleryKeepsAuthoredOrderAndNativeRoleEditor() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)=="447:2")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);QCOMPARE(view->property("variantSpecimenCount").toInt(),32);
        QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(2256,1893));
        const auto specimens=canvas->findChildren<QDialogButtonBox*>();QCOMPARE(specimens.size(),32);QSet<QString> ids,buttonIds;int children=0,disabled=0,vertical=0,centered=0,destructive=0;
        for(auto* box:specimens){ids.insert(box->property("figmaNodeId").toString());QCOMPARE(box->geometry(),box->property("figmaDesignBounds").toRect());QVERIFY(box->property("figmaDialogSourceSnapshot").toBool());QVERIFY(!box->layout()->isEnabled());disabled+=!box->isEnabled();vertical+=box->orientation()==Qt::Vertical;centered+=box->centerButtons();for(auto* button:box->buttons()){++children;buttonIds.insert(button->property("figmaNodeId").toString());QVERIFY(qobject_cast<Z_PushButton*>(button));QCOMPARE(button->geometry(),button->property("figmaDesignBounds").toRect());QVERIFY(button->property("figmaAppearance").toBool());QVERIFY(!button->isDown());QVERIFY(!button->hasFocus());QCOMPARE(button->height(),32);destructive+=box->buttonRole(button)==QDialogButtonBox::DestructiveRole;}}
        QCOMPARE(ids.size(),32);QCOMPARE(children,80);QCOMPARE(buttonIds.size(),80);QCOMPARE(disabled,16);QCOMPARE(vertical,16);QCOMPARE(centered,16);QCOMPARE(destructive,8);
        auto* box=page->findChild<QDialogButtonBox*>("componentInteractivePreview");QVERIFY(box);QVERIFY(box->layout()->isEnabled());auto* configuration=page->findChild<QComboBox*>("dialogConfigurationControl");QVERIFY(configuration);QCOMPARE(box->standardButtons(),QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel));QSignalSpy accepted(box,&QDialogButtonBox::accepted),rejected(box,&QDialogButtonBox::rejected);
        configuration->setCurrentText("Destructive");auto* action=box->findChild<QPushButton*>("dialogDeleteAction");QVERIFY(action);QCOMPARE(box->buttonRole(action),QDialogButtonBox::DestructiveRole);QVERIFY(!action->isDefault());QVERIFY(!action->autoDefault());action->click();QCOMPARE(accepted.count(),0);QCOMPARE(rejected.count(),0);QCOMPARE(box->property("dialogClicks").toInt(),1);QCOMPARE(box->property("dialogLastRole").toInt(),int(QDialogButtonBox::DestructiveRole));
        page->findChild<QComboBox*>("dialogOrientationControl")->setCurrentText("Vertical");page->findChild<QCheckBox*>("dialogCenteredControl")->setChecked(true);page->findChild<QCheckBox*>("dialogRtlControl")->setChecked(true);QCOMPARE(box->orientation(),Qt::Vertical);QVERIFY(box->centerButtons());QCOMPARE(box->layoutDirection(),Qt::RightToLeft);QCOMPARE(box->findChild<QPushButton*>("dialogDeleteAction"),action);
        configuration->setCurrentText("Help");QVERIFY(box->button(QDialogButtonBox::Help));QCOMPARE(box->buttons().size(),3);box->button(QDialogButtonBox::Help)->click();QCOMPARE(box->property("dialogHelp").toInt(),1);
        auto* variant=page->findChild<QComboBox*>("componentVariantControl");QCOMPARE(variant->count(),32);variant->setCurrentIndex(1);QCoreApplication::processEvents();QVERIFY(!box->isEnabled());QCOMPARE(configuration->currentText(),QString("Confirm"));QCOMPARE(box->orientation(),Qt::Horizontal);
        variant->setCurrentIndex(0);QCoreApplication::processEvents();page->findChild<QPushButton*>("dialogOpenControl")->click();auto* dialog=page->findChild<QDialog*>("dialogKeyboardTest");QVERIFY(dialog);QVERIFY(dialog->isVisible());QTest::keyClick(dialog,Qt::Key_Escape);QCoreApplication::processEvents();
        QTest::qWait(60);QCOMPARE(page->findChild<QScrollArea*>("componentSettingsScroll")->horizontalScrollBar()->maximum(),0);
    }
    void toolButtonGalleryKeepsNativeActionAndSplitIdentity() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)=="443:2")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);QCOMPARE(view->property("variantSpecimenCount").toInt(),34);
        QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(2256,873));
        const auto specimens=canvas->findChildren<QToolButton*>();QCOMPARE(specimens.size(),34);QSet<QString> ids;int disabled=0,checked=0,split=0,raised=0,open=0;
        for(auto* button:specimens){ids.insert(button->property("figmaNodeId").toString());QCOMPARE(button->font().pixelSize(),14);QCOMPARE(button->font().weight(),int(QFont::DemiBold));QCOMPARE(button->geometry(),button->property("figmaDesignBounds").toRect());QVERIFY(button->property("figmaToolSourceSnapshot").toBool());QVERIFY(!button->isDown());QVERIFY(!button->hasFocus());QVERIFY(!button->menu());disabled+=!button->isEnabled();checked+=button->isChecked();split+=button->popupMode()==QToolButton::MenuButtonPopup;raised+=button->autoRaise();open+=button->property("figmaToolSnapshotState")=="Open";button->grab();const auto paintFont=button->property("figmaToolPaintFont").value<QFont>();const auto family=QRawFont::fromFont(paintFont).familyName();QVERIFY(family=="Inter"||family=="Inter SemiBold");QCOMPARE(paintFont.pixelSize(),14);QCOMPARE(QRawFont::fromFont(paintFont).weight(),int(QFont::DemiBold));QCOMPARE(button->property("figmaToolIconBounds").toRect().size(),QSize(16,16));}
        QCOMPARE(ids.size(),34);QCOMPARE(disabled,4);QCOMPARE(checked,12);QCOMPARE(split,18);QCOMPARE(raised,17);QCOMPARE(open,2);
        auto* button=page->findChild<QToolButton*>("componentInteractivePreview");QVERIFY(button);QVERIFY(button->menu());QCOMPARE(button->height(),32);
        page->findChild<QCheckBox*>("toolActionSourceControl")->setChecked(true);auto* action=button->defaultAction();QVERIFY(action);
        page->findChild<QLineEdit*>("componentTextControl")->setText("Updated");QCOMPARE(action->text(),QString("Updated"));QCOMPARE(button->text(),QString("Updated"));
        page->findChild<QCheckBox*>("toolCheckableControl")->setChecked(true);button->click();QVERIFY(action->isChecked());QVERIFY(button->isChecked());
        page->findChild<QComboBox*>("toolPopupControl")->setCurrentText("MenuButtonPopup");page->findChild<QComboBox*>("toolSnapshotControl")->setCurrentText("Open");QVERIFY(!button->menu()->isVisible());QVERIFY(!button->isDown());QVERIFY(action->isChecked());
        page->findChild<QComboBox*>("toolStyleControl")->setCurrentText("Icon only");QCOMPARE(button->toolButtonStyle(),Qt::ToolButtonIconOnly);QCOMPARE(action->text(),QString("Updated"));
        page->findChild<QCheckBox*>("toolRtlControl")->setChecked(true);QCOMPARE(button->layoutDirection(),Qt::RightToLeft);
        page->findChild<QCheckBox*>("toolActionSourceControl")->setChecked(false);QVERIFY(!button->defaultAction());QVERIFY(!button->actions().contains(action));
        QTest::qWait(60);QCOMPARE(page->findChild<QScrollArea*>("componentSettingsScroll")->horizontalScrollBar()->maximum(),0);
    }
    void commandLinkGalleryKeepsAuthoredClippingSeparateFromLiveDescription() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)=="450:2")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);QCOMPARE(view->property("variantSpecimenCount").toInt(),16);
        QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(2256,983));
        const auto specimens=canvas->findChildren<QCommandLinkButton*>();QCOMPARE(specimens.size(),16);QSet<QString> ids;int disabled=0,checked=0,flat=0,focus=0;
        for(auto* button:specimens){ids.insert(button->property("figmaNodeId").toString());QCOMPARE(button->geometry(),button->property("figmaDesignBounds").toRect());QVERIFY(button->property("figmaCommandSourceSnapshot").toBool());QVERIFY(!button->isDown());QVERIFY(!button->hasFocus());disabled+=!button->isEnabled();checked+=button->isChecked();flat+=button->isFlat();focus+=button->height()==80;button->grab();QCOMPARE(button->property("figmaCommandContentBounds").toRect().height(),52);QCOMPARE(button->property("figmaCommandIconBounds").toRect().size(),QSize(16,16));}
        QCOMPARE(ids.size(),16);QCOMPARE(disabled,2);QCOMPARE(checked,6);QCOMPARE(flat,8);QCOMPARE(focus,4);
        auto* button=page->findChild<QCommandLinkButton*>("componentInteractivePreview");QVERIFY(button);const auto description=button->description();
        page->findChild<QComboBox*>("commandSnapshotControl")->setCurrentText("Pressed");QVERIFY(!button->isDown());page->findChild<QCheckBox*>("commandCheckableControl")->setChecked(true);button->click();QVERIFY(button->isChecked());
        page->findChild<QCheckBox*>("commandShowDescriptionControl")->setChecked(false);QCOMPARE(button->description(),description);page->findChild<QCheckBox*>("commandShowDescriptionControl")->setChecked(true);
        page->findChild<QLineEdit*>("commandDescriptionControl")->setText(QString("Description ").repeated(50));QTest::qWait(60);QVERIFY(button->minimumHeight()>80);
        page->findChild<QCheckBox*>("commandRtlControl")->setChecked(true);QCOMPARE(button->layoutDirection(),Qt::RightToLeft);
        QTest::qWait(60);QCOMPARE(page->findChild<QScrollArea*>("componentSettingsScroll")->horizontalScrollBar()->maximum(),0);
    }
    void comboGalleriesKeepNativeSelectionAndFontModel() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");
        for(const auto& pageId:{QString("1120:2"),QString("1121:2")}) {
            for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)==pageId)tree->setCurrentItem(*it);
            QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();
            auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);QCOMPARE(view->property("variantSpecimenCount").toInt(),12);
            QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
            QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(1488,391));const auto specimens=canvas->findChildren<QComboBox*>();QCOMPARE(specimens.size(),12);QSet<QString> ids;int disabled=0;
            for(auto* combo:specimens){ids.insert(combo->property("figmaNodeId").toString());QCOMPARE(combo->geometry(),combo->property("figmaDesignBounds").toRect());
                QVERIFY(combo->property("figmaComboSourceSnapshot").toBool());QVERIFY(!combo->isEditable());QVERIFY(combo->testAttribute(Qt::WA_TransparentForMouseEvents));disabled+=!combo->isEnabled();
                if(pageId=="1120:2"){QCOMPARE(combo->currentIndex(),-1);QVERIFY(combo->currentText().isEmpty());QCOMPARE(combo->placeholderText(),QString("Please select"));}}
            QCOMPARE(ids.size(),12);QCOMPARE(disabled,3);
            auto* combo=page->findChild<QComboBox*>("componentInteractivePreview");QVERIFY(combo);QVERIFY(!combo->property("figmaComboSourceSnapshot").toBool());auto* model=combo->model();auto* delegate=combo->itemDelegate();
            QCOMPARE(page->findChild<QComboBox*>("comboSnapshotControl")->currentData().toString(),combo->property("figmaComboSnapshotState").toString());
            auto* fonts=qobject_cast<QFontComboBox*>(combo);if(fonts)QVERIFY(fonts->isEditable());else QCOMPARE(combo->currentIndex(),-1);
            if(combo->count()>1)combo->setCurrentIndex(1);const auto selected=combo->currentText();
            page->findChild<QComboBox*>("comboSizeControl")->setCurrentText("Large");QCOMPARE(combo->height(),40);QCOMPARE(combo->currentText(),selected);
            page->findChild<QComboBox*>("comboSnapshotControl")->setCurrentText("Focus");QVERIFY(!combo->view()->isVisible());QCOMPARE(combo->currentText(),selected);
            page->findChild<QComboBox*>("componentVariantControl")->setCurrentIndex(2);QCoreApplication::processEvents();QCOMPARE(combo->height(),24);QCOMPARE(combo->property("figmaComboSnapshotState").toString(),QString("Focus"));QCOMPARE(combo->currentText(),selected);
            QCOMPARE(combo->model(),model);QCOMPARE(combo->itemDelegate(),delegate);
            page->findChild<QCheckBox*>("comboEditableControl")->setChecked(true);QVERIFY(combo->lineEdit());auto* editor=combo->lineEdit();
            page->findChild<QComboBox*>("comboSizeControl")->setCurrentText("Middle");QCOMPARE(combo->lineEdit(),editor);
            if(fonts){page->findChild<QComboBox*>("fontFiltersControl")->setCurrentText("MonospacedFonts");QCOMPARE(fonts->fontFilters(),QFontComboBox::FontFilters(QFontComboBox::MonospacedFonts));QCOMPARE(fonts->model(),model);QCOMPARE(fonts->itemDelegate(),delegate);}
            else {const int count=combo->count();page->findChild<QPushButton*>("comboResetControl")->click();QCOMPARE(combo->currentIndex(),-1);QCOMPARE(combo->count(),count);QVERIFY(combo->currentText().isEmpty());}
            page->findChild<QCheckBox*>("comboRtlControl")->setChecked(true);QCOMPARE(combo->layoutDirection(),Qt::RightToLeft);
            QTest::qWait(60);auto* settings=page->findChild<QScrollArea*>("componentSettingsScroll");QVERIFY(settings);
            QCOMPARE(settings->horizontalScrollBar()->maximum(),0);
        }
    }
    void shortcutGallerySeparatesSnapshotsFromNativeCandidate() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)=="478:337")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();
        auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);QCOMPARE(view->property("variantSpecimenCount").toInt(),6);
        QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
        QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(1312,353));const auto specimens=canvas->findChildren<QKeySequenceEdit*>();QCOMPARE(specimens.size(),6);
        QSet<QString> ids;int disabled=0,empty=0;
        for(auto* edit:specimens){ids.insert(edit->property("figmaNodeId").toString());QCOMPARE(edit->geometry(),edit->property("figmaDesignBounds").toRect());
            QCOMPARE(edit->property("designSystemClass").toString(),QString("commonWidget::Z_KeySequenceEdit"));QVERIFY(edit->property("figmaShortcutSourceSnapshot").toBool());
            QVERIFY(!edit->property("figmaShortcutRecording").toBool());disabled+=!edit->isEnabled();empty+=edit->keySequence().isEmpty();
            auto* clear=edit->findChild<QAbstractButton*>("figmaShortcutClear");QVERIFY(clear);QCOMPARE(clear->geometry(),QRect(248,8,24,24));
            QCOMPARE(edit->findChild<QLineEdit*>("qt_keysequenceedit_lineedit")->geometry(),QRect(0,0,280,40));}
        QCOMPARE(ids.size(),6);QCOMPARE(disabled,1);QCOMPARE(empty,1);
        auto* captured=specimens.first();auto* capturedLine=captured->findChild<QLineEdit*>("qt_keysequenceedit_lineedit");
        const auto sourceTextImage=capturedLine->grab().toImage();captured->setKeySequence(QKeySequence("Alt+X"));QCoreApplication::processEvents();
        QVERIFY(sourceTextImage==capturedLine->grab().toImage());
        auto* edit=page->findChild<QKeySequenceEdit*>("componentInteractivePreview");QVERIFY(edit);QVERIFY(!edit->property("figmaShortcutSourceSnapshot").toBool());
        auto* sequence=page->findChild<QLineEdit*>("shortcutSequenceControl");sequence->setText("Ctrl+S");page->findChild<QPushButton*>("shortcutApplyControl")->click();
        QCOMPARE(edit->keySequence(),QKeySequence("Ctrl+S"));QVERIFY(edit->property("figmaShortcutConflict").toString().contains("Save"));
        page->findChild<QCheckBox*>("shortcutRegistryControl")->setChecked(false);QVERIFY(edit->property("figmaShortcutConflict").toString().isEmpty());
        page->findChild<QPushButton*>("shortcutSaveControl")->click();page->findChild<QPushButton*>("shortcutClearControl")->click();QVERIFY(edit->keySequence().isEmpty());
        page->findChild<QPushButton*>("shortcutRestoreControl")->click();QCOMPARE(edit->keySequence(),QKeySequence("Ctrl+S"));
        page->findChild<QComboBox*>("shortcutSnapshotControl")->setCurrentText("Recording");QVERIFY(!edit->property("figmaShortcutRecording").toBool());QCOMPARE(edit->keySequence(),QKeySequence("Ctrl+S"));
        page->findChild<QCheckBox*>("shortcutShowClearControl")->setChecked(false);QVERIFY(!edit->findChild<QAbstractButton*>("figmaShortcutClear")->isVisible());
        page->findChild<QCheckBox*>("shortcutShowHintControl")->setChecked(false);QVERIFY(!edit->findChild<QWidget*>("figmaShortcutHint")->isVisible());
        page->findChild<QComboBox*>("componentVariantControl")->setCurrentIndex(5);QCOMPARE(edit->keySequence(),QKeySequence("Ctrl+S"));
        QTest::qWait(60);auto* settings=page->findChild<QScrollArea*>("componentSettingsScroll");QVERIFY(settings);QCOMPARE(settings->horizontalScrollBar()->maximum(),0);
    }
    void lineEditGalleryRetainsIndependentNativeContract() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)=="1116:2")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();
        auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);QCOMPARE(view->property("variantSpecimenCount").toInt(),18);
        QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
        QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(2264,377));const auto specimens=canvas->findChildren<QLineEdit*>();QCOMPARE(specimens.size(),18);
        QSet<QString> ids;int filled=0,disabled=0,large=0;
        for(auto* edit:specimens){ids.insert(edit->property("figmaNodeId").toString());QCOMPARE(edit->geometry(),edit->property("figmaDesignBounds").toRect());
            QCOMPARE(edit->property("designSystemClass").toString(),QString("commonWidget::Z_LineEdit"));QCOMPARE(edit->placeholderText(),QString("Input text"));
            QVERIFY(edit->testAttribute(Qt::WA_TransparentForMouseEvents));QCOMPARE(edit->focusPolicy(),Qt::NoFocus);filled+=!edit->text().isEmpty();disabled+=!edit->isEnabled();large+=edit->height()==40;}
        QCOMPARE(ids.size(),18);QCOMPARE(filled,3);QCOMPARE(disabled,3);QCOMPARE(large,6);
        auto* edit=page->findChild<QLineEdit*>("componentInteractivePreview");QVERIFY(edit);QVERIFY(edit->text().isEmpty());
        page->findChild<QLineEdit*>("componentTextControl")->setText("LOT-73");QCOMPARE(edit->text(),QString("LOT-73"));edit->setSelection(0,3);
        page->findChild<QComboBox*>("lineSizeControl")->setCurrentText("Large");QCOMPARE(edit->height(),40);QCOMPARE(edit->selectedText(),QString("LOT"));
        page->findChild<QComboBox*>("lineSnapshotControl")->setCurrentText("Error");QVERIFY(edit->hasAcceptableInput());QCOMPARE(edit->text(),QString("LOT-73"));
        page->findChild<QCheckBox*>("linePrefixControl")->setChecked(true);page->findChild<QCheckBox*>("lineSuffixControl")->setChecked(true);QVERIFY(edit->findChild<QAction*>("figmaLineEditPrefixAction"));
        page->findChild<QCheckBox*>("lineReadOnlyControl")->setChecked(true);page->findChild<QComboBox*>("lineEchoControl")->setCurrentIndex(2);QCOMPARE(edit->echoMode(),QLineEdit::Password);
        page->findChild<QComboBox*>("componentVariantControl")->setCurrentIndex(6);QCoreApplication::processEvents();QVERIFY(edit->isReadOnly());QCOMPARE(edit->text(),QString("LOT-73"));QCOMPARE(edit->height(),32);
        page->findChild<QComboBox*>("lineValidatorControl")->setCurrentIndex(1);QVERIFY(!edit->hasAcceptableInput());page->findChild<QLineEdit*>("componentTextControl")->setText("123");QVERIFY(edit->hasAcceptableInput());
        page->findChild<QCheckBox*>("lineRtlControl")->setChecked(true);QCOMPARE(edit->layoutDirection(),Qt::RightToLeft);
        QTest::qWait(60);auto* settings=page->findChild<QScrollArea*>("componentSettingsScroll");QVERIFY(settings);QCOMPARE(settings->horizontalScrollBar()->maximum(),0);
    }
    void dialGalleryAndEditorsKeepGeometryAuthoritiesSeparate() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)=="475:334")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();
        auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);QCOMPARE(view->property("variantSpecimenCount").toInt(),8);
        QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
        QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(1312,543));const auto specimens=canvas->findChildren<QDial*>();QCOMPARE(specimens.size(),8);
        QSet<QString> ids;int wrappingCount=0,disabled=0;
        for(auto* dial:specimens){ids.insert(dial->property("figmaNodeId").toString());QCOMPARE(dial->geometry(),dial->property("figmaDesignBounds").toRect());
            QCOMPARE(dial->property("designSystemClass").toString(),QString("commonWidget::Z_Dial"));QVERIFY(dial->property("figmaDialSnapshotGeometry").toBool());
            QVERIFY(!dial->isSliderDown());QVERIFY(dial->testAttribute(Qt::WA_TransparentForMouseEvents));QCOMPARE(dial->focusPolicy(),Qt::NoFocus);
            wrappingCount+=dial->wrapping();disabled+=!dial->isEnabled();}
        QCOMPARE(ids.size(),8);QCOMPARE(wrappingCount,4);QCOMPARE(disabled,2);
        auto* dial=page->findChild<QDial*>("componentInteractivePreview");QVERIFY(dial);QVERIFY(!dial->property("figmaDialSnapshotGeometry").toBool());
        page->findChild<QSpinBox*>("dialValueControl")->setValue(73);QCOMPARE(dial->value(),73);
        page->findChild<QCheckBox*>("dialWrappingControl")->setChecked(true);QVERIFY(dial->wrapping());QCOMPARE(dial->value(),73);
        page->findChild<QCheckBox*>("dialNotchesControl")->setChecked(false);QVERIFY(!dial->notchesVisible());
        page->findChild<QDoubleSpinBox*>("dialNotchTargetControl")->setValue(8.5);QCOMPARE(dial->notchTarget(),8.5);
        page->findChild<QComboBox*>("dialSnapshotControl")->setCurrentText("Dragging");QVERIFY(!dial->isSliderDown());
        page->findChild<QCheckBox*>("dialAutomaticLabelControl")->setChecked(false);page->findChild<QLineEdit*>("dialLabelControl")->setText("LOT-40");QCOMPARE(dial->value(),73);
        page->findChild<QCheckBox*>("dialTrackingControl")->setChecked(false);page->findChild<QSpinBox*>("dialPositionControl")->setValue(80);QCOMPARE(dial->value(),73);QCOMPARE(dial->sliderPosition(),80);
        page->findChild<QComboBox*>("componentVariantControl")->setCurrentIndex(7);QCoreApplication::processEvents();QVERIFY(!dial->isEnabled());QCOMPARE(dial->value(),80);QVERIFY(!dial->notchesVisible());
        page->findChild<QComboBox*>("componentVariantControl")->setCurrentIndex(0);QCoreApplication::processEvents();QVERIFY(!dial->wrapping());
        page->findChild<QSpinBox*>("dialMaximumControl")->setValue(0);QCOMPARE(dial->minimum(),dial->maximum());
        page->findChild<QCheckBox*>("dialWrappingControl")->setChecked(true);QCOMPARE(dial->maximum()-dial->minimum(),1);
        QTest::qWait(60);auto* settings=page->findChild<QScrollArea*>("componentSettingsScroll");QVERIFY(settings);QCOMPARE(settings->horizontalScrollBar()->maximum(),0);
    }
    void scrollBarGalleryAndEditorsRetainNativeAuthority() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)=="471:334")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();
        auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);QCOMPARE(view->property("variantSpecimenCount").toInt(),8);
        QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
        QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(1312,543));const auto specimens=canvas->findChildren<QScrollBar*>();QCOMPARE(specimens.size(),8);
        QSet<QString> ids;int vertical=0,disabled=0;
        for(auto* bar:specimens){ids.insert(bar->property("figmaNodeId").toString());QCOMPARE(bar->geometry(),bar->property("figmaDesignBounds").toRect());
            QCOMPARE(bar->property("designSystemClass").toString(),QString("commonWidget::Z_ScrollBar"));QVERIFY(bar->property("figmaScrollBarSnapshotGeometry").toBool());
            QVERIFY(!bar->isSliderDown());QVERIFY(bar->testAttribute(Qt::WA_TransparentForMouseEvents));QCOMPARE(bar->focusPolicy(),Qt::NoFocus);
            vertical+=bar->orientation()==Qt::Vertical;disabled+=!bar->isEnabled();}
        QCOMPARE(ids.size(),8);QCOMPARE(vertical,4);QCOMPARE(disabled,2);
        auto* preview=page->findChild<QScrollBar*>("componentInteractivePreview");QVERIFY(preview);QCOMPARE(preview->focusPolicy(),Qt::NoFocus);
        QVERIFY(!preview->property("figmaScrollBarSnapshotGeometry").toBool());
        page->findChild<QSpinBox*>("scrollValueControl")->setValue(73);page->findChild<QSpinBox*>("scrollPageStepControl")->setValue(35);QCOMPARE(preview->value(),73);QCOMPARE(preview->pageStep(),35);
        page->findChild<QCheckBox*>("scrollShowLinesControl")->setChecked(false);QVERIFY(!preview->property("figmaScrollBarShowLineButtons").toBool());QCOMPARE(preview->value(),73);
        page->findChild<QComboBox*>("scrollSnapshotControl")->setCurrentText("Dragging");QVERIFY(!preview->isSliderDown());
        QCOMPARE(preview->property("figmaScrollBarSnapshotState").toString(),QString("Dragging"));
        page->findChild<QComboBox*>("scrollOrientationControl")->setCurrentIndex(1);QCOMPARE(preview->orientation(),Qt::Vertical);QCOMPARE(preview->width(),100);
        page->findChild<QCheckBox*>("scrollTrackingControl")->setChecked(false);page->findChild<QSpinBox*>("scrollPositionControl")->setValue(80);
        QCOMPARE(preview->value(),73);QCOMPARE(preview->sliderPosition(),80);
        page->findChild<QCheckBox*>("scrollKeyboardControl")->setChecked(true);QCOMPARE(preview->focusPolicy(),Qt::StrongFocus);
        page->findChild<QCheckBox*>("scrollInvertedAppearanceControl")->setChecked(true);QVERIFY(preview->invertedAppearance());
        page->findChild<QCheckBox*>("scrollRtlControl")->setChecked(true);QCOMPARE(preview->layoutDirection(),Qt::RightToLeft);
        page->findChild<QComboBox*>("componentVariantControl")->setCurrentIndex(7);QCoreApplication::processEvents();QVERIFY(!preview->isEnabled());
        QCOMPARE(preview->value(),80);QCOMPARE(preview->pageStep(),35);QVERIFY(!preview->property("figmaScrollBarShowLineButtons").toBool());
        QTest::qWait(60);auto* settings=page->findChild<QScrollArea*>("componentSettingsScroll");QVERIFY(settings);QCOMPARE(settings->horizontalScrollBar()->maximum(),0);
    }
    void sliderGalleryAndInteractivePropertiesStayDistinct() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)=="467:2")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();
        auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);QCOMPARE(view->property("variantSpecimenCount").toInt(),8);
        QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
        QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(1312,543));const auto specimens=canvas->findChildren<QSlider*>();QCOMPARE(specimens.size(),8);
        page->findChild<QTabWidget*>("componentSections")->setCurrentIndex(1);QCoreApplication::processEvents();
        const auto keyLabels=canvas->findChild<QWidget*>("variantKey")->findChildren<QLabel*>();QCOMPARE(keyLabels.size(),3);
        for(auto* label:keyLabels){QCOMPARE(label->font().family(),QString("Inter"));QCOMPARE(label->font().weight(),int(QFont::DemiBold));
            QVERIFY(QRawFont::fromFont(label->font()).familyName().startsWith("Inter"));QCOMPARE(QRawFont::fromFont(label->font()).weight(),int(QFont::DemiBold));}
        page->findChild<QTabWidget*>("componentSections")->setCurrentIndex(0);QCoreApplication::processEvents();
        QSet<QString> ids;int vertical=0,disabled=0;
        for(auto* slider:specimens) {
            ids.insert(slider->property("figmaNodeId").toString());QCOMPARE(slider->geometry(),slider->property("figmaDesignBounds").toRect());
            QCOMPARE(slider->property("nativeQtClass").toString(),QString("QSlider"));QVERIFY(slider->property("figmaSliderSnapshotGeometry").toBool());
            QVERIFY(!slider->isSliderDown());QVERIFY(slider->testAttribute(Qt::WA_TransparentForMouseEvents));QCOMPARE(slider->focusPolicy(),Qt::NoFocus);
            disabled+=!slider->isEnabled();vertical+=slider->orientation()==Qt::Vertical;
        }
        QCOMPARE(ids.size(),8);QCOMPARE(vertical,4);QCOMPARE(disabled,2);
        auto* preview=page->findChild<QSlider*>("componentInteractivePreview");QVERIFY(preview);QVERIFY(!preview->property("figmaSliderSnapshotGeometry").toBool());
        page->findChild<QSpinBox*>("sliderValueControl")->setValue(73);QCOMPARE(preview->value(),73);
        page->findChild<QCheckBox*>("sliderAutomaticLabelControl")->setChecked(false);page->findChild<QLineEdit*>("sliderLabelControl")->setText("text only");
        QCOMPARE(preview->value(),73);QCOMPARE(preview->property("figmaSliderValueLabel").toString(),QString("text only"));
        page->findChild<QComboBox*>("sliderSnapshotControl")->setCurrentText("Dragging");QVERIFY(!preview->isSliderDown());
        QCOMPARE(preview->property("figmaSliderSnapshotState").toString(),QString("Dragging"));
        page->findChild<QComboBox*>("sliderOrientationControl")->setCurrentIndex(1);QCOMPARE(preview->orientation(),Qt::Vertical);QCOMPARE(preview->value(),73);
        page->findChild<QCheckBox*>("sliderTrackingControl")->setChecked(false);QVERIFY(!preview->hasTracking());
        page->findChild<QSpinBox*>("sliderPositionControl")->setValue(80);QCOMPARE(preview->sliderPosition(),80);QCOMPARE(preview->value(),73);
        page->findChild<QComboBox*>("sliderTicksControl")->setCurrentIndex(0);QCOMPARE(preview->tickPosition(),QSlider::NoTicks);
        page->findChild<QCheckBox*>("sliderShowValueControl")->setChecked(false);QVERIFY(!preview->property("figmaSliderShowValue").toBool());
        page->findChild<QCheckBox*>("sliderInvertedAppearanceControl")->setChecked(true);QVERIFY(preview->invertedAppearance());
        page->findChild<QCheckBox*>("sliderInvertedControlsControl")->setChecked(true);QVERIFY(preview->invertedControls());
        QCOMPARE(preview->value(),73);QCOMPARE(preview->sliderPosition(),80);
        // Qt 5.15 commits a pending sliderPosition when disabling, even when
        // sliderDown was already false. Compare with an unstyled native slider.
        QSlider native;native.setRange(preview->minimum(),preview->maximum());native.setValue(73);
        native.setTracking(false);native.setSliderPosition(80);QSignalSpy nativeChanges(&native,&QSlider::valueChanged);
        native.setEnabled(false);QCOMPARE(native.value(),80);QCOMPARE(nativeChanges.count(),1);
        QSignalSpy previewChanges(preview,&QSlider::valueChanged);
        page->findChild<QComboBox*>("componentVariantControl")->setCurrentIndex(7);QCoreApplication::processEvents();QVERIFY(!preview->isEnabled());
        QCOMPARE(preview->value(),native.value());QCOMPARE(previewChanges.count(),nativeChanges.count());QVERIFY(!preview->isSliderDown());
        QTest::qWait(60);auto* settings=page->findChild<QScrollArea*>("componentSettingsScroll");QVERIFY(settings);QCOMPARE(settings->horizontalScrollBar()->maximum(),0);
    }
    void dateTimeGalleriesAndEditorsUseSourceVariants() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");
        QSet<QString> allIds;
        for(const auto& pageId:{"463:2","464:2","465:2"}) {
            for(QTreeWidgetItemIterator it(tree);*it;++it)
                if((*it)->data(0,Qt::UserRole)==pageId)tree->setCurrentItem(*it);
            QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();
            const bool timeOnly=QString(pageId)=="465:2",dateOnly=QString(pageId)=="464:2";
            QVERIFY(!page->findChild<QTabWidget*>("componentSetTabs"));
            auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);
            QCOMPARE(view->property("variantSpecimenCount").toInt(),8);QWidget* canvas=nullptr;
            for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
            QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(timeOnly?787:dateOnly?696:817,409));
            const auto specimens=canvas->findChildren<QDateTimeEdit*>();QCOMPARE(specimens.size(),8);
            int disabled=0,readOnly=0,calendars=0;
            for(auto* date:specimens) {
                allIds.insert(date->property("figmaNodeId").toString());
                QCOMPARE(date->geometry(),date->property("figmaDesignBounds").toRect());
                QCOMPARE(date->property("nativeQtClass").toString(),QString(timeOnly?"QTimeEdit":dateOnly?"QDateEdit":"QDateTimeEdit"));
                if(timeOnly)QCOMPARE(date->time(),QTime(date->displayedSections()&QDateTimeEdit::AmPmSection?14:2,30,45));
                else QCOMPARE(date->date(),QDate(2026,10,7));
                QCOMPARE(date->height(),32);QVERIFY(date->findChild<QObject*>("figmaDateBinding"));
                disabled+=!date->isEnabled();readOnly+=date->isReadOnly();calendars+=date->calendarPopup();
            }
            QCOMPARE(disabled,2);QCOMPARE(readOnly,2);QCOMPARE(calendars,timeOnly?0:4);
            auto* preview=page->findChild<QDateTimeEdit*>("componentInteractivePreview");QVERIFY(preview);
            auto* editor=preview->findChild<QLineEdit*>();QVERIFY(editor);
            auto* value=page->findChild<QDateTimeEdit*>("dateValueControl");QVERIFY(value);
            if(timeOnly)value->setTime(QTime(14,42,37));else value->setDateTime(QDateTime(QDate(2026,10,9),QTime(14,42,37)));
            const auto original=preview->dateTime();QSignalSpy changes(preview,&QDateTimeEdit::dateTimeChanged);
            if(timeOnly) {
                page->findChild<QComboBox*>("dateFormatControl")->setCurrentText("12h");
                page->findChild<QCheckBox*>("dateShowSecondsControl")->setChecked(false);
                QCOMPARE(preview->displayFormat(),QString("hh:mm AP"));
                QCOMPARE(preview->time(),QTime(14,42,37));
            } else {
                page->findChild<QCheckBox*>("dateCalendarControl")->setChecked(false);QVERIFY(!preview->calendarPopup());
                if(!dateOnly) {
                    page->findChild<QCheckBox*>("dateShowTimeControl")->setChecked(false);
                    QCOMPARE(preview->displayFormat(),QString("yyyy-MM-dd"));
                    QVERIFY(preview->property("figmaDatePreviewTimeSpec").toString().isEmpty());
                    page->findChild<QCheckBox*>("dateShowTimeControl")->setChecked(true);
                    QCOMPARE(preview->property("figmaDatePreviewTimeSpec").toString(),QString(" CST"));
                }
            }
            QCOMPARE(preview->dateTime(),original);QCOMPARE(changes.count(),0);QCOMPARE(preview->findChild<QLineEdit*>(),editor);
            auto* minimum=page->findChild<QDateTimeEdit*>("dateMinimumControl");
            auto* maximum=page->findChild<QDateTimeEdit*>("dateMaximumControl");
            const auto priorMin=preview->minimumDateTime(),priorMax=preview->maximumDateTime();
            if(timeOnly){minimum->setTime(QTime(18,0));maximum->setTime(QTime(8,0));}
            else {minimum->setDate(QDate(2027,1,1));maximum->setDate(QDate(2026,1,1));}
            QMetaObject::invokeMethod(minimum,"editingFinished");
            QCOMPARE(preview->minimumDateTime(),priorMin);QCOMPARE(preview->maximumDateTime(),priorMax);
            QVERIFY(!page->findChild<QLabel*>("dateRangeNotice")->text().isEmpty());
            page->findChild<QCheckBox*>("dateReadOnlyControl")->setChecked(true);QVERIFY(preview->isReadOnly());
            QTest::keyClick(preview,Qt::Key_Up);QCOMPARE(preview->dateTime(),original);
            page->findChild<QCheckBox*>("dateSnapshotControl")->setChecked(true);
            QCOMPARE(preview->property("figmaDateSnapshotState").toString(),QString("Focus"));
            auto* variant=page->findChild<QComboBox*>("componentVariantControl");variant->setCurrentIndex(7);
            QCoreApplication::processEvents();QVERIFY(!preview->isEnabled());QCOMPARE(preview->dateTime(),original);
            QTest::qWait(60);auto* settings=page->findChild<QScrollArea*>("componentSettingsScroll");QVERIFY(settings);
            QCOMPARE(settings->horizontalScrollBar()->maximum(),0);
        }
        QCOMPARE(allIds.size(),24);
    }
    void numericGalleriesAndEditorsUseSourceVariants() {
        window->resize(1600,920);
        auto* tree=part<QTreeWidget>("componentTree");
        for(const auto& pageId:{"458:2","460:2"}) {
            for(QTreeWidgetItemIterator it(tree);*it;++it)
                if((*it)->data(0,Qt::UserRole)==pageId)tree->setCurrentItem(*it);
            QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();
            QVERIFY(!page->findChild<QTabWidget*>("componentSetTabs"));
            auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);
            QCOMPARE(view->property("variantSpecimenCount").toInt(),16);
            QWidget* canvas=nullptr;
            for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
            QVERIFY(canvas);const bool decimal=QString(pageId)=="460:2";
            QCOMPARE(canvas->size(),QSize(decimal?1232:1168,409));
            const auto specimens=canvas->findChildren<QAbstractSpinBox*>();QCOMPARE(specimens.size(),16);
            QSet<QString> ids;int disabled=0,readOnly=0,split=0,none=0;
            for(auto* spin:specimens) {
                ids.insert(spin->property("figmaNodeId").toString());
                QCOMPARE(spin->geometry(),spin->property("figmaDesignBounds").toRect());
                QCOMPARE(spin->property("nativeQtClass").toString(),QString(decimal?"QDoubleSpinBox":"QSpinBox"));
                QCOMPARE(spin->property("value").toDouble(),decimal?72.5:128.0);
                QCOMPARE(spin->height(),32);QVERIFY(spin->findChild<QObject*>("figmaSpinBinding"));
                disabled+=!spin->isEnabled();readOnly+=spin->isReadOnly();
                split+=spin->property("figmaSpinControls")=="SplitPlusMinus";none+=spin->buttonSymbols()==QAbstractSpinBox::NoButtons;
            }
            QCOMPARE(ids.size(),16);QCOMPARE(disabled,4);QCOMPARE(readOnly,4);QCOMPARE(split,4);QCOMPARE(none,4);
            auto* preview=page->findChild<QAbstractSpinBox*>("componentInteractivePreview");QVERIFY(preview);
            auto* editor=preview->findChild<QLineEdit*>();QVERIFY(editor);
            auto* inspector=page->findChild<QTreeWidget*>("componentObjectTree");QVERIFY(inspector);
            const auto inspected=[&](const QString& name) {
                const auto* root=inspector->topLevelItem(0);
                for(int i=0;i<root->childCount();++i)
                    if(root->child(i)->text(0)==name)return root->child(i)->text(1);
                return QString();
            };
            QCOMPARE(inspected("visible"),QString("true"));
            const auto geometry=preview->geometry();
            QCOMPARE(inspected("geometry"),QString("%1,%2 / %3x%4").arg(geometry.x()).arg(geometry.y()).arg(geometry.width()).arg(geometry.height()));
            QFont numericFont;numericFont.setFamilies({"Inter","Noto Sans SC"});numericFont.setPixelSize(14);numericFont.setWeight(QFont::Medium);
            const auto raw=QRawFont::fromFont(numericFont);
            QCOMPARE(QFontInfo(editor->font()).family(),QString("Inter"));
            QCOMPARE(raw.weight(),int(QFont::Medium));
            for(auto* label:canvas->findChildren<QLabel*>()) {
                QCOMPARE(QFontInfo(label->font()).family(),QString("Inter"));
                QCOMPARE(QRawFont::fromFont(label->font()).weight(),int(QFont::DemiBold));
            }
            auto* settings=page->findChild<QScrollArea*>("componentSettingsScroll");QVERIFY(settings);
            QTest::qWait(60);
            // The narrow property column must not require horizontal navigation.
            QCOMPARE(settings->horizontalScrollBar()->maximum(),0);
            page->findChild<QDoubleSpinBox*>("spinValueControl")->setValue(42);QCOMPARE(preview->property("value").toDouble(),42.0);
            page->findChild<QDoubleSpinBox*>("spinStepControl")->setValue(decimal?.25:2);
            page->findChild<QComboBox*>("spinControlsControl")->setCurrentText("SplitPlusMinus");
            QCOMPARE(preview->property("figmaSpinControls").toString(),QString("SplitPlusMinus"));
            QCOMPARE(preview->findChild<QLineEdit*>(),editor);QCOMPARE(preview->property("value").toDouble(),42.0);
            page->findChild<QCheckBox*>("spinShowPrefixControl")->setChecked(false);QCOMPARE(preview->property("prefix").toString(),QString());
            page->findChild<QCheckBox*>("spinReadOnlyControl")->setChecked(true);QVERIFY(preview->isReadOnly());
            page->findChild<QCheckBox*>("spinSnapshotControl")->setChecked(true);
            QCOMPARE(preview->property("figmaSpinSnapshotState").toString(),QString("Focus"));
            if(decimal) {
                page->findChild<QSpinBox*>("spinDecimalsControl")->setValue(3);QCOMPARE(preview->property("decimals").toInt(),3);
                page->findChild<QComboBox*>("spinLocaleControl")->setCurrentText("de_DE");QVERIFY(preview->text().contains("42,000"));
            }
        }
    }
    void inputGalleryUsesAllSevenAuthoredSets() {
        auto* tree=part<QTreeWidget>("componentTree");
        for(QTreeWidgetItemIterator it(tree);*it;++it)
            if((*it)->data(0,Qt::UserRole)=="339:2")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();
        QVERIFY(!page->findChild<QTabWidget*>("componentSetTabs"));
        auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);
        QCOMPARE(view->property("componentSetCount").toInt(),7);QCOMPARE(view->property("variantSpecimenCount").toInt(),108);
        QWidget* canvas=nullptr;
        for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
        QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(2264,1995));
        const auto inputs=canvas->findChildren<Z_InputField*>();QCOMPARE(inputs.size(),96);
        QSet<QString> ids;int disabled=0,anatomy=0;
        for(auto* input:inputs) {
            ids.insert(input->property("figmaNodeId").toString());input->layout()->activate();
            QCOMPARE(input->geometry(),input->property("figmaDesignBounds").toRect());
            QCOMPARE(input->control()->height(),input->height());
            QVERIFY(input->control()->isFigmaInputAppearance());disabled+=!input->isEnabled();
        }
        for(auto* action:canvas->findChildren<field::Action*>())
            if(action->property("internalAnatomy").toBool()) {
                ++anatomy;ids.insert(action->property("figmaNodeId").toString());
                QCOMPARE(action->geometry(),action->property("figmaDesignBounds").toRect());
                QVERIFY(!action->property("publicAdapterImplemented").toBool());
            }
        QCOMPARE(ids.size(),108);QVERIFY(!ids.contains(""));QCOMPARE(anatomy,12);QCOMPARE(disabled,15);
        auto* preview=page->findChild<Z_InputField*>();QVERIFY(preview);
        auto* kind=page->findChild<QComboBox*>("inputKindControl");QVERIFY(kind);
        kind->setCurrentText("Password");page->findChild<QCheckBox*>("inputPasswordVisibleControl")->setChecked(true);
        QCOMPARE(preview->lineEdit()->echoMode(),QLineEdit::Normal);
        const auto value=preview->value();auto* line=preview->lineEdit();
        page->findChild<QComboBox*>("inputSnapshotStateControl")->setCurrentText("Focus");
        QCOMPARE(preview->value(),value);QCOMPARE(preview->lineEdit(),line);
        page->findChild<QComboBox*>("inputSnapshotStateControl")->setCurrentIndex(0);kind->setCurrentIndex(0);
    }
    void nativeFigmaAppearanceRemainsEditable() {
        auto* tree=part<QTreeWidget>("componentTree");
        auto* stack=part<QStackedWidget>("componentPages");
        auto selectPage=[&](const QString& id) {
            for(QTreeWidgetItemIterator it(tree);*it;++it)
                if((*it)->data(0,Qt::UserRole).toString()==id){tree->setCurrentItem(*it);return;}
        };
        selectPage("414:7");QCoreApplication::processEvents();
        auto* page=stack->currentWidget();
        auto* editors=page->findChild<QStackedWidget*>("appearanceComponentSetEditors");
        QVERIFY(editors);
        auto* editor=editors->currentWidget();
        auto* label=editor->findChild<QLabel*>("componentInteractivePreview");
        auto* content=editor->findChild<QLineEdit*>("componentTextControl");
        auto* enabled=editor->findChild<QCheckBox*>("componentEnabledControl");
        auto* variant=editor->findChild<QComboBox*>("componentVariantControl");
        auto* inspector=editor->findChild<QTreeWidget*>("componentObjectTree");
        QVERIFY(label&&content&&enabled&&variant&&inspector);
        QVERIFY(content->isEnabled());content->setText("Edited label");QCOMPARE(label->text(),QString("Edited label"));
        enabled->setChecked(false);QVERIFY(!label->isEnabled());enabled->setChecked(true);
        QVERIFY(variant->count()>1);variant->setCurrentIndex(1);
        QCOMPARE(label->alignment(),Qt::Alignment(Qt::AlignHCenter|Qt::AlignVCenter|Qt::AlignAbsolute));
        variant->setCurrentIndex(5);QVERIFY(label->wordWrap());
        QCOMPARE(label->alignment(),Qt::Alignment(Qt::AlignTrailing|Qt::AlignVCenter|Qt::AlignAbsolute));
        QCOMPARE(label->text(),QString("Edited label"));
        QCOMPARE(label->property("figmaVariant").toString(),variant->currentText());
        QTest::qWait(450);QVERIFY(inspector->topLevelItemCount()>0);
        selectPage("511:11");QCoreApplication::processEvents();page=stack->currentWidget();
        auto* sets=page->findChild<QComboBox*>("appearanceComponentSetControl");
        editors=page->findChild<QStackedWidget*>("appearanceComponentSetEditors");
        QVERIFY(sets&&editors);QCOMPARE(sets->count(),2);
        sets->setCurrentIndex(0); // Establish this test's target; prior editor tests may select Area.
        QCOMPARE(editors->currentWidget()->findChild<QWidget*>("componentInteractivePreview")->property("nativeQtClass").toString(),QString("QMdiSubWindow"));
        sets->setCurrentIndex(1);
        QCOMPARE(editors->currentWidget()->findChild<QWidget*>("componentInteractivePreview")->property("nativeQtClass").toString(),QString("QMdiArea"));
    }
    void appearanceEditorsRemainReachableFromFigmaNavigation() {
        auto* tree = part<QTreeWidget>("componentTree");
        auto* stack = part<QStackedWidget>("componentPages");
        const QList<QPair<QString, QString>> expected{
            {"172:2710", "buttonAppearanceTab"}, {"234:2717", "checkboxAppearanceTab"},
            {"339:2", "inputAppearanceTab"}, {"242:2720", "selectAppearanceTab"}};
        for (const auto& entry : expected) {
            QTreeWidgetItem* selected = pageItem(tree, entry.first);
            QVERIFY(selected);
            tree->setCurrentItem(selected);
            QCoreApplication::processEvents();
            auto* editor = stack->currentWidget()->findChild<QWidget*>(entry.second);
            QVERIFY2(editor, qPrintable("Appearance controls disconnected: " + entry.second));
            QVERIFY(editor->isVisibleTo(stack->currentWidget()));
            QVERIFY(editor->findChildren<QComboBox*>().size() >= 2);
            QVERIFY(!editor->findChildren<QTreeWidget*>().isEmpty());
        }
    }
    void navigationEditorsDriveNativeActions() {
        std::unique_ptr<QWidget> menuPage(native_gallery::createNavigationAppearance("482:335", nullptr));
        menuPage->show();
        auto* menu = menuPage->findChild<QMenu*>("navigationInteractivePreview");
        auto* item = menuPage->findChild<QComboBox*>("navigationItem");
        auto* state = menuPage->findChild<QComboBox*>("navigationState");
        QVERIFY(menu && item && state);
        QCoreApplication::processEvents();
        QCOMPARE(menu->actionGeometry(menu->actions().first()), QRect(6, 6, 260, 40));
        item->setCurrentIndex(2); // Auto save: QAction owns checked data.
        auto* action = menu->actions()[3];
        QVERIFY(action->isChecked());
        state->setCurrentText("Disabled");
        QVERIFY(!action->isEnabled());
        state->setCurrentIndex(0);
        QVERIFY(action->isEnabled());
        QSignalSpy triggered(menu, &QMenu::triggered);
        QTest::mouseClick(menu, Qt::LeftButton, Qt::NoModifier, menu->actionGeometry(action).center());
        QCOMPARE(triggered.count(), 1);
        QVERIFY(!action->isChecked());
        QCoreApplication::processEvents();
        QVERIFY(menu->isVisible());
        QVERIFY(menuPage->findChild<QLabel*>("navigationEventLog")->text().contains("checked=false"));

        std::unique_ptr<QWidget> barPage(native_gallery::createNavigationAppearance("486:10", nullptr));
        barPage->show();
        auto* bar = barPage->findChild<QMenuBar*>("navigationInteractivePreview");
        QVERIFY(bar);
        QCoreApplication::processEvents();
        auto* first = bar->actions().first();
        QTest::mouseClick(bar, Qt::LeftButton, Qt::NoModifier, bar->actionGeometry(first).center());
        QTRY_VERIFY(first->menu()->isVisible());
        QTest::keyClick(first->menu(), Qt::Key_Escape);

        std::unique_ptr<QWidget> toolbarPage(native_gallery::createNavigationAppearance("492:10", nullptr));
        auto* toolbar = toolbarPage->findChild<QToolBar*>("navigationInteractivePreview");
        QVERIFY(toolbar);
        toolbarPage->findChild<QComboBox*>("navigationOrientation")->setCurrentText("Vertical");
        toolbarPage->findChild<QComboBox*>("navigationPresentation")->setCurrentText("TextBesideIcon");
        toolbarPage->findChild<QComboBox*>("navigationCapacity")->setCurrentText("Overflow");
        QCOMPARE(toolbar->orientation(), Qt::Vertical);
        QCOMPARE(toolbar->size(), QSize(120, 220));
        auto* overflow = toolbar->findChild<QToolButton*>("figmaOverflowButton");
        QVERIFY(overflow && overflow->menu());
    }
    void navigationGalleryRetainsFigmaGeometryAndNativeSpecimens() {
        const QList<QPair<QString, int>> pages{{"482:335", 20}, {"486:10", 5}, {"492:10", 16}};
        for (const auto& entry : pages) {
            std::unique_ptr<QWidget> page(native_gallery::createNavigationGallery(entry.first, nullptr));
            auto* view = qobject_cast<QGraphicsView*>(page.get());
            QVERIFY(view);
            QWidget* canvas = nullptr;
            for (auto* item : view->scene()->items()) {
                auto* proxy = qgraphicsitem_cast<QGraphicsProxyWidget*>(item);
                if (proxy && proxy->widget()->objectName() == "figmaGalleryCanvas") canvas = proxy->widget();
            }
            QVERIFY(canvas);
            QCOMPARE(canvas->width(), 1312);
            QList<QWidget*> specimens;
            for (auto* widget : canvas->findChildren<QWidget*>())
                if (widget->property("figmaVariant").isValid()) specimens.append(widget);
            QCOMPARE(specimens.size(), entry.second);
            QCOMPARE(specimens.first()->pos(), QPoint(52, 109));
            if (entry.first == "482:335") {
                for (auto* widget : specimens) {
                    auto* menu = qobject_cast<QMenu*>(widget);
                    QVERIFY(menu);
                    QCOMPARE(menu->size(), QSize(260, 40));
                    QCOMPARE(menu->actionGeometry(menu->actions().first()), QRect(0, 0, 260, 40));
                }
            }
        }
    }
    void cleanup() {
        FocusIndicator::instance().setMode(FocusIndicator::Mode::Keyboard);
        window->activateWindow();
    }
    void focusOptionAndTokenDefaults() {
        auto* choice = part<QComboBox>("focusIndicatorCombo");
        QVERIFY(choice);
        QCOMPARE(choice->count(), 3);
        QCOMPARE(choice->currentData().toString(), QString("keyboard"));
        QCOMPARE(FocusIndicator::instance().opacity(), 0.55);
        for (const auto& mode : {"off", "always", "keyboard"}) {
            choice->setCurrentIndex(choice->findData(mode));
            QCOMPARE(FocusIndicator::modeName(FocusIndicator::instance().mode()), QString(mode));
        }
        QVERIFY_EXCEPTION_THROWN(FocusIndicator::parseMode("unknown"), std::invalid_argument);
    }
    void mouseFocusDoesNotOverrideDangerBorder() {
        auto* tree = part<QTreeWidget>("componentTree");
        for (QTreeWidgetItemIterator it(tree); *it; ++it)
            if ((*it)->data(0, Qt::UserRole).toString() == "172:2710")
                tree->setCurrentItem(*it);
        QCoreApplication::processEvents();
        auto* b = part<Z_PushButton>("BUTTON_ROOT");
        part<Z_CheckBox>("buttonDanger")->setChecked(true);
        window->activateWindow();
        QTest::mouseClick(b, Qt::LeftButton);
        QTRY_VERIFY(b->hasFocus());
        QTest::qWait(30);
        const auto qss = b->styleSheet();
        QVERIFY(!b->property("focusIndicatorVisible").toBool());
        part<Z_CheckBox>("buttonDanger")->setChecked(false);
        QVERIFY2(!qss.contains(":focus"), qPrintable(qss));
    }
    void focusModeSwitchPreservesKeyboardAndGeometry() {
        QWidget host;
        auto* layout = new QVBoxLayout(&host);
        auto* first = new Z_PushButton;
        auto* second = new Z_PushButton;
        first->setText("First"); second->setText("Second");
        layout->addWidget(first); layout->addWidget(second);
        QWidget::setTabOrder(first, second);
        host.show(); host.activateWindow();
        QTest::mouseClick(first, Qt::LeftButton);
        QTRY_VERIFY(first->hasFocus());
        QVERIFY(!first->property("focusIndicatorVisible").toBool());
        QTest::keyClick(first, Qt::Key_Tab);
        QTRY_VERIFY(second->hasFocus());
        QVERIFY(second->property("focusIndicatorVisible").toBool());
        const auto before = second->size();
        const auto withFocus = second->grab().toImage();
        FocusIndicator::instance().setMode(FocusIndicator::Mode::Off);
        QVERIFY(second->hasFocus());
        QVERIFY(!second->property("focusIndicatorVisible").toBool());
        QCOMPARE(second->size(), before);
        QVERIFY(second->grab().toImage() != withFocus);
        QSignalSpy clicks(second, &QPushButton::clicked);
        QTest::keyClick(second, Qt::Key_Space);
        QCOMPARE(clicks.count(), 1);
        QVERIFY(!second->property("focusIndicatorVisible").toBool());
        QTest::keyClick(second, Qt::Key_Backtab);
        QTRY_VERIFY(first->hasFocus());
        QVERIFY(!first->property("focusIndicatorVisible").toBool());
        FocusIndicator::instance().setMode(FocusIndicator::Mode::Always);
        QVERIFY(first->property("focusIndicatorVisible").toBool());
        QTest::mouseClick(second, Qt::LeftButton);
        QTRY_VERIFY(second->property("focusIndicatorVisible").toBool());
        FocusIndicator::instance().setMode(FocusIndicator::Mode::Keyboard);
        QVERIFY(!second->property("focusIndicatorVisible").toBool());
        // A click on an already-focused widget must also hide a keyboard-only indicator.
        QTest::keyClick(second, Qt::Key_Space);
        QVERIFY(second->property("focusIndicatorVisible").toBool());
        QTest::mouseClick(second, Qt::LeftButton);
        QVERIFY(!second->property("focusIndicatorVisible").toBool());
    }
    void switchAndSegmentedControlKeepIntrinsicGalleryGeometry() {
        QWidget host;
        host.resize(900, 180);
        auto* layout = new QHBoxLayout(&host);
        auto* toggle = new Z_Switch(&host);
        QCOMPARE(toggle->size(), QSize(180, 32));
        QCOMPARE(toggle->label(), QStringLiteral("Switch label"));
        QVERIFY(toggle->showLabel());
        QCOMPARE(toggle->trackGeometry(), QRect(2, 6, 36, 20));
        QCOMPARE(toggle->sizePolicy().horizontalPolicy(), QSizePolicy::Fixed);
        QCOMPARE(toggle->sizePolicy().verticalPolicy(), QSizePolicy::Fixed);
        layout->addWidget(toggle);
        host.show();
        QCoreApplication::processEvents();
        QCOMPARE(toggle->size(), QSize(180, 32));
        toggle->resize(640, 96);
        QCOMPARE(toggle->size(), QSize(180, 32));
        toggle->setVariantTuple("Size=Small, Value=On, State=Default");
        QCOMPARE(toggle->size(), QSize(180, 32));
        QCOMPARE(toggle->trackGeometry(), QRect(2, 8, 28, 16));
        toggle->setShowLabel(false);
        QCOMPARE(toggle->size(), QSize(32, 32));
        QCOMPARE(toggle->trackGeometry(), QRect(2, 8, 28, 16));
        toggle->setLabel(QStringLiteral("Immediate setting"));
        QCOMPARE(toggle->label(), QStringLiteral("Immediate setting"));
        toggle->setShowLabel(true);
        QCOMPARE(toggle->size(), QSize(180, 32));

        auto* tree = part<QTreeWidget>("componentTree");
        tree->setCurrentItem(pageItem(tree, QStringLiteral("346:2")));
        QCoreApplication::processEvents();
        auto* switchPage = part<QStackedWidget>("componentPages")->currentWidget();
        switchPage->findChild<QTabWidget*>("componentSections")->setCurrentIndex(1);
        QCoreApplication::processEvents();
        for (auto* specimen : switchPage->findChildren<Z_Switch*>()) {
            QCOMPARE(specimen->size(), specimen->sizeHint());
            QCOMPARE(specimen->size(), QSize(180, 32));
            QCOMPARE(specimen->label(), QStringLiteral("Switch label"));
            QVERIFY(specimen->showLabel());
            QCOMPARE(specimen->trackGeometry().size(),
                     specimen->variantProperties().value("Size") == "Small"
                         ? QSize(28, 16) : QSize(36, 20));
        }

        tree->setCurrentItem(pageItem(tree, QStringLiteral("2148:1146")));
        QCoreApplication::processEvents();
        auto* segmentedPage = part<QStackedWidget>("componentPages")->currentWidget();
        segmentedPage->findChild<QTabWidget*>("componentSections")->setCurrentIndex(1);
        QCoreApplication::processEvents();
        const auto segmentedSpecimens = segmentedPage->findChildren<Z_SegmentedControl*>();
        QVERIFY(!segmentedSpecimens.isEmpty());
        for (auto* specimen : segmentedSpecimens)
            QCOMPARE(specimen->size(), specimen->sizeHint());
    }
    void selectMatricesPreserveCapturedFieldWidth() {
        QWidget* canvas = nullptr;
        try {
            canvas = variantCanvas(QStringLiteral("242:2720"));
        } catch (const std::exception& error) {
            QFAIL(error.what());
        }
        QVERIFY(canvas);
        const auto fields = canvas->findChildren<Z_Select*>();
        QVERIFY(!fields.isEmpty());
        for (auto* field : fields) {
            QCOMPARE(field->width(), 240);
            QVERIFY(field->height() >= 24);
            QVERIFY(field->height() <= 40);
            QCOMPARE(field->property("runtimeBacked").toBool(), true);
        }
        if (!evidence.isEmpty())
            QVERIFY(fields.first()->grab().save(evidence + "/select-runtime-specimen.png"));
        QCOMPARE(canvas->property("runtimeSpecimenCount").toInt(), fields.size());
    }
    void focusIndicatorCanBeDisabledPerComponentWithoutRemovingFocus() {
        Z_Chart presentationOnly;
        Z_Switch interactiveControl;
        QCOMPARE(presentationOnly.focusPolicy(), Qt::NoFocus);
        QCOMPARE(interactiveControl.focusPolicy(), Qt::StrongFocus);

        QWidget host;
        auto* layout = new QVBoxLayout(&host);
        auto* button = new Z_PushButton;
        button->setText(QStringLiteral("Keyboard action"));
        layout->addWidget(button);
        host.show(); host.activateWindow();
        FocusIndicator::instance().setMode(FocusIndicator::Mode::Always);
        button->setFocus(Qt::TabFocusReason);
        QTRY_VERIFY(button->hasFocus());
        QVERIFY(FocusIndicator::instance().visibleFor(button));

        FocusIndicator::instance().setEnabledFor(button, false);
        QVERIFY(button->hasFocus());
        QVERIFY(!FocusIndicator::instance().enabledFor(button));
        QVERIFY(!FocusIndicator::instance().visibleFor(button));
        QVERIFY(!button->property("focusIndicatorVisible").toBool());
        QSignalSpy clicks(button, &QPushButton::clicked);
        QTest::keyClick(button, Qt::Key_Space);
        QCOMPARE(clicks.count(), 1);

        ComponentFactory factory(profile);
        auto* checkbox = qobject_cast<Z_CheckBox*>(
            factory.create(QStringLiteral("commonWidget::Z_CheckBox"), &host));
        auto* input = qobject_cast<Z_InputField*>(
            factory.create(QStringLiteral("commonWidget::Z_InputField"), &host));
        QVERIFY(checkbox && input && input->lineEdit() && input->control());
        checkbox->setText(QStringLiteral("Keyboard choice"));
        input->setPlaceholder(QStringLiteral("Semantic input"));
        layout->addWidget(checkbox);
        layout->addWidget(input);
        QCoreApplication::processEvents();

        checkbox->setFocus(Qt::TabFocusReason);
        QTRY_VERIFY(checkbox->hasFocus());
        QVERIFY(FocusIndicator::instance().visibleFor(checkbox));
        FocusIndicator::instance().setEnabledFor(checkbox, false);
        QVERIFY(checkbox->hasFocus());
        QVERIFY(!FocusIndicator::instance().visibleFor(checkbox));
        const bool checkedBefore = checkbox->isChecked();
        QTest::keyClick(checkbox, Qt::Key_Space);
        QCOMPARE(checkbox->isChecked(), !checkedBefore);

        input->lineEdit()->setFocus(Qt::TabFocusReason);
        QTRY_VERIFY(input->lineEdit()->hasFocus());
        QTRY_VERIFY(input->control()->property("focusIndicatorVisible").toBool());
        FocusIndicator::instance().setEnabledFor(input, false);
        QVERIFY(input->lineEdit()->hasFocus());
        QVERIFY(!FocusIndicator::instance().enabledFor(input->lineEdit()));
        QTRY_VERIFY(!input->control()->property("focusIndicatorVisible").toBool());
        QTest::keyClicks(input->lineEdit(), QStringLiteral("abc"));
        QCOMPARE(input->value(), QStringLiteral("abc"));

        auto* nativeInput = new Z_LineEdit;
        nativeInput->setPlaceholderText(QStringLiteral("Native input"));
        layout->addWidget(nativeInput);
        nativeInput->setFocus(Qt::TabFocusReason);
        QTRY_VERIFY(nativeInput->hasFocus());
        QVERIFY(nativeInput->property("focusIndicatorVisible").toBool());
        FocusIndicator::instance().setEnabledFor(nativeInput, false);
        QVERIFY(nativeInput->hasFocus());
        QVERIFY(!nativeInput->property("focusIndicatorVisible").toBool());
        QTest::keyClicks(nativeInput, QStringLiteral("abc"));
        QCOMPARE(nativeInput->text(), QStringLiteral("abc"));

        FocusIndicator::instance().setEnabledFor(button, true);
        button->setFocus(Qt::TabFocusReason);
        QTRY_VERIFY(button->hasFocus());
        QVERIFY(FocusIndicator::instance().visibleFor(button));
        // Remove the child override before verifying an inherited container policy.
        button->setProperty("focusIndicatorEnabled", QVariant());
        FocusIndicator::instance().setEnabledFor(&host, false);
        QVERIFY(!FocusIndicator::instance().enabledFor(button));
        QVERIFY(!FocusIndicator::instance().visibleFor(button));
        FocusIndicator::instance().setEnabledFor(&host, true);

        // Qt 5 native opt-out removes focus itself; use this only for controls
        // that must not participate in keyboard navigation.
        button->clearFocus();
        button->setFocusPolicy(Qt::NoFocus);
        QCOMPARE(button->focusPolicy(), Qt::NoFocus);
    }
    void dangerFocusFollowsRoleForEveryMaterial() {
        auto catalog = std::make_shared<surface::Catalog>(profile);
        Z_PushButton button;
        button.setType(ButtonType::Primary);
        button.setDanger(true);
        button.setText("Danger");
        ButtonAppearanceAdapter adapter(&button, catalog);
        button.resize(180, 40);
        button.show(); button.activateWindow();
        FocusIndicator::instance().setMode(FocusIndicator::Mode::Always);
        button.setFocus(Qt::MouseFocusReason);
        QTRY_VERIFY(button.hasFocus());
        const auto color = button.focusIndicatorColor();
        QVERIFY(color.red() > color.blue());
        QVERIFY(qAbs(color.alphaF() - 0.55) < 0.005);
        for (const auto& id : catalog->buttonPresetIds()) {
            adapter.setAppearance(id);
            QTest::qWait(15);
            QVERIFY2(button.styleSheet().contains(color.name(QColor::HexArgb)), qPrintable(id));
            const auto visible = button.grab().toImage();
            FocusIndicator::instance().setMode(FocusIndicator::Mode::Off);
            QTest::qWait(15);
            QVERIFY(!button.styleSheet().contains(":focus"));
            QVERIFY(button.grab().toImage() != visible);
            FocusIndicator::instance().setMode(FocusIndicator::Mode::Always);
        }
        button.setLoading(true);
        QVERIFY(!FocusIndicator::instance().visibleFor(&button));
        button.setLoading(false);
        button.setEnabled(false);
        QVERIFY(!FocusIndicator::instance().visibleFor(&button));
    }
    void composedFieldsAndCheckboxShareFocusPolicy() {
        auto catalog = std::make_shared<surface::Catalog>(profile);
        QWidget host;
        auto* layout = new QVBoxLayout(&host);
        auto* check = new Z_CheckBox(catalog, "Enable");
        auto* input = new Z_InputField(tokens);
        auto* select = new Z_Select(tokens);
        auto* otp = new Z_OtpInput(tokens);
        input->setAllowClear(true); input->setValue("recipe");
        select->setOptions({{"a", "Alpha", {}, false}});
        select->setAllowClear(true); select->setValues({"a"});
        for (QWidget* control : QList<QWidget*>{check, input, select, otp})
            layout->addWidget(control);
        host.resize(480, host.sizeHint().height());
        host.show(); host.activateWindow();
        QList<QWidget*> targets{check, input->lineEdit(),
                               select->findChild<QWidget*>("DISPLAY"), otp->editors().first()};
        QList<QWidget*> surfaces{check, input->control(), select->findChild<field::Chrome*>(),
                                otp->findChild<field::Chrome*>("CELL_0")};
        for (int i = 0; i < targets.size(); ++i) {
            auto* target = targets[i]; auto* frame = surfaces[i];
            QVERIFY(target && frame);
            FocusIndicator::instance().setMode(FocusIndicator::Mode::Always);
            target->setFocus(Qt::MouseFocusReason);
            QTRY_VERIFY(target->hasFocus());
            QVERIFY(frame->property("focusIndicatorVisible").toBool());
            const auto before = frame->size();
            const auto pixels = frame->grab().toImage();
            FocusIndicator::instance().setMode(FocusIndicator::Mode::Off);
            QVERIFY(!frame->property("focusIndicatorVisible").toBool());
            QVERIFY(target->hasFocus());
            QCOMPARE(frame->size(), before);
            QVERIFY(frame->grab().toImage() != pixels);
            if (auto* chrome = qobject_cast<field::Chrome*>(frame))
                QVERIFY(chrome->visualState() != "focus");
        }
        input->lineEdit()->setFocus();
        input->setValue({});
        QTest::keyClicks(input->lineEdit(), "abc");
        QCOMPARE(input->value(), QString("abc"));
        input->setStatus("error");
        QCOMPARE(input->control()->visualState(), QString("error"));
        QVERIFY(input->control()->styleSheet().contains(tokens->colorText("errorBorder")));
        FocusIndicator::instance().setMode(FocusIndicator::Mode::Always);
        QCOMPARE(input->control()->visualState(), QString("error"));
        FocusIndicator::instance().setMode(FocusIndicator::Mode::Off);
        QTest::mouseClick(input->findChild<field::Action*>("CLEAR"), Qt::LeftButton);
        QCOMPARE(input->value(), QString());
        check->setFocus();
        const bool checked = check->isChecked();
        QTest::keyClick(check, Qt::Key_Space);
        QCOMPARE(check->isChecked(), !checked);
    }
    void demoIconsHaveNoSeparateSurface() {
        // This regression explicitly exercises the legacy material/icon editor;
        // the initial Figma specimen intentionally has both icon slots hidden.
        auto* material=part<QComboBox>("buttonAppearance");
        material->setCurrentIndex(material->findData("solid"));
        part<Z_CheckBox>("leadingVisible")->setChecked(true);
        part<Z_CheckBox>("trailingVisible")->setChecked(true);
        QVERIFY(!part<QComboBox>("leadingStyle"));
        QVERIFY(!part<QComboBox>("trailingStyle"));
        auto* b = part<Z_PushButton>("BUTTON_ROOT");
        for (int i = 0; i < material->count(); ++i) {
            material->setCurrentIndex(i);
            for (auto* slot : b->findChildren<QWidget*>("ICON_CONTAINER"))
                QCOMPARE(slot->property("iconContainerStyle").toString(), QString("BARE"));
        }
        material->setCurrentIndex(material->findData("solid"));
    }
    void fieldActionsRemainInsideControl() {
        Z_InputField input(tokens);
        input.setAllowClear(true);
        input.setValue("recipe");
        Z_Select select(tokens);
        select.setOptions({{"a", "Alpha", {}, false}});
        select.setValues({"a"});
        select.setAllowClear(true);
        for (auto* host : QList<QWidget*>{&input, &select}) {
            host->resize(420, host->sizeHint().height());
            host->show();
            QTest::qWait(30);
            auto* clear = host->findChild<field::Action*>("CLEAR");
            auto* control = host->findChild<field::Chrome*>("CONTROL");
            QVERIFY(clear && control && clear->isVisible());
            QVERIFY(clear->height() >= 32);
            const auto top = clear->mapTo(control, QPoint()).y();
            QVERIFY2(top > tokens->metric("borderWidth"), "Clear action touches outer frame");
            QVERIFY(control->height() - top - clear->height() > tokens->metric("borderWidth"));
            clear->setFocus(Qt::TabFocusReason);
            QTest::qWait(20);
            const auto pixels = clear->grab().toImage();
            const auto focus = tokens->color("focusBorder");
            int edgePixels = 0;
            for (int y = pixels.height() / 4; y < pixels.height() * 3 / 4; ++y)
                edgePixels += pixels.pixelColor(0, y) == focus;
            QCOMPARE(edgePixels, 0);
            if (!evidence.isEmpty())
                QVERIFY(host->grab().save(
                    evidence + (host == &input ? "/input-action.png" : "/select-action.png")));
        }
    }
    void invalidResourceDisablesPreview() {
        auto* b = part<Z_PushButton>("BUTTON_ROOT");
        edit("leadingSource", ":/icons/missing.svg");
        const bool invalidEnabled = b->isEnabled();
        const bool hasError = !part<QLabel>("buttonValidation")->text().isEmpty();
        edit("leadingSource", ":/icons/arrow-left.svg");
        QVERIFY(hasError);
        QVERIFY(!invalidEnabled);
        QVERIFY(b->isEnabled());
    }
    void unavailableMaterialIsNotSelectable() {
        auto* type = part<QComboBox>("buttonType");
        auto* material = part<QComboBox>("buttonAppearance");
        material->setCurrentIndex(material->findData("solid"));
        type->setCurrentIndex(3);
        const auto flags = material->model()->flags(
            material->model()->index(material->findData("gradient-inset"), 0));
        type->setCurrentIndex(0);
        QVERIFY(!(flags & Qt::ItemIsEnabled));
    }
    void cascadingConfigurationAndRecovery() {
        auto* b = part<Z_PushButton>("BUTTON_ROOT");
        auto* type = part<QComboBox>("buttonType");
        auto* material = part<QComboBox>("buttonAppearance");
        auto* ghost = part<Z_CheckBox>("buttonGhost");
        material->setCurrentIndex(material->findData("solid"));
        ghost->setChecked(true);
        material->setCurrentIndex(material->findData("solid-inner-shadow"));
        QVERIFY(!ghost->isEnabled() && !ghost->isChecked());
        QVERIFY(!b->property("ghost").toBool());
        part<Z_CheckBox>("buttonDanger")->setChecked(true);
        QVERIFY(!b->findChild<surface::solid_inner_shadow::Layer*>()->isHidden());
        part<Z_CheckBox>("buttonDanger")->setChecked(false);
        material->setCurrentIndex(material->findData("gradient-inset"));
        type->setCurrentIndex(3);
        QCOMPARE(material->currentData().toString(), QString("solid"));
        QVERIFY(!part<QLabel>("buttonConfigurationStatus")->text().isEmpty());
        type->setCurrentIndex(0);
        part<QRadioButton>("buttonIconMode")->setChecked(true);
        QVERIFY(!part<QLineEdit>("trailingSource")->isEnabled());
        QVERIFY(!part<QComboBox>("trailingStyle"));
        QVERIFY(!part<Z_CheckBox>("trailingVisible")->isEnabled());
        part<QComboBox>("buttonIconSide")->setCurrentIndex(1);
        QVERIFY(!b->findChild<QWidget*>("TRAILING")->isHidden());
        QVERIFY(b->findChild<QWidget*>("LEADING")->isHidden());
        QVERIFY(!part<QLineEdit>("leadingSource")->isEnabled());
        QVERIFY(part<QLineEdit>("trailingSource")->isEnabled());
        edit("trailingSource", ":/icons/missing.svg");
        QVERIFY(!b->isEnabled());
        QSignalSpy clicks(b, &QPushButton::clicked);
        b->click();
        QCOMPARE(clicks.count(), 0);
        edit("trailingSource", ":/icons/arrow-right.svg");
        QVERIFY(b->isEnabled());
        part<QRadioButton>("buttonTextMode")->setChecked(true);
        QVERIFY(!b->findChild<QWidget*>("LEADING")->isHidden());
        QVERIFY(!b->findChild<QWidget*>("TRAILING")->isHidden());
        QVERIFY(!part<QComboBox>("buttonIconSide")->isEnabled());
        auto* state = part<QComboBox>("buttonState");
        state->setCurrentIndex(3);
        QVERIFY(b->isDown());
        state->setCurrentIndex(0);
        QVERIFY(!b->isDown());
        state->setCurrentIndex(2);
        part<Z_CheckBox>("buttonDisabled")->setChecked(true);
        QVERIFY(!state->isEnabled());
        QCOMPARE(state->currentIndex(), 0);
        part<Z_CheckBox>("buttonDisabled")->setChecked(false);
        part<Z_CheckBox>("buttonLoading")->setChecked(true);
        QVERIFY(!type->isEnabled() && !material->isEnabled());
        QVERIFY(!state->isEnabled());
        part<Z_CheckBox>("buttonLoading")->setChecked(false);
        QVERIFY(type->isEnabled() && material->isEnabled() && state->isEnabled());
        auto* statesMaterial = part<QComboBox>("buttonStateAppearance");
        statesMaterial->setCurrentIndex(statesMaterial->findData("solid-inner-shadow"));
        QVERIFY(!part<Z_PushButton>("buttonStateExample4")->isEnabled());
        QVERIFY(part<Z_PushButton>("buttonStateExample3")->isEnabled());
        statesMaterial->setCurrentIndex(statesMaterial->findData("solid"));
        QVERIFY(part<Z_PushButton>("buttonStateExample4")->isEnabled());
    }
    void buttonBothSourcesAreEditable() {
        QVERIFY(part<QLineEdit>("leadingSource"));
        QVERIFY(part<QLineEdit>("trailingSource"));
        auto* b = part<Z_PushButton>("BUTTON_ROOT");
        QCOMPARE(b->leadingIconSource(), QString(":/icons/arrow-left.svg"));
        QCOMPARE(b->trailingIconSource(), QString(":/icons/arrow-right.svg"));
        auto* left = b->findChild<QWidget*>("LEADING");
        auto* right = b->findChild<QWidget*>("TRAILING");
        QVERIFY(left && right);
        QVERIFY(!left->isHidden() && !right->isHidden());
        part<Z_CheckBox>("leadingVisible")->setChecked(false);
        QVERIFY(left->isHidden() && !right->isHidden());
        edit("trailingSource", ":/icons/x.svg");
        QCOMPARE(b->trailingIconSource(), QString(":/icons/x.svg"));
        QCOMPARE(b->leadingIconSource(), QString(":/icons/arrow-left.svg"));
        part<Z_CheckBox>("leadingVisible")->setChecked(true);
        QCOMPARE(
            left->findChild<QWidget*>("ICON_CONTAINER")->property("iconContainerStyle").toString(),
            QString("BARE"));
        QCOMPARE(
            right->findChild<QWidget*>("ICON_CONTAINER")->property("iconContainerStyle").toString(),
            QString("BARE"));
        edit("trailingSource", ":/icons/arrow-right.svg");
    }
    void iconOnlyLoadingAndRoundtrip() {
        auto* b = part<Z_PushButton>("BUTTON_ROOT");
        part<QRadioButton>("buttonIconMode")->setChecked(true);
        QVERIFY(b->isIconOnly());
        part<Z_CheckBox>("leadingVisible")->setChecked(false);
        QVERIFY(b->isIconOnly());
        const auto before = b->sizeHint();
        part<Z_CheckBox>("buttonLoading")->setChecked(true);
        QVERIFY(b->property("loading").toBool());
        QVERIFY(!b->isEnabled());
        QCOMPARE(b->sizeHint(), before);
        QSignalSpy clicked(b, &QPushButton::clicked);
        b->click();
        QCOMPARE(clicked.count(), 0);
        QTest::qWait(60);
        QCOMPARE(b->sizeHint(), before);
        part<Z_CheckBox>("buttonLoading")->setChecked(false);
        QVERIFY(b->isEnabled());
        part<QRadioButton>("buttonTextMode")->setChecked(true);
        QVERIFY(!b->isIconOnly());
        QCOMPARE(b->trailingIconSource(), QString(":/icons/arrow-right.svg"));
        part<Z_CheckBox>("leadingVisible")->setChecked(true);
        QVERIFY(!b->findChild<QWidget*>("LEADING")->isHidden());
        QVERIFY(!b->findChild<QWidget*>("TRAILING")->isHidden());
    }
    void referenceAppearanceIsButtonOnly() {
        auto* material = part<QComboBox>("buttonAppearance");
        const auto index = material->findData("solid-inner-shadow");
        QVERIFY(index >= 0);
        QVERIFY(part<QComboBox>("checkboxAppearance")->findData("solid-inner-shadow") < 0);
        QVERIFY(part<QComboBox>("inputAppearance")->findData("solid-inner-shadow") < 0);
        QVERIFY(!window->findChild<QWidget*>("shadowEditor"));
        material->setCurrentIndex(index);
        QTest::qWait(30);
        auto* b = part<Z_PushButton>("BUTTON_ROOT");
        auto* layer = b->findChild<surface::solid_inner_shadow::Layer*>();
        QVERIFY(layer && !layer->isHidden());
        part<Z_CheckBox>("buttonDisabled")->setChecked(true);
        QTest::qWait(30);
        QCOMPARE(layer->state(), QString("disabled"));
        part<Z_CheckBox>("buttonDisabled")->setChecked(false);
        material->setCurrentIndex(material->findData("solid"));
    }
    void restoredButtonPreservesMaterials() {
        auto* b = part<Z_PushButton>("BUTTON_ROOT");
        auto* material = part<QComboBox>("buttonAppearance");
        auto* adapter = b->findChild<ButtonAppearanceAdapter*>();
        QVERIFY(adapter);
        const auto before = b->sizeHint();
        for (auto id : {"gradient-inset", "gradient-elevated", "outline", "solid"}) {
            material->setCurrentIndex(material->findData(id));
            QTest::qWait(30);
            QCOMPARE(b->sizeHint(), before);
            QCOMPARE(b->trailingIconSource(), QString(":/icons/arrow-right.svg"));
            QCOMPARE(b->findChild<QWidget*>("TRAILING")
                         ->findChild<QWidget*>("ICON_CONTAINER")
                         ->property("iconContainerStyle")
                         .toString(),
                     QString("BARE"));
            if (QString(id).startsWith("gradient"))
                QVERIFY(b->styleSheet().contains("qlineargradient"));
            QVERIFY(b->findChild<surface::InsetLayer*>());
            QVERIFY(b->graphicsEffect());
            if (!evidence.isEmpty())
                QVERIFY(b->grab().save(evidence + "/button-" + id + ".png"));
        }
    }
    void fieldActionsReuseButton() {
        Z_InputField input(tokens);
        Z_Select select(tokens);
        select.setMode(Z_Select::Mode::Multiple);
        select.setOptions({{"a", "Alpha", {}, false}});
        select.setValues({"a"});
        Z_PushButton reference;
        reference.setType(ButtonType::Text);
        reference.setSize(ButtonSize::XS);
        reference.setLeadingIconSource(":/icons/x.svg");
        reference.setAccessibleName("Clear");
        reference.setIconOnly(true);
        input.setKind(Z_InputField::Kind::Search);
        input.setValue("recipe");
        input.setAllowClear(true);
        input.show();
        select.show();
        QTest::qWait(30);
        int total = 0;
        for (auto* parent : QList<QWidget*>{&input, &select}) {
            const auto actions = parent->findChildren<field::Action*>();
            for (auto* action : actions) {
                QVERIFY(qobject_cast<Z_PushButton*>(action));
                QVERIFY(action->isIconOnly());
                QCOMPARE(action->property("buttonType").toString(), QString("TEXT"));
                QCOMPARE(action->property("buttonSize").toString(), QString("XS"));
                QCOMPARE(action->sizeHint(), reference.sizeHint());
                ++total;
            }
        }
        QCOMPARE(total, 6);
        auto* search = input.findChild<Z_PushButton*>("SEARCH_ACTION");
        input.setLoading(true);
        QVERIFY(search->property("loading").toBool());
        QVERIFY(!search->isEnabled());
        input.setLoading(false);
        QVERIFY(search->isEnabled());
        auto* clear = input.findChild<Z_PushButton*>("CLEAR");
        clear->click();
        QCOMPARE(input.value(), QString());
    }
    void selectRowsReuseCheckbox() {
        Z_Select select(tokens);
        select.setMode(Z_Select::Mode::Multiple);
        select.setOptions({{"a", "Alpha", {}, false}, {"b", "Beta", {}, true}});
        select.setValues({"a"});
        QCOMPARE(select.findChildren<Z_CheckBox*>().count(), 1);
        select.resize(420, select.sizeHint().height());
        select.show();
        select.setOpen(true);
        QTest::qWait(40);
        auto* view = select.optionView();
        auto* renderer = select.findChild<Z_CheckBox*>("CHECKBOX_ROW_RENDERER");
        for (const auto& appearance : {"solid", "gradient-inset", "gradient-elevated"}) {
            select.setAppearance(appearance);
            for (int row = 0; row < 2; ++row) {
                QImage image(360, 48, QImage::Format_ARGB32_Premultiplied);
                image.fill(Qt::white);
                QStyleOptionViewItem opt;
                opt.initFrom(view);
                opt.widget = view;
                opt.rect = image.rect();
                opt.state = row == 0 ? QStyle::State_Enabled : QStyle::State_None;
                QPainter painter(&image);
                view->itemDelegate()->paint(&painter, opt, view->model()->index(row, 0));
                painter.end();
                QCOMPARE(renderer->isChecked(), row == 0);
                QCOMPARE(renderer->isEnabled(), row == 0);
                QCOMPARE(renderer->appearance(), QString(appearance));
                const auto area = renderer->indicatorRect().adjusted(3, 3, -3, -3);
                QImage direct(renderer->size(), QImage::Format_ARGB32_Premultiplied);
                direct.fill(Qt::white);
                renderer->render(&direct);
                const QPoint offset(tokens->metric("popupPadding"), 0);
                QCOMPARE(image.copy(area.translated(offset)).convertToFormat(QImage::Format_RGB32),
                         direct.copy(area).convertToFormat(QImage::Format_RGB32));
            }
            QTest::qWait(20);
            if (!evidence.isEmpty())
                QVERIFY(select.popupWidget()->grab().save(evidence + "/multiple-" + appearance +
                                                          ".png"));
        }
        QCOMPARE(select.values(), QStringList{"a"});
        select.setOpen(false);
        QVector<Z_Select::Option> many;
        for (int i = 0; i < 10000; ++i)
            many.append({QString::number(i), QString("Row %1").arg(i), {}, false});
        QElapsedTimer timer;
        timer.start();
        select.setOptions(many);
        qInfo("Multiple model 10000 rows: %lld ms", timer.elapsed());
        QCOMPARE(view->model()->rowCount(), 10000);
        QCOMPARE(select.findChildren<Z_CheckBox*>().count(), 1);
        QVERIFY(select.findChildren<QWidget*>().count() < 100);
    }
    void pageFlowGalleryKeepsNativeWizardStackAndPageEditors() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)=="516:11")tree->setCurrentItem(*it);
        QCoreApplication::processEvents();auto* page=part<QStackedWidget>("componentPages")->currentWidget();
        auto* sections=page->findChild<QTabWidget*>("componentSections");QVERIFY(sections);QCOMPARE(sections->count(),2);
        sections->setCurrentIndex(1);QTest::qWait(80);
        auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);
        QCOMPARE(view->property("variantSpecimenCount").toInt(),25);QCOMPARE(view->property("componentSetCount").toInt(),3);
        QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
        QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(1312,2140));
        QVERIFY(canvas->findChild<QStackedWidget*>());QVERIFY(canvas->findChild<QWizard*>());QVERIFY(canvas->findChild<QWizardPage*>());
        sections->setCurrentIndex(0);QTest::qWait(60);
        auto* choices=page->findChild<QComboBox*>("appearanceComponentSetControl");QVERIFY(choices);QCOMPARE(choices->count(),3);
        QCOMPARE(choices->itemText(0),QString("ComponentLab/StackedWidget"));
        QCOMPARE(choices->itemText(1),QString("ComponentLab/WizardPage"));
        QCOMPARE(choices->itemText(2),QString("ComponentLab/Wizard"));
        auto* editors=page->findChild<QStackedWidget*>("appearanceComponentSetEditors");QVERIFY(editors);
        auto* stackEditor=editors->widget(0);QVERIFY(stackEditor);
        auto* stack=stackEditor->findChild<QStackedWidget*>("componentInteractivePreview");QVERIFY(stack);
        const int initial=stack->count();QCOMPARE(initial,3);
        stackEditor->findChild<QPushButton*>("flowAddPageControl")->click();QCOMPARE(stack->count(),initial+1);
        stackEditor->findChild<QPushButton*>("flowRemovePageControl")->click();QCOMPARE(stack->count(),initial);
        stackEditor->findChild<QPushButton*>("flowReinsertPageControl")->click();QCOMPARE(stack->count(),initial+1);
        choices->setCurrentIndex(1);QTest::qWait(60);
        auto* pageEditor=editors->widget(1);QVERIFY(pageEditor);
        QVERIFY(pageEditor->findChild<QComboBox*>("flowKindControl"));QVERIFY(pageEditor->findChild<QLineEdit*>("flowFieldControl"));
        auto* standalone=pageEditor->findChild<QWizardPage*>("componentInteractivePreview");QVERIFY(standalone);
        pageEditor->findChild<QLineEdit*>("flowFieldControl")->setText("LOT-1");
        QCOMPARE(standalone->findChild<QLineEdit*>("flowRequiredField")->text(),QString("LOT-1"));
        choices->setCurrentIndex(2);QTest::qWait(60);
        auto* wizardEditor=editors->widget(2);QVERIFY(wizardEditor);
        auto* wizard=wizardEditor->findChild<QWizard*>("componentInteractivePreview");QVERIFY(wizard);
        QCOMPARE(wizard->pageIds().size(),4);
        QVERIFY(wizardEditor->findChild<QComboBox*>("flowWizardStyleControl"));
        QVERIFY(wizardEditor->findChild<QPushButton*>("flowForwardControl"));
        QVERIFY(wizardEditor->findChild<QPushButton*>("flowBackControl"));
        QTest::qWait(80);
        for(auto* editor:{stackEditor,pageEditor,wizardEditor})
            QCOMPARE(editor->findChild<QScrollArea*>("componentSettingsScroll")->horizontalScrollBar()->maximum(),0);
    }
    void compositeGalleriesConsumePublishedNavigationContracts() {
        for(const auto& asset:{"home.svg","home-accent.svg","home-disabled.svg","search.svg","notifications.svg","chevron-down.svg","chevron-left.svg","chevron-right.svg","document.svg","settings.svg"})
            QVERIFY(QFile::exists(QStringLiteral(":/figma/composite/%1").arg(asset)));
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");
        for(const auto& entry:QList<QPair<QString,QString>>{{"380:2","2013:28"},{"392:2","2010:1360"},{"1307:2","2014:66"}}){
            for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)==entry.first)tree->setCurrentItem(*it);
            QCoreApplication::processEvents();
            auto* page=part<QStackedWidget>("componentPages")->currentWidget();
            auto* sections=page->findChild<QTabWidget*>("componentSections");QVERIFY(sections);QCOMPARE(sections->count(),2);
            sections->setCurrentIndex(1);QTest::qWait(80);
            auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);
            QCOMPARE(view->property("galleryNodeId").toString(),entry.second);
            QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
            QVERIFY(canvas);
            const QImage pixels=canvas->grab().toImage().convertToFormat(QImage::Format_ARGB32);
            const auto nonWhite=[&pixels](QRect area){
                const qreal dpr=pixels.devicePixelRatio();
                area=QRect(qRound(area.x()*dpr),qRound(area.y()*dpr),
                           qRound(area.width()*dpr),qRound(area.height()*dpr));
                area=area.intersected(pixels.rect());int count=0;
                for(int y=area.top();y<=area.bottom();++y)for(int x=area.left();x<=area.right();++x){
                    const QColor color=pixels.pixelColor(x,y);
                    if(color.alpha()>0&&(color.red()<248||color.green()<248||color.blue()<248))++count;
                }
                return count;
            };
            if(entry.first=="380:2"){
                QCOMPARE(view->property("variantSpecimenCount").toInt(),2);QCOMPARE(view->property("componentSetCount").toInt(),1);QCOMPARE(canvas->size(),QSize(2576,256));
                const auto navbars=canvas->findChildren<Z_Navbar*>();QCOMPARE(navbars.size(),2);QCOMPARE(canvas->property("runtimeSpecimenCount").toInt(),2);
                QSet<QString> tuples;
                for(auto* navbar:navbars){tuples.insert(navbar->variantTuple());QCOMPARE(navbar->geometry(),navbar->property("figmaDesignBounds").toRect());QCOMPARE(navbar->focusPolicy(),Qt::NoFocus);QVERIFY(navbar->searchInput());QCOMPARE(navbar->notificationButton()->focusPolicy(),Qt::StrongFocus);QCOMPARE(navbar->profileButton()->focusPolicy(),Qt::StrongFocus);QVERIFY(navbar->grab().save(evidence+"/navbar-"+navbar->density().toLower()+".png"));}
                QCOMPARE(tuples,QSet<QString>({QStringLiteral("Density=Comfortable"),QStringLiteral("Density=Compact")}));
                QVERIFY(nonWhite(QRect(68,125,1200,63))>500);QVERIFY(nonWhite(QRect(1308,125,1200,56))>500);
            }
            if(entry.first=="392:2"){
                QCOMPARE(view->property("variantSpecimenCount").toInt(),20);QCOMPARE(view->property("componentSetCount").toInt(),2);QCOMPARE(canvas->size(),QSize(1312,1059));
                const auto sidebars=canvas->findChildren<Z_Sidebar*>();QCOMPARE(sidebars.size(),4);QCOMPARE(canvas->property("runtimeSpecimenCount").toInt(),4);
                QSet<QString> tuples;
                for(auto* sidebar:sidebars){tuples.insert(sidebar->variantTuple());QCOMPARE(sidebar->geometry(),sidebar->property("figmaDesignBounds").toRect());QCOMPARE(sidebar->focusPolicy(),Qt::NoFocus);QVERIFY(sidebar->testAttribute(Qt::WA_StyledBackground));QCOMPARE(sidebar->findChildren<QToolButton*>(QRegularExpression("sidebarRoute.*")).size(),6);QCOMPARE(sidebar->collapseButton()->focusPolicy(),Qt::StrongFocus);QVERIFY(sidebar->grab().save(evidence+"/sidebar-"+sidebar->density().toLower()+(sidebar->isCollapsed()?"-collapsed":"-expanded")+".png"));}
                QCOMPARE(tuples,QSet<QString>({QStringLiteral("Density=Comfortable, Collapsed=False"),QStringLiteral("Density=Comfortable, Collapsed=True"),QStringLiteral("Density=Compact, Collapsed=False"),QStringLiteral("Density=Compact, Collapsed=True")}));
                for(const QRect& area:{QRect(236,471,280,520),QRect(556,471,80,520),QRect(676,471,280,466),QRect(996,471,80,466)})QVERIFY(nonWhite(area)>300);
            }
            if(entry.first=="1307:2"){
                QCOMPARE(view->property("variantSpecimenCount").toInt(),20);QCOMPARE(view->property("componentSetCount").toInt(),3);QCOMPARE(canvas->size(),QSize(1312,846));
                const auto bars=canvas->findChildren<Z_DesktopBottomNavigation*>();QCOMPARE(bars.size(),2);QCOMPARE(canvas->property("runtimeSpecimenCount").toInt(),2);
                QSet<QString> tuples;
                for(auto* bar:bars){
                    tuples.insert(bar->variantTuple());QCOMPARE(bar->geometry(),bar->property("figmaDesignBounds").toRect());
                    QCOMPARE(bar->findChildren<QToolButton*>().size(),3);QCOMPARE(bar->findChildren<QPushButton*>().size(),3);
                    QCOMPARE(bar->findChildren<QFrame*>(QStringLiteral("desktopBottomNavigationSeparator")).size(),2);
                    QVERIFY(bar->grab().save(evidence+"/desktop-bottom-navigation-"+bar->iconPlacement().toLower()+".png"));
                }
                QCOMPARE(tuples,QSet<QString>({QStringLiteral("IconPlacement=Top"),QStringLiteral("IconPlacement=Start")}));
                QVERIFY(nonWhite(QRect(236,475,840,138))>1000);QVERIFY(nonWhite(QRect(236,656,840,122))>1000);
            }
            sections->setCurrentIndex(0);QTest::qWait(40);
            auto* choices=page->findChild<QComboBox*>("appearanceComponentSetControl");
            if(entry.first=="380:2"){QVERIFY(choices);QCOMPARE(choices->count(),1);QCOMPARE(choices->itemText(0),QString("DSM/Navbar"));}
            if(entry.first=="392:2"){
                QVERIFY(!choices);
                auto* sidebar=page->findChild<Z_Sidebar*>("navigationInteractivePreview");QVERIFY(sidebar);
                QCOMPARE(sidebar->variantTuple(),QString("Density=Comfortable, Collapsed=False"));
                auto* density=page->findChild<QComboBox*>("navigationDensity");QVERIFY(density);
                density->setCurrentText("Compact");QCOMPARE(sidebar->density(),QString("Compact"));
                auto* collapse=page->findChild<QCheckBox*>("sidebarCollapsedControl");QVERIFY(collapse);
                collapse->setChecked(true);QCOMPARE(sidebar->isCollapsed(),true);QCOMPARE(sidebar->size(),sidebar->sizeHint());
                auto* route=page->findChild<QComboBox*>("sidebarActiveRouteControl");QVERIFY(route);route->setCurrentIndex(1);
                QCOMPARE(sidebar->activeRoute(),route->currentData().toString());
            }
            if(entry.first=="1307:2"){QVERIFY(choices);QCOMPARE(choices->count(),3);}
        }
    }
    void dataViewsGalleriesReproduceFigmaListContracts() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");
        for(const auto& entry:QList<QPair<QString,QString>>{{"707:19","2016:22888"},{"709:2","2016:23407"},{"712:8","2015:27014"},{"713:2","2015:27533"}}){
            for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)==entry.first)tree->setCurrentItem(*it);
            QCoreApplication::processEvents();
            auto* page=part<QStackedWidget>("componentPages")->currentWidget();
            auto* sections=page->findChild<QTabWidget*>("componentSections");QVERIFY(sections);QCOMPARE(sections->count(),2);
            sections->setCurrentIndex(1);QTest::qWait(80);
            auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);
            QCOMPARE(view->property("galleryNodeId").toString(),entry.second);
            QCOMPARE(view->property("variantSpecimenCount").toInt(),12);
            QCOMPARE(view->property("componentSetCount").toInt(),1);
            QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
            QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(1596,1157));
            sections->setCurrentIndex(0);QTest::qWait(40);
            auto* choices=page->findChild<QComboBox*>("appearanceComponentSetControl");QVERIFY(choices);QCOMPARE(choices->count(),1);
            auto* editors=page->findChild<QStackedWidget*>("appearanceComponentSetEditors");QVERIFY(editors);
            if(entry.first=="707:19"){QVERIFY(editors->widget(0)->findChild<QListView*>("componentInteractivePreview"));}
            if(entry.first=="709:2"){QVERIFY(editors->widget(0)->findChild<QListWidget*>("componentInteractivePreview"));}
            if(entry.first=="712:8"){QVERIFY(editors->widget(0)->findChild<QTreeView*>("componentInteractivePreview"));}
            if(entry.first=="713:2"){QVERIFY(editors->widget(0)->findChild<QTreeWidget*>("componentInteractivePreview"));}
        }
        for(const auto& entry:QList<QPair<QString,QString>>{{"717:2","2016:28"},{"1111:2","2016:2206"},{"363:2","2015:25457"}}){
            for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)==entry.first)tree->setCurrentItem(*it);
            QCoreApplication::processEvents();
            auto* page=part<QStackedWidget>("componentPages")->currentWidget();
            auto* sections=page->findChild<QTabWidget*>("componentSections");
            sections->setCurrentIndex(1);QTest::qWait(80);
            auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);
            QCOMPARE(view->property("galleryNodeId").toString(),entry.second);
            QCOMPARE(view->property("variantSpecimenCount").toInt(),15);
            QCOMPARE(view->property("componentSetCount").toInt(),1);
            QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
            QVERIFY(canvas);QCOMPARE(canvas->size(),QSize(2116,1373));
        }
        for(const auto& entry:QList<QPair<QString,QString>>{{"720:6","2016:1168"},{"721:5","2016:1687"}}){
            for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)==entry.first)tree->setCurrentItem(*it);
            QCoreApplication::processEvents();
            auto* page=part<QStackedWidget>("componentPages")->currentWidget();
            auto* sections=page->findChild<QTabWidget*>("componentSections");
            sections->setCurrentIndex(1);QTest::qWait(80);
            auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);
            QCOMPARE(view->property("galleryNodeId").toString(),entry.second);
            QCOMPARE(view->property("variantSpecimenCount").toInt(),entry.first=="720:6"?12:14);
            QCOMPARE(view->property("componentSetCount").toInt(),1);
            QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
            QVERIFY(canvas);
            QCOMPARE(canvas->size(),entry.first=="720:6"?QSize(1596,1157):QSize(1596,1445));
            sections->setCurrentIndex(0);QTest::qWait(40);
            auto* editors=page->findChild<QStackedWidget*>("appearanceComponentSetEditors");QVERIFY(editors);
            if(entry.first=="720:6"){QVERIFY(editors->widget(0)->findChild<QColumnView*>("componentInteractivePreview"));}
            if(entry.first=="721:5"){QVERIFY(editors->widget(0)->findChild<QUndoView*>("componentInteractivePreview"));}
        }
        for(const auto& asset:{"tree-expanded.svg","tree-collapsed.svg","branch-leaf.svg","sort.svg"})
            QVERIFY(QFile::exists(QStringLiteral(":/figma/composite/%1").arg(asset)));
    }
    void windowsGalleriesPreserveVariantTuplesAndNativeBoundaries() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");
        for(const auto& pageId:{QStringLiteral("573:2"),QStringLiteral("588:12"),QStringLiteral("591:14"),QStringLiteral("598:2")}){
            const auto expected=native_gallery::figmaGalleryLayout(pageId);
            for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)==pageId)tree->setCurrentItem(*it);
            QCoreApplication::processEvents();
            auto* page=part<QStackedWidget>("componentPages")->currentWidget();
            auto* sections=page->findChild<QTabWidget*>("componentSections");
            sections->setCurrentIndex(1);QTest::qWait(80);
            auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);
            QCOMPARE(view->property("galleryNodeId").toString(),expected.galleryId);
            QCOMPARE(view->property("variantSpecimenCount").toInt(),expected.variants.size());
            QCOMPARE(view->property("componentSetCount").toInt(),expected.componentSetCount);
            QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
            QVERIFY(canvas);QCOMPARE(canvas->size(),expected.size);
            const int count=canvas->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly).size();
            QVERIFY(count>0);
            QCOMPARE(actualVariantNames(canvas), expectedVariantNames(pageId));
        }
        auto* dialogCanvas = variantCanvas(QStringLiteral("573:2"));
        QVERIFY(dialogCanvas);
        const auto dialogVariants = actualVariantNames(dialogCanvas);
        QCOMPARE(dialogVariants.size(), 22);
        int mediumCount = 0;
        for (const auto& variant : dialogVariants)
            if (variant.startsWith(QStringLiteral("Modality=")) &&
                variant.contains(QStringLiteral("Size=Medium")))
                ++mediumCount;
        QCOMPARE(mediumCount, 6);
        QCOMPARE(dialogCanvas->findChildren<QMessageBox*>().size(), 10);
        for (auto* box : dialogCanvas->findChildren<QMessageBox*>()) {
            QVERIFY(box->property("lifecycleBoundary").toBool());
            QVERIFY(!box->isVisible());
        }

        auto* pickerCanvas = variantCanvas(QStringLiteral("591:14"));
        QVERIFY(pickerCanvas);
        const auto boundaries = pickerCanvas->findChildren<QWidget*>("nativeSurfaceBoundary");
        QCOMPARE(boundaries.size(), 12);
        for (auto* boundary : boundaries) {
            QVERIFY(boundary->property("nativeSurfaceBoundary").toBool());
            QVERIFY(boundary->property("figmaVariant").toString().contains(QStringLiteral("Surface=Native")));
        }
        QCOMPARE(pickerCanvas->findChildren<QFileDialog*>().size(), 4);
        QCOMPARE(pickerCanvas->findChildren<QColorDialog*>().size(), 4);
        QCOMPARE(pickerCanvas->findChildren<QFontDialog*>().size(), 4);
        for (auto* dialog : pickerCanvas->findChildren<QDialog*>()) {
            if (!dialog->property("figmaVariant").toString().isEmpty())
                QVERIFY2(dialog->property("figmaVariant").toString().contains(QStringLiteral("Surface=QtWidget")),
                         qPrintable(dialog->property("figmaVariant").toString()));
        }
    }
    void containersGalleriesUseRealNativeSpecimens() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");
        for(const auto& pageId:{QStringLiteral("520:11"),QStringLiteral("526:11"),QStringLiteral("531:11"),QStringLiteral("537:2")}){
            const auto expected=native_gallery::figmaGalleryLayout(pageId);
            for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)==pageId)tree->setCurrentItem(*it);
            QCoreApplication::processEvents();
            auto* page=part<QStackedWidget>("componentPages")->currentWidget();
            auto* sections=page->findChild<QTabWidget*>("componentSections");
            sections->setCurrentIndex(1);QTest::qWait(80);
            auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);
            QCOMPARE(view->property("galleryNodeId").toString(),expected.galleryId);
            QCOMPARE(view->property("variantSpecimenCount").toInt(),expected.variants.size());
            QCOMPARE(view->property("componentSetCount").toInt(),expected.componentSetCount);
            QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
            QVERIFY(canvas);QCOMPARE(canvas->size(),expected.size);
            QCOMPARE(actualVariantNames(canvas), expectedVariantNames(pageId));
            if(pageId=="520:11")QVERIFY(canvas->findChild<QSplitter*>());
            if(pageId=="526:11")QVERIFY(canvas->findChild<QGroupBox*>());
            if(pageId=="531:11")QVERIFY(canvas->findChild<QScrollArea*>());
            if(pageId=="537:2")QVERIFY(canvas->findChild<QFrame*>());
        }
        auto* splitterCanvas = variantCanvas(QStringLiteral("520:11"));
        QVERIFY(splitterCanvas);
        commonWidget::Z_Splitter* hover = nullptr;
        commonWidget::Z_Splitter* dragging = nullptr;
        for (auto* splitter : splitterCanvas->findChildren<QSplitter*>()) {
            const auto variant = splitter->property("figmaVariant").toString();
            if (variant.contains(QStringLiteral("State=Hover")))
                hover = qobject_cast<commonWidget::Z_Splitter*>(splitter);
            if (variant.contains(QStringLiteral("State=Dragging")))
                dragging = qobject_cast<commonWidget::Z_Splitter*>(splitter);
        }
        QVERIFY(hover);
        QVERIFY(dragging);
        QCOMPARE(hover->focusPolicy(), Qt::NoFocus);
        QCOMPARE(hover->handleWidth(), 12);
        QCOMPARE(dragging->handleWidth(), 12);
        QVERIFY(hover->showGrip());
        QCOMPARE(hover->snapshotState(), QString("Hover"));
        QCOMPARE(dragging->snapshotState(), QString("Dragging"));
        QCOMPARE(hover->handle(1)->property("figmaSnapshotState").toString(), QString("Hover"));
        QCOMPARE(dragging->handle(1)->property("figmaSnapshotState").toString(),
                 QString("Dragging"));
        QVERIFY(dragging->handle(1)->isVisibleTo(dragging));
        QCOMPARE(qMin(dragging->handle(1)->width(), dragging->handle(1)->height()), 12);
        auto* dragEvidence = dragging->findChild<QWidget*>(
            QStringLiteral("splitterRubberBandEvidence"));
        QVERIFY(dragEvidence);
        QVERIFY(dragEvidence->isVisibleTo(dragging));
        QCOMPARE(qMin(dragEvidence->width(), dragEvidence->height()), 2);
        QVERIFY(hover->styleSheet().isEmpty());
        QVERIFY(dragging->styleSheet().isEmpty());

        tree->setCurrentItem(pageItem(tree, QStringLiteral("520:11")));
        QCoreApplication::processEvents();
        auto* splitterPage=part<QStackedWidget>("componentPages")->currentWidget();
        splitterPage->findChild<QTabWidget*>("componentSections")->setCurrentIndex(0);
        QCoreApplication::processEvents();
        auto* interactive=splitterPage->findChild<QSplitter*>("splitterInteractivePreview");
        QVERIFY(interactive);QCOMPARE(interactive->count(),2);QCOMPARE(interactive->size(),QSize(554,196));
        auto* collapseButton=splitterPage->findChild<QPushButton*>("splitterCollapseControl");QVERIFY(collapseButton);
        collapseButton->click();QCoreApplication::processEvents();QCOMPARE(interactive->sizes().first(),0);
        auto* restoreButton=splitterPage->findChild<QPushButton*>("splitterRestoreControl");QVERIFY(restoreButton);
        restoreButton->click();QCoreApplication::processEvents();QVERIFY(interactive->sizes().first()>0);
        QVERIFY(splitterPage->findChild<QLabel*>("splitterNativeReadout"));

        auto* scrollCanvas = variantCanvas(QStringLiteral("531:11"));
        QVERIFY(scrollCanvas);
        QVERIFY(qApp->styleSheet().contains(QStringLiteral("commonWidget::Z_ScrollArea")));
        QVERIFY(qApp->styleSheet().contains(QStringLiteral("Z_ScrollAreaContentSkeleton")));
        QCOMPARE(scrollCanvas->findChildren<QScrollArea*>().size(), 24);
        for (auto* area : scrollCanvas->findChildren<QScrollArea*>()) {
            const QString variant = area->property("figmaVariant").toString();
            const bool vertical = variant.contains(QStringLiteral("Bars=Vertical")) ||
                                  variant.contains(QStringLiteral("Bars=Both"));
            const bool horizontal = variant.contains(QStringLiteral("Bars=Horizontal")) ||
                                    variant.contains(QStringLiteral("Bars=Both"));
            QCOMPARE(area->verticalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
            QCOMPARE(area->horizontalScrollBarPolicy(), Qt::ScrollBarAsNeeded);
            auto* designArea = qobject_cast<commonWidget::Z_ScrollArea*>(area);
            QVERIFY(designArea);
            QCOMPARE(designArea->bars(), variant.contains("Bars=Both") ? QString("Both") :
                     variant.contains("Bars=Vertical") ? QString("Vertical") :
                     variant.contains("Bars=Horizontal") ? QString("Horizontal") : QString("None"));
            QCOMPARE(designArea->resizeMode(), variant.contains("Resize=Resizable")
                         ? QString("Resizable") : QString("Fixed"));
            QCOMPARE(designArea->snapshotState(), variant.contains("State=Focus")
                         ? QString("Focus") : variant.contains("State=Disabled")
                         ? QString("Disabled") : QString("Default"));
            QVERIFY(area->styleSheet().isEmpty());
            QCOMPARE(area->verticalScrollBar()->property("designSystemClass").toString(),
                     QString("commonWidget::Z_ScrollBar"));
            QCOMPARE(area->horizontalScrollBar()->property("designSystemStyleOwner").toString(),
                     QString("tokens"));
            QCOMPARE(area->findChildren<QLabel*>(QStringLiteral("scrollAreaContentTitle")).size(), 1);
            QCOMPARE(area->findChildren<QFrame*>(QStringLiteral("scrollAreaContentRow")).size(), 5);
            QCOMPARE(area->findChildren<QPushButton*>(QStringLiteral("scrollAreaFocusTarget")).size(), 1);
            for (auto* row : area->findChildren<QFrame*>(QStringLiteral("scrollAreaContentRow"))) {
                QCOMPARE(row->property("designSystemStyleOwner").toString(), QString("tokens"));
                QVERIFY(row->width() >= 238);
            }
            QCoreApplication::processEvents();
            QCOMPARE(area->verticalScrollBar()->isVisibleTo(area), vertical);
            QCOMPARE(area->horizontalScrollBar()->isVisibleTo(area), horizontal);
        }
        commonWidget::Z_ScrollArea policyOwner;
        policyOwner.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        policyOwner.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        policyOwner.setBars(QStringLiteral("Vertical"));
        QCOMPARE(policyOwner.verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
        QCOMPARE(policyOwner.horizontalScrollBarPolicy(), Qt::ScrollBarAlwaysOn);
    }
    void nativeDisplayGalleriesUseRealWidgets() {
        window->resize(1600,920);auto* tree=part<QTreeWidget>("componentTree");
        for(const auto& pageId:{QStringLiteral("418:11"),QStringLiteral("420:13"),QStringLiteral("422:16"),QStringLiteral("423:18"),QStringLiteral("426:15"),QStringLiteral("430:20"),QStringLiteral("431:18")}){
            const auto expected=native_gallery::figmaGalleryLayout(pageId);
            for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)==pageId)tree->setCurrentItem(*it);
            QCoreApplication::processEvents();
            auto* page=part<QStackedWidget>("componentPages")->currentWidget();
            auto* sections=page->findChild<QTabWidget*>("componentSections");
            sections->setCurrentIndex(1);QTest::qWait(80);
            auto* view=page->findChild<QGraphicsView*>("figmaVariantGallery");QVERIFY(view);
            QCOMPARE(view->property("galleryNodeId").toString(),expected.galleryId);
            QCOMPARE(view->property("variantSpecimenCount").toInt(),expected.variants.size());
            QCOMPARE(view->property("componentSetCount").toInt(),expected.componentSetCount);
            QWidget* canvas=nullptr;for(auto* item:view->scene()->items())if(auto* proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(item))canvas=proxy->widget();
            QVERIFY(canvas);QCOMPARE(canvas->size(),expected.size);
            QCOMPARE(actualVariantNames(canvas), expectedVariantNames(pageId));
            if(pageId=="418:11")QVERIFY(canvas->findChild<QLCDNumber*>());
            if(pageId=="420:13")QVERIFY(canvas->findChild<QTextBrowser*>());
            if(pageId=="422:16")QVERIFY(canvas->findChild<QGraphicsView*>());
            if(pageId=="423:18")QVERIFY(canvas->findChild<QCalendarWidget*>());
            if(pageId=="426:15")QVERIFY(canvas->findChild<QTextEdit*>());
            if(pageId=="430:20")QVERIFY(canvas->findChild<QPlainTextEdit*>());
            if(pageId=="431:18")QVERIFY(canvas->findChild<QOpenGLWidget*>());
        }
        auto* calendarCanvas=variantCanvas(QStringLiteral("423:18"));QVERIFY(calendarCanvas);
        const auto calendars=calendarCanvas->findChildren<commonWidget::Z_CalendarWidget*>();
        QCOMPARE(calendars.size(),8);
        QSet<QString> calendarTuples;
        for(auto* calendar:calendars){
            calendarTuples.insert(calendar->property("figmaVariant").toString());
            QCOMPARE(calendar->size(),QSize(440,360));
            QCOMPARE(calendar->selectedDate(),QDate(2026,9,23));
            QCOMPARE(calendar->firstDayOfWeek(),calendar->weekStart()=="Monday"?Qt::Monday:Qt::Sunday);
            QCOMPARE(calendar->isGridVisible(),calendar->grid());
            QCOMPARE(calendar->isEnabled(),calendar->snapshotState()!=QString("Disabled"));
            QCOMPARE(calendar->property("designSystemStyleOwner").toString(),QString("tokens"));
            // Variant-gallery specimens are intentionally passive; the public SDK
            // calendar remains keyboard-focusable in the interactive preview.
            QCOMPARE(calendar->focusPolicy(),Qt::NoFocus);
            QVERIFY(calendar->showNavigation());
            QCOMPARE(calendar->horizontalHeaderFormat(),QCalendarWidget::SingleLetterDayNames);
            QCOMPARE(calendar->layout()->contentsMargins(),QMargins(16,16,16,15));
            auto* navigation=calendar->findChild<QWidget*>(QStringLiteral("qt_calendar_navigationbar"));
            QVERIFY(navigation);
            QCOMPARE(navigation->height(),44);
            auto* previous=calendar->findChild<QToolButton*>(QStringLiteral("qt_calendar_prevmonth"));
            auto* next=calendar->findChild<QToolButton*>(QStringLiteral("qt_calendar_nextmonth"));
            QVERIFY(previous);QVERIFY(next);QCOMPARE(previous->size(),QSize(16,16));QCOMPARE(next->size(),QSize(16,16));
            auto* month=calendar->findChild<QToolButton*>(QStringLiteral("qt_calendar_monthbutton"));
            QVERIFY(month);QVERIFY(month->menu()==nullptr);QVERIFY(month->testAttribute(Qt::WA_TransparentForMouseEvents));
            QCOMPARE(navigation->palette().color(QPalette::Window),
                     calendar->palette().color(QPalette::Active,QPalette::AlternateBase));
            auto* calendarView=calendar->findChild<QAbstractItemView*>(QStringLiteral("qt_calendar_calendarview"));
            QVERIFY(calendarView);
            QStringList headerValues;
            for(int column=0;column<calendarView->model()->columnCount();++column)
                headerValues<<calendarView->model()->index(0,column).data().toString();
            QCOMPARE(headerValues,calendar->weekStart()==QStringLiteral("Monday")
                                      ? QStringList({"一","二","三","四","五","六","日"})
                                      : QStringList({"日","一","二","三","四","五","六"}));
            if(calendar->snapshotState()==QStringLiteral("Disabled")){
                QCOMPARE(calendarView->palette().color(QPalette::Disabled,QPalette::Base),
                         calendar->palette().color(QPalette::Active,QPalette::AlternateBase));
                QVERIFY(calendarView->styleSheet().contains(
                    calendar->palette().color(QPalette::Disabled,QPalette::Text).name(QColor::HexArgb)));
            }
        }
        QCOMPARE(calendarTuples.size(),8);
        if(!evidence.isEmpty())
            QVERIFY(calendarCanvas->grab().save(evidence+"/calendar-gallery.png"));
        tree->setCurrentItem(pageItem(tree,QStringLiteral("423:18")));QCoreApplication::processEvents();
        auto* calendarPage=part<QStackedWidget>("componentPages")->currentWidget();
        calendarPage->findChild<QTabWidget*>("componentSections")->setCurrentIndex(0);QCoreApplication::processEvents();
        auto* interactiveCalendar=calendarPage->findChild<commonWidget::Z_CalendarWidget*>("componentInteractivePreview");
        QVERIFY(interactiveCalendar);QCOMPARE(interactiveCalendar->size(),QSize(440,360));
        QCOMPARE(interactiveCalendar->focusPolicy(),Qt::StrongFocus);
        auto* weekStart=calendarPage->findChild<QComboBox*>("calendarWeekStartControl");QVERIFY(weekStart);
        weekStart->setCurrentText("Sunday");QCOMPARE(interactiveCalendar->weekStart(),QString("Sunday"));
        auto* grid=calendarPage->findChild<QCheckBox*>("calendarGridControl");QVERIFY(grid);grid->setChecked(false);
        QCOMPARE(interactiveCalendar->grid(),false);
        interactiveCalendar->setShowNavigation(false);QVERIFY(!interactiveCalendar->showNavigation());
        interactiveCalendar->setShowNavigation(true);QVERIFY(interactiveCalendar->showNavigation());
        for (const auto& pageId : {QStringLiteral("426:15"), QStringLiteral("430:20")}) {
            auto* canvas = variantCanvas(pageId);
            QVERIFY(canvas);
            bool foundFocus = false;
            for (auto* widget : canvas->findChildren<QWidget*>()) {
                if (widget->property("figmaVariant").toString().contains(QStringLiteral("State=Focus"))) {
                    foundFocus = true;
                    QVERIFY(widget->styleSheet().contains(QStringLiteral("#6d4aff")));
                }
            }
            QVERIFY(foundFocus);
        }
        auto* glCanvas = variantCanvas(QStringLiteral("431:18"));
        QVERIFY(glCanvas);
        bool foundPartial = false;
        bool foundNoPartial = false;
        for (auto* gl : glCanvas->findChildren<QOpenGLWidget*>()) {
            const QString variant = gl->property("figmaVariant").toString();
            if (variant.startsWith(QStringLiteral("Update=Partial"))) {
                foundPartial = true;
                QCOMPARE(gl->updateBehavior(), QOpenGLWidget::PartialUpdate);
            }
            if (variant.startsWith(QStringLiteral("Update=NoPartial"))) {
                foundNoPartial = true;
                QCOMPARE(gl->updateBehavior(), QOpenGLWidget::NoPartialUpdate);
            }
        }
        QVERIFY(foundPartial);
        QVERIFY(foundNoPartial);
    }
    void runtimeGalleriesUseFigmaEvidenceInsteadOfGenericMatrices() {
        const QStringList pages{
            QStringLiteral("397:2"), QStringLiteral("400:2"), QStringLiteral("402:2"),
            QStringLiteral("433:7"), QStringLiteral("436:8"), QStringLiteral("242:2720"),
            QStringLiteral("346:2"), QStringLiteral("351:2"), QStringLiteral("359:2"),
            QStringLiteral("363:2"), QStringLiteral("405:2"), QStringLiteral("542:2"),
            QStringLiteral("553:15"),
            QStringLiteral("567:2"), QStringLiteral("579:33"), QStringLiteral("727:14"),
            QStringLiteral("735:2"), QStringLiteral("737:2")};
        for (const auto& pageId : pages) {
            auto* canvas = variantCanvas(pageId);
            QVERIFY2(canvas, qPrintable(pageId));
            QVERIFY2(canvas->property("evidenceBacked").toBool(), qPrintable(pageId));
            QVERIFY2(!canvas->property("contractOnly").toBool(), qPrintable(pageId));
            QVERIFY2(canvas->property("runtimeBacked").toBool(), qPrintable(pageId));
            QVERIFY2(!canvas->findChild<QTabWidget*>("componentSetTabs"), qPrintable(pageId));
            const auto actual = actualVariantNames(canvas);
            const auto expected = expectedVariantNames(pageId);
            const auto missing = expected - actual;
            const auto extra = actual - expected;
            QVERIFY2(actual == expected,
                     qPrintable(pageId + QStringLiteral(" missing=") +
                                QStringList(missing.values()).join(QStringLiteral(" | ")) +
                                QStringLiteral(" extra=") +
                                QStringList(extra.values()).join(QStringLiteral(" | "))));
            QCOMPARE(canvas->property("variantSpecimenCount").toInt(),
                     [&] { int count = 0; for (const auto& p : commonWidget::figmaComponentPages()) if (p.id == pageId) for (const auto& s : p.componentSets) count += s.variants.size(); return count; }());
            if (pageId != QStringLiteral("242:2720"))
                QVERIFY2(canvas->property("runtimeSpecimenCount").toInt() > 0,
                         qPrintable(pageId));
        }
        auto* chart = variantCanvas(QStringLiteral("400:2"));
        QVERIFY(chart);
        const auto charts = chart->findChildren<commonWidget::Z_Chart*>("figmaRuntimeSpecimen");
        QCOMPARE(charts.size(), 6);
        int emptyCharts = 0;
        int statusCharts = 0;
        for (auto* specimen : charts) {
            QCOMPARE(specimen->size(), QSize(520, 300));
            QCOMPARE(specimen->title(), QStringLiteral("Chart title"));
            QCOMPARE(specimen->meta(), QStringLiteral("Last 7 days"));
            QCOMPARE(specimen->emptyTitle(), QStringLiteral("No chart data"));
            QCOMPARE(specimen->emptyHint(), QStringLiteral("Check filters or data source"));
            if (specimen->variantValue(QStringLiteral("State")) == QStringLiteral("Empty"))
                ++emptyCharts;
            if (specimen->variantValue(QStringLiteral("Kind")) == QStringLiteral("Status"))
                ++statusCharts;
        }
        QCOMPARE(emptyCharts, 3);
        QCOMPARE(statusCharts, 2);
        auto* metric = variantCanvas(QStringLiteral("397:2"));
        QVERIFY(metric);
        const auto metricCards =
            metric->findChildren<commonWidget::Z_MetricCard*>("figmaRuntimeSpecimen");
        QCOMPARE(metricCards.size(), 4);
        const QStringList metricStates{QStringLiteral("Default"), QStringLiteral("Loading"),
                                       QStringLiteral("Empty"), QStringLiteral("Error")};
        for (auto* card : metricCards) {
            QCOMPARE(card->titleText(), QStringLiteral("Total revenue"));
            QCOMPARE(card->focusPolicy(), Qt::NoFocus);
            QVERIFY(metricStates.contains(card->state()));
        }
        auto* statisticCanvas = variantCanvas(QStringLiteral("436:8"));
        QVERIFY(statisticCanvas);
        const auto statistics =
            statisticCanvas->findChildren<commonWidget::Z_Statistic*>("figmaRuntimeSpecimen");
        QCOMPARE(statistics.size(), 8);
        for (auto* statistic : statistics) {
            QCOMPARE(statistic->titleText(), QStringLiteral("Active users"));
            QVERIFY(statistic->showTitle());
            QVERIFY(!statistic->showPrefix());
            QVERIFY(statistic->showSuffix());
            QCOMPARE(statistic->focusPolicy(), Qt::NoFocus);
        }
        auto* dataTable = variantCanvas(QStringLiteral("363:2"));
        QVERIFY(dataTable);
        QCOMPARE(dataTable->findChildren<commonWidget::Z_DataTable*>("figmaRuntimeSpecimen").size(), 15);
    }
    void foundationComponentsUseSdkVariantsAndFocusContracts() {
        auto* tree = part<QTreeWidget>("componentTree");
        struct Expected { QString pageId; int count; };
        const QList<Expected> pages{{"2131:2", 10}, {"2131:3", 10}, {"2131:4", 20},
                                    {"2131:5", 5}, {"2131:6", 12}};
        for (const auto& expected : pages) {
            auto* item = pageItem(tree, expected.pageId);
            QVERIFY2(item, qPrintable(expected.pageId));
            tree->setCurrentItem(item);
            QCoreApplication::processEvents();
            auto* page = part<QStackedWidget>("componentPages")->currentWidget();
            auto* sections = page->findChild<QTabWidget*>("componentSections");
            QVERIFY(sections);
            sections->setCurrentIndex(1);
            QCoreApplication::processEvents();
            int runtimeVariants = 0;
            for (auto* widget : page->findChildren<QWidget*>())
                if (widget->objectName().startsWith("variantPreview_") &&
                    widget->property("runtimeImplementation").toBool())
                    ++runtimeVariants;
            QCOMPARE(runtimeVariants, expected.count);
        }
        auto* tooltipPage = part<QStackedWidget>("componentPages")->currentWidget();
        auto* tooltipMatrix = tooltipPage->findChild<QTableWidget*>("variantMatrix_2132_189");
        QVERIFY(tooltipMatrix);
        QCOMPARE(tooltipMatrix->rowCount(), 3);
        QCOMPARE(tooltipMatrix->columnCount(), 4);
        QVERIFY(tooltipMatrix->horizontalHeader()->isHidden());
        QCOMPARE(tooltipPage->findChildren<QLabel*>("tooltipPlacementLabel").size(), 12);
        commonWidget::ComponentFactory factory(profile);
        std::unique_ptr<QWidget> badge(factory.create("commonWidget::Z_Badge"));
        std::unique_ptr<QWidget> indicator(factory.create("commonWidget::Z_StatusIndicator"));
        std::unique_ptr<QWidget> link(factory.create("commonWidget::Z_Link"));
        std::unique_ptr<QWidget> tag(factory.create("commonWidget::Z_Tag"));
        std::unique_ptr<QWidget> tooltip(factory.create("commonWidget::Z_Tooltip"));
        auto* linkControl = qobject_cast<commonWidget::Z_Link*>(link.get());
        QVERIFY(linkControl);
        QCOMPARE(badge->focusPolicy(), Qt::NoFocus);
        QCOMPARE(indicator->focusPolicy(), Qt::NoFocus);
        QCOMPARE(link->focusPolicy(), Qt::StrongFocus);
        QCOMPARE(tag->focusPolicy(), Qt::NoFocus);
        QCOMPARE(tooltip->focusPolicy(), Qt::NoFocus);
        commonWidget::FocusIndicator::instance().setEnabledFor(link.get(), false);
        QVERIFY(!commonWidget::FocusIndicator::instance().enabledFor(link.get()));
        linkControl->setSnapshotState("Disabled");
        QCOMPARE(linkControl->snapshotState(), QString("Disabled"));
        QVERIFY(!linkControl->isEnabled());
        linkControl->setSnapshotState(QString());
        QVERIFY(linkControl->isEnabled());
        QVERIFY_EXCEPTION_THROWN(linkControl->setSnapshotState("Selected"),
                                 std::invalid_argument);
    }
    void interactionComponentsUseSdkVariantsAndFocusContracts() {
        auto* tree = part<QTreeWidget>("componentTree");
        struct Expected { QString pageId; int count; };
        const QList<Expected> pages{{"2147:2", 2}, {"2147:747", 5}, {"2147:960", 6},
                                    {"2147:1181", 5}, {"2148:1146", 6},
                                    {"2147:454", 4}, {"2148:342", 4},
                                    {"2148:2", 4}, {"2150:362", 2},
                                    {"2150:2", 4}, {"2148:164", 4},
                                    {"2148:908", 2}, {"2147:1374", 3},
                                    {"2148:552", 3},
                                    {"2148:746", 5}};
        for (const auto& expected : pages) {
            auto* item = pageItem(tree, expected.pageId);
            QVERIFY2(item, qPrintable(expected.pageId));
            tree->setCurrentItem(item);
            QCoreApplication::processEvents();
            auto* page = part<QStackedWidget>("componentPages")->currentWidget();
            auto* sections = page->findChild<QTabWidget*>("componentSections");
            QVERIFY(sections);
            sections->setCurrentIndex(1);
            QCoreApplication::processEvents();
            int runtimeVariants = 0;
            for (auto* widget : page->findChildren<QWidget*>())
                if (widget->objectName().startsWith("variantPreview_") &&
                    widget->property("runtimeImplementation").toBool())
                    ++runtimeVariants;
            QCOMPARE(runtimeVariants, expected.count);
        }

        // Isolate the standalone keyboard-focus probes from the gallery window.
        // The offscreen platform otherwise may reactivate the last selected
        // Upload specimen while another top-level probe is exercising arrows.
        window->hide();
        QApplication::processEvents();

        QWidget probeHost;
        probeHost.resize(760, 520);
        commonWidget::ComponentFactory factory(profile);
        std::unique_ptr<QWidget> searchWidget(factory.create("commonWidget::Z_SearchInput"));
        std::unique_ptr<QWidget> numberWidget(factory.create("commonWidget::Z_NumberInput"));
        std::unique_ptr<QWidget> colorPickerWidget(
            factory.create("commonWidget::Z_ColorPicker"));
        std::unique_ptr<QWidget> numericKeypadWidget(
            factory.create("commonWidget::Z_NumericKeypad"));
        std::unique_ptr<QWidget> overflowWidget(
            factory.create("commonWidget::Z_OverflowMenuButton"));
        std::unique_ptr<QWidget> segmentedWidget(
            factory.create("commonWidget::Z_SegmentedControl"));
        std::unique_ptr<QWidget> stepperWidget(factory.create("commonWidget::Z_Stepper"));
        std::unique_ptr<QWidget> timelineWidget(factory.create("commonWidget::Z_Timeline"));
        std::unique_ptr<QWidget> anchorWidget(
            factory.create("commonWidget::Z_AnchorNavigation"));
        std::unique_ptr<QWidget> filterWidget(factory.create("commonWidget::Z_Filter"));
        std::unique_ptr<QWidget> paginationWidget(factory.create("commonWidget::Z_Pagination"));
        std::unique_ptr<QWidget> pageHeaderWidget(factory.create("commonWidget::Z_PageHeader"));
        std::unique_ptr<QWidget> titleBarWidget(factory.create("commonWidget::Z_TitleBar"));
        auto* search = qobject_cast<commonWidget::Z_SearchInput*>(searchWidget.get());
        auto* number = qobject_cast<commonWidget::Z_NumberInput*>(numberWidget.get());
        auto* colorPicker = qobject_cast<commonWidget::Z_ColorPicker*>(
            colorPickerWidget.get());
        auto* numericKeypad = qobject_cast<commonWidget::Z_NumericKeypad*>(
            numericKeypadWidget.get());
        auto* overflow = qobject_cast<commonWidget::Z_OverflowMenuButton*>(overflowWidget.get());
        auto* segmented = qobject_cast<commonWidget::Z_SegmentedControl*>(segmentedWidget.get());
        auto* stepper = qobject_cast<commonWidget::Z_Stepper*>(stepperWidget.get());
        auto* timeline = qobject_cast<commonWidget::Z_Timeline*>(timelineWidget.get());
        auto* anchor = qobject_cast<commonWidget::Z_AnchorNavigation*>(anchorWidget.get());
        auto* filter = qobject_cast<commonWidget::Z_Filter*>(filterWidget.get());
        auto* pagination = qobject_cast<commonWidget::Z_Pagination*>(paginationWidget.get());
        auto* pageHeader = qobject_cast<commonWidget::Z_PageHeader*>(pageHeaderWidget.get());
        auto* titleBar = qobject_cast<commonWidget::Z_TitleBar*>(titleBarWidget.get());
        QVERIFY(search && number && colorPicker && numericKeypad && overflow && segmented && stepper && timeline && anchor && filter &&
                pagination && pageHeader && titleBar);

        // Exercise keyboard focus inside one stable top-level host. Creating a
        // succession of standalone probe windows makes the Qt offscreen
        // platform's active-window choice timing-dependent.
        const QList<QWidget*> keyboardProbes{colorPicker, numericKeypad, anchor, filter};
        for (auto* probe : keyboardProbes)
            probe->setParent(&probeHost);
        probeHost.show();
        QApplication::setActiveWindow(&probeHost);
        QApplication::processEvents();
        const auto sendNavigationKey = [](QWidget* target, int key) {
            QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
            const bool delivered = QApplication::sendEvent(target, &event);
            return delivered && event.isAccepted();
        };

        QCOMPARE(search->focusPolicy(), Qt::NoFocus);
        QVERIFY(search->lineEdit());
        QVERIFY(search->lineEdit()->focusPolicy() != Qt::NoFocus);
        search->setSnapshotState("Filled");
        search->setValue("wafer");
        QCOMPARE(search->value(), QString("wafer"));

        QCOMPARE(number->focusPolicy(), Qt::NoFocus);
        number->setType("Decimal");
        number->setValue(12.5);
        QCOMPARE(number->type(), QString("Decimal"));
        QVERIFY(qAbs(number->value() - 12.5) < 0.0001);
        QVERIFY(number->findChild<QAbstractSpinBox*>()->focusPolicy() != Qt::NoFocus);

        QCOMPARE(colorPicker->focusPolicy(), Qt::StrongFocus);
        QCOMPARE(colorPicker->sizeHint(), QSize(300, 40));
        QCOMPARE(colorPicker->color(), QColor("#5B5BD6"));
        QCOMPARE(colorPicker->colorText(), QString("#5B5BD6"));
        QSignalSpy pickerRequests(colorPicker, &commonWidget::Z_ColorPicker::pickerRequested);
        QSignalSpy colorChanges(colorPicker, &commonWidget::Z_ColorPicker::colorChanged);
        colorPicker->click();
        QCOMPARE(pickerRequests.count(), 1);
        QCOMPARE(pickerRequests.takeFirst().at(0).value<QColor>(), QColor("#5B5BD6"));
        colorPicker->setColor(QColor(0x12, 0x34, 0x56, 0x80));
        QCOMPARE(colorChanges.count(), 1);
        QCOMPARE(colorPicker->colorText(), QString("#123456"));
        colorPicker->setColorFormat("HexArgb");
        QCOMPARE(colorPicker->colorText(), QString("#80123456"));
        QVERIFY(colorPicker->accessibleDescription().contains("#80123456"));
        QVERIFY_EXCEPTION_THROWN(colorPicker->setColor(QColor()), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(colorPicker->setColorFormat("CssHex"), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(colorPicker->setSnapshotState("Pressed"), std::invalid_argument);
        colorPicker->setColor(QColor("#5B5BD6"));
        colorPicker->setColorFormat("HexRgb");
        colorPicker->setSnapshotState("Default");
        colorPicker->resize(colorPicker->sizeHint());
        colorPicker->show();
        QApplication::setActiveWindow(&probeHost);
        QApplication::processEvents();
        colorPicker->setFocus(Qt::TabFocusReason);
        QTest::keyClick(colorPicker, Qt::Key_Return);
        QCOMPARE(pickerRequests.count(), 1);
        commonWidget::FocusIndicator::instance().setEnabledFor(colorPicker, false);
        QVERIFY(!commonWidget::FocusIndicator::instance().enabledFor(colorPicker));
        if (!evidence.isEmpty()) {
            colorPicker->setSnapshotState("Default");
            QVERIFY(colorPicker->grab().save(evidence + "/color-picker-default.png"));
            colorPicker->setSnapshotState("Focus");
            QVERIFY(colorPicker->grab().save(evidence + "/color-picker-focus.png"));
            colorPicker->setSnapshotState("Disabled");
            QVERIFY(colorPicker->grab().save(evidence + "/color-picker-disabled.png"));
        }
        colorPicker->setSnapshotState("Default");
        commonWidget::FocusIndicator::instance().setEnabledFor(colorPicker, true);
        colorPicker->hide();
        QApplication::processEvents();

        QCOMPARE(numericKeypad->focusPolicy(), Qt::NoFocus);
        QCOMPARE(numericKeypad->density(), QString("Comfortable"));
        QCOMPARE(numericKeypad->sizeHint(), QSize(260, 290));
        QCOMPARE(numericKeypad->keyLayout().size(), 12);
        const auto keypadButtons = numericKeypad->findChildren<QToolButton*>();
        QCOMPARE(keypadButtons.size(), 12);
        for (auto* button : keypadButtons)
            QCOMPARE(button->focusPolicy(), Qt::StrongFocus);
        auto* sevenKey = numericKeypad->findChild<QToolButton*>("numericKeypadKey0");
        auto* eightKey = numericKeypad->findChild<QToolButton*>("numericKeypadKey1");
        auto* fiveKey = numericKeypad->findChild<QToolButton*>("numericKeypadKey4");
        auto* decimalKey = numericKeypad->findChild<QToolButton*>("numericKeypadKey9");
        auto* zeroKey = numericKeypad->findChild<QToolButton*>("numericKeypadKey10");
        auto* finalKey = numericKeypad->findChild<QToolButton*>("numericKeypadKey11");
        QVERIFY(sevenKey && eightKey && fiveKey && decimalKey && zeroKey && finalKey);
        QSignalSpy keyRequests(numericKeypad, &commonWidget::Z_NumericKeypad::keyPressed);
        QSignalSpy commitRequests(numericKeypad, &commonWidget::Z_NumericKeypad::commitRequested);
        zeroKey->click();
        QCOMPARE(keyRequests.count(), 1);
        QCOMPARE(keyRequests.takeFirst().at(0).toString(), QString("0"));
        numericKeypad->setAllowedKeys({"decimal", "0"});
        QVERIFY(!sevenKey->isEnabled());
        QVERIFY(decimalKey->isEnabled());
        numericKeypad->setDecimalSeparator(",");
        QCOMPARE(decimalKey->text(), QString(","));
        auto commitLayout = numericKeypad->keyLayout();
        commitLayout[11] = QStringLiteral("commit");
        numericKeypad->setKeyLayout(commitLayout);
        numericKeypad->setAllowedKeys({});
        finalKey->click();
        QCOMPARE(commitRequests.count(), 1);
        QCOMPARE(keyRequests.takeFirst().at(0).toString(), QString("commit"));
        numericKeypad->show();
        QApplication::setActiveWindow(&probeHost);
        QApplication::processEvents();
        QVERIFY(probeHost.isVisible());
        const bool keypadWindowActivationAvailable = probeHost.isActiveWindow();
        if (keypadWindowActivationAvailable) {
            sevenKey->setFocus(Qt::TabFocusReason);
            QTRY_COMPARE(QApplication::focusWidget(), sevenKey);
        }
        QVERIFY(sendNavigationKey(sevenKey, Qt::Key_Right));
        if (keypadWindowActivationAvailable)
            QTRY_COMPARE(QApplication::focusWidget(), eightKey);
        QVERIFY(sendNavigationKey(eightKey, Qt::Key_Down));
        if (keypadWindowActivationAvailable)
            QTRY_COMPARE(QApplication::focusWidget(), fiveKey);
        commonWidget::FocusIndicator::instance().setEnabledFor(numericKeypad, false);
        QVERIFY(!commonWidget::FocusIndicator::instance().enabledFor(fiveKey));
        QVERIFY_EXCEPTION_THROWN(numericKeypad->setDensity("Dense"), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(numericKeypad->setKeyLayout({"1"}), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(numericKeypad->setAllowedKeys({"unknown"}), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(numericKeypad->setDecimalSeparator(""), std::invalid_argument);
        numericKeypad->setKeyLayout({"7", "8", "9", "4", "5", "6",
                                     "1", "2", "3", "decimal", "0", "backspace"});
        numericKeypad->setDecimalSeparator(".");
        numericKeypad->setDensity("Comfortable");
        numericKeypad->resize(numericKeypad->sizeHint());
        QApplication::processEvents();
        QCOMPARE(numericKeypad->size(), QSize(260, 290));
        QCOMPARE(sevenKey->minimumHeight(), 50);
        QCOMPARE(sevenKey->maximumHeight(), 50);
        QCOMPARE(sevenKey->size(), QSize(68, 50));
        if (!evidence.isEmpty())
            QVERIFY(numericKeypad->grab().save(evidence + "/numeric-keypad-comfortable.png"));
        numericKeypad->setDensity("Compact");
        numericKeypad->resize(numericKeypad->sizeHint());
        QApplication::processEvents();
        QCOMPARE(numericKeypad->sizeHint(), QSize(260, 250));
        QCOMPARE(sevenKey->size(), QSize(68, 42));
        if (!evidence.isEmpty())
            QVERIFY(numericKeypad->grab().save(evidence + "/numeric-keypad-compact.png"));
        numericKeypad->hide();
        QApplication::processEvents();

        QCOMPARE(overflow->focusPolicy(), Qt::StrongFocus);
        commonWidget::FocusIndicator::instance().setEnabledFor(overflow, false);
        QVERIFY(!commonWidget::FocusIndicator::instance().enabledFor(overflow));

        QCOMPARE(segmented->focusPolicy(), Qt::NoFocus);
        segmented->setSegmentCount(4);
        segmented->setCurrentIndex(2);
        QCOMPARE(segmented->segmentCount(), 4);
        QCOMPARE(segmented->currentIndex(), 2);
        const auto segmentButtons = segmented->findChildren<QToolButton*>();
        QCOMPARE(segmentButtons.size(), 4);
        for (auto* button : segmentButtons)
            QCOMPARE(button->focusPolicy(), Qt::StrongFocus);

        QCOMPARE(stepper->focusPolicy(), Qt::NoFocus);
        stepper->setSnapshotState("Error");
        QCOMPARE(stepper->snapshotState(), QString("Error"));

        QCOMPARE(timeline->focusPolicy(), Qt::NoFocus);
        timeline->setDensity("Compact");
        QCOMPARE(timeline->sizeHint(), QSize(420, 190));
        timeline->setEvents({"Queued", "Running", "Completed"},
                            {"09:10", "09:20", "10:05"},
                            {"Brand", "Process", "Success"});
        QCOMPARE(timeline->eventLabels().last(), QString("Completed"));
        QVERIFY(timeline->accessibleDescription().contains("Completed"));
        QVERIFY_EXCEPTION_THROWN(timeline->setDensity("Dense"), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(
            timeline->setEvents({"Only"}, {}, {"Brand"}), std::invalid_argument);
        timeline->resize(timeline->sizeHint());
        if (!evidence.isEmpty())
            QVERIFY(timeline->grab().save(evidence + "/timeline.png"));

        QCOMPARE(anchor->focusPolicy(), Qt::NoFocus);
        anchor->setOrientation("Horizontal");
        anchor->setSnapshotState("Scrolled");
        QCOMPARE(anchor->sizeHint(), QSize(520, 64));
        QCOMPARE(anchor->currentIndex(), 1);
        QCOMPARE(anchor->snapshotState(), QString("Scrolled"));
        QVERIFY(anchor->accessibleDescription().contains("Parameters"));
        const auto anchorButtons = anchor->findChildren<QToolButton*>();
        QCOMPARE(anchorButtons.size(), 3);
        for (auto* button : anchorButtons)
            QCOMPARE(button->focusPolicy(), Qt::StrongFocus);
        QSignalSpy anchorRequests(anchor, &commonWidget::Z_AnchorNavigation::anchorActivated);
        anchorButtons[2]->click();
        QCOMPARE(anchorRequests.count(), 1);
        QCOMPARE(anchorRequests.takeFirst().at(0).toInt(), 2);
        QCOMPARE(anchor->currentIndex(), 1);
        anchor->show();
        QApplication::setActiveWindow(&probeHost);
        QApplication::processEvents();
        QVERIFY(probeHost.isVisible());
        const bool anchorWindowActivationAvailable = probeHost.isActiveWindow();
        if (anchorWindowActivationAvailable) {
            anchorButtons[0]->setFocus(Qt::TabFocusReason);
            QTRY_COMPARE(QApplication::focusWidget(), anchorButtons[0]);
        }
        QVERIFY(sendNavigationKey(anchorButtons[0], Qt::Key_Right));
        if (anchorWindowActivationAvailable)
            QTRY_COMPARE(QApplication::focusWidget(), anchorButtons[1]);
        commonWidget::FocusIndicator::instance().setEnabledFor(anchor, false);
        QVERIFY(!commonWidget::FocusIndicator::instance().enabledFor(anchorButtons[1]));
        QVERIFY_EXCEPTION_THROWN(anchor->setOrientation("Diagonal"), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(anchor->setItems({"Only"}), std::invalid_argument);
        anchor->resize(anchor->sizeHint());
        if (!evidence.isEmpty())
            QVERIFY(anchor->grab().save(evidence + "/anchor-navigation-horizontal.png"));
        anchor->setOrientation("Vertical");
        anchor->setSnapshotState("Default");
        anchor->resize(anchor->sizeHint());
        if (!evidence.isEmpty())
            QVERIFY(anchor->grab().save(evidence + "/anchor-navigation-vertical.png"));
        anchor->hide();
        QApplication::processEvents();

        QCOMPARE(filter->focusPolicy(), Qt::NoFocus);
        filter->setLayoutMode("Inline");
        filter->setSnapshotState("Applied");
        filter->setCriteria({"Material: Si", "Status"});
        filter->setApplyLabel("Apply 1");
        filter->resize(filter->sizeHint());
        QCOMPARE(filter->sizeHint(), QSize(430, 58));
        QCOMPARE(filter->criteria(), QStringList({"Material: Si", "Status"}));
        QCOMPARE(filter->findChildren<QToolButton*>().size(), 3);
        auto* material = filter->findChild<QToolButton*>("filterCriterion0");
        auto* status = filter->findChild<QToolButton*>("filterCriterion1");
        auto* apply = filter->findChild<QToolButton*>("filterApply");
        QVERIFY(material && status && apply);
        for (auto* button : {material, status, apply})
            QCOMPARE(button->focusPolicy(), Qt::StrongFocus);
        QSignalSpy criterionRequests(filter, &commonWidget::Z_Filter::criterionActivated);
        QSignalSpy applyRequests(filter, &commonWidget::Z_Filter::applyRequested);
        material->click();
        apply->click();
        QCOMPARE(criterionRequests.count(), 1);
        QCOMPARE(criterionRequests.takeFirst().at(0).toInt(), 0);
        QCOMPARE(applyRequests.count(), 1);
        QCOMPARE(filter->snapshotState(), QString("Applied"));
        filter->show();
        QApplication::setActiveWindow(&probeHost);
        QApplication::processEvents();
        QVERIFY(probeHost.isVisible());
        const bool filterWindowActivationAvailable = probeHost.isActiveWindow();
        if (filterWindowActivationAvailable) {
            material->setFocus(Qt::TabFocusReason);
            QTRY_COMPARE(QApplication::focusWidget(), material);
        }
        QVERIFY(sendNavigationKey(material, Qt::Key_Right));
        if (filterWindowActivationAvailable)
            QTRY_COMPARE(QApplication::focusWidget(), status);
        commonWidget::FocusIndicator::instance().setEnabledFor(filter, false);
        QVERIFY(!commonWidget::FocusIndicator::instance().enabledFor(status));
        QVERIFY_EXCEPTION_THROWN(filter->setLayoutMode("Drawer"), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(filter->setSnapshotState("Cleared"), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(filter->setCriteria({"Only"}), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(filter->setApplyLabel(""), std::invalid_argument);
        if (!evidence.isEmpty())
            QVERIFY(filter->grab().save(evidence + "/filter-inline-applied.png"));
        filter->setLayoutMode("Panel");
        filter->setSnapshotState("Default");
        filter->setCriteria({"Material", "Status"});
        filter->setApplyLabel("Apply");
        filter->resize(filter->sizeHint());
        QCOMPARE(filter->sizeHint(), QSize(430, 112));
        QCOMPARE(material->geometry(), QRect(15, -8, 145, 36));
        QCOMPARE(status->geometry(), QRect(15, 38, 110, 36));
        QCOMPARE(apply->geometry(), QRect(15, 84, 90, 36));
        if (!evidence.isEmpty())
            QVERIFY(filter->grab().save(evidence + "/filter-panel-default.png"));

        QCOMPARE(pagination->focusPolicy(), Qt::NoFocus);
        pagination->setPageCount(7);
        pagination->setCurrentPage(4);
        QCOMPARE(pagination->findChildren<QToolButton*>().size(), 5);
        for (auto* button : pagination->findChildren<QToolButton*>())
            QCOMPARE(button->focusPolicy(), Qt::StrongFocus);

        QCOMPARE(pageHeader->focusPolicy(), Qt::NoFocus);
        QVERIFY(pageHeader->actionButton());
        QCOMPARE(pageHeader->actionButton()->focusPolicy(), Qt::StrongFocus);
        pageHeader->setActionsVisible(false);
        QVERIFY(!pageHeader->actionsVisible());

        QCOMPARE(titleBar->focusPolicy(), Qt::NoFocus);
        QCOMPARE(titleBar->findChildren<QToolButton*>().size(), 3);
        for (auto* button : titleBar->findChildren<QToolButton*>())
            QCOMPARE(button->focusPolicy(), Qt::StrongFocus);
    }

    void transferUsesSdkControlledModelsAndFocusContract() {
        commonWidget::ComponentFactory factory(profile);
        std::unique_ptr<QWidget> transferWidget(factory.create("commonWidget::Z_Transfer"));
        auto* transfer = qobject_cast<commonWidget::Z_Transfer*>(transferWidget.get());
        QVERIFY(transfer);
        QCOMPARE(transfer->focusPolicy(), Qt::NoFocus);
        QCOMPARE(transfer->sizeHint(), QSize(520, 210));
        QCOMPARE(transfer->snapshotState(), QString("Default"));
        QCOMPARE(transfer->findChildren<QListView*>().size(), 2);
        QCOMPARE(transfer->findChildren<QToolButton*>().size(), 2);
        QCOMPARE(transfer->availableView()->focusPolicy(), Qt::StrongFocus);
        QCOMPARE(transfer->selectedView()->focusPolicy(), Qt::StrongFocus);
        QCOMPARE(transfer->moveToSelectedButton()->focusPolicy(), Qt::StrongFocus);
        QCOMPARE(transfer->moveToAvailableButton()->focusPolicy(), Qt::StrongFocus);

        QStringListModel available(
            {QStringLiteral("□ Chamber A"), QStringLiteral("□ Chamber B"),
             QStringLiteral("□ Chamber C")});
        QStringListModel selected({QStringLiteral("□ Chamber D")});
        transfer->setAvailableModel(&available);
        transfer->setSelectedModel(&selected);
        QCOMPARE(transfer->availableModel(), &available);
        QCOMPARE(transfer->selectedModel(), &selected);
        QSignalSpy selectionChanges(transfer, &commonWidget::Z_Transfer::selectionChanged);
        transfer->availableView()->selectionModel()->select(
            available.index(1, 0),
            QItemSelectionModel::Select | QItemSelectionModel::Rows);
        QCOMPARE(selectionChanges.count(), 1);

        bool moveRightRequested = false;
        QObject::connect(transfer, &commonWidget::Z_Transfer::moveToSelectedRequested,
                         [&moveRightRequested](const QModelIndexList& rows) {
                             moveRightRequested = rows.size() == 1 && rows.first().row() == 1;
                         });
        transfer->moveToSelectedButton()->click();
        QVERIFY(moveRightRequested);
        QCOMPARE(available.rowCount(), 3);
        QCOMPARE(selected.rowCount(), 1);

        transfer->setMoveToSelectedAllowed(false);
        QVERIFY(!transfer->moveToSelectedButton()->isEnabled());
        transfer->setMoveToSelectedAllowed(true);
        QVERIFY(transfer->moveToSelectedButton()->isEnabled());
        QVERIFY_EXCEPTION_THROWN(transfer->setSnapshotState("Error"), std::invalid_argument);
        transfer->setSnapshotState("Loading");
        QVERIFY(transfer->isEnabled());
        QVERIFY(transfer->accessibleDescription().contains("state Loading"));
        transfer->setSnapshotState("Disabled");
        QVERIFY(!transfer->isEnabled());
        transfer->setSnapshotState("Default");
        QVERIFY(transfer->isEnabled());

        commonWidget::FocusIndicator::instance().setEnabledFor(transfer, false);
        QVERIFY(!commonWidget::FocusIndicator::instance().enabledFor(
            transfer->availableView()));
        transfer->resize(transfer->sizeHint());
        if (!evidence.isEmpty()) {
            transfer->clearFocus();
            transfer->availableView()->clearSelection();
            transfer->selectedView()->clearSelection();
            transfer->setAttribute(Qt::WA_UnderMouse, false);
            const QStringList states{"Default", "Disabled", "Loading"};
            for (const auto& state : states) {
                if (state == QStringLiteral("Loading"))
                    selected.setStringList({QStringLiteral("Synchronizing")});
                else
                    selected.setStringList({QStringLiteral("□ Chamber D")});
                transfer->setSnapshotState(state);
                QVERIFY(transfer->grab().save(
                    evidence + "/transfer-" + state.toLower() + ".png"));
            }
        }
        transfer->setSnapshotState("Default");
        commonWidget::FocusIndicator::instance().setEnabledFor(transfer, true);

        auto* tree = part<QTreeWidget>("componentTree");
        tree->setCurrentItem(pageItem(tree, QStringLiteral("2148:552")));
        QCoreApplication::processEvents();
        auto* page = part<QStackedWidget>("componentPages")->currentWidget();
        auto* demo = page->findChild<commonWidget::Z_Transfer*>("componentInteractivePreview");
        QVERIFY(demo);
        auto* availableDemo = qobject_cast<QStandardItemModel*>(demo->availableModel());
        auto* selectedDemo = qobject_cast<QStandardItemModel*>(demo->selectedModel());
        QVERIFY(availableDemo && selectedDemo);
        const int availableBefore = availableDemo->rowCount();
        const int selectedBefore = selectedDemo->rowCount();
        demo->availableView()->selectionModel()->select(
            availableDemo->index(0, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
        demo->moveToSelectedButton()->click();
        QCOMPARE(availableDemo->rowCount(), availableBefore - 1);
        QCOMPARE(selectedDemo->rowCount(), selectedBefore + 1);
    }

    void tooltipSupportsTwelvePlacementsAndMultilineContent() {
        Z_Tooltip tooltip(profile);
        QCOMPARE(tooltip.content(), QStringLiteral("Helpful context"));
        const QStringList placements{"TopLeft", "Top", "TopRight", "RightTop", "Right", "RightBottom",
                                     "BottomRight", "Bottom", "BottomLeft", "LeftBottom", "Left", "LeftTop"};
        tooltip.setContent(QString::fromUtf8(
            "设备温度超过建议范围，需要检查当前工艺参数与联锁状态后再继续。"));
        const int oneLineHeight = Z_Tooltip(profile).sizeHint().height();
        for (const auto& placement : placements) {
            tooltip.setPlacement(placement);
            QCOMPARE(tooltip.placement(), placement);
            QVERIFY(tooltip.sizeHint().width() <= 303);
            QVERIFY(tooltip.sizeHint().height() >= oneLineHeight);
        }
        tooltip.setPlacement("Top");
        tooltip.setContent(QString::fromUtf8("第一行\n第二行"));
        QVERIFY(tooltip.sizeHint().height() > oneLineHeight);
        QVERIFY_EXCEPTION_THROWN(tooltip.setPlacement("Center"), std::invalid_argument);
    }

    void uploadUsesSdkLifecycleContract() {
        commonWidget::ComponentFactory factory(profile);
        std::unique_ptr<QWidget> uploadWidget(factory.create("commonWidget::Z_Upload"));
        auto* upload = qobject_cast<commonWidget::Z_Upload*>(uploadWidget.get());
        QVERIFY(upload);
        QCOMPARE(upload->focusPolicy(), Qt::StrongFocus);
        QCOMPARE(upload->sizeHint(), QSize(390, 150));
        QCOMPARE(upload->snapshotState(), QString("Default"));
        QCOMPARE(upload->progress(), 0);
        QCOMPARE(upload->helperText(), QString::fromUtf8("CSV, JSON · max 20 MB"));
        QSignalSpy browseRequests(upload, &commonWidget::Z_Upload::browseRequested);
        QSignalSpy droppedFiles(upload, &commonWidget::Z_Upload::filesDropped);
        QSignalSpy progressChanges(upload, &commonWidget::Z_Upload::progressChanged);
        upload->click();
        QCOMPARE(browseRequests.count(), 1);
        upload->setFileName("recipe");
        upload->setProgress(48);
        QCOMPARE(progressChanges.count(), 1);
        QVERIFY_EXCEPTION_THROWN(upload->setProgress(-1), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(upload->setProgress(101), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(upload->setSnapshotState("Disabled"), std::invalid_argument);
        upload->setSnapshotState("Uploading");
        QVERIFY(upload->accessibleDescription().contains("48 percent"));
        upload->click();
        QCOMPARE(browseRequests.count(), 1);
        upload->setSnapshotState("Default");
        QMimeData mimeData;
        mimeData.setUrls({QUrl::fromLocalFile("C:/fixtures/recipe.csv")});
        QDragEnterEvent dragEnter(QPoint(12, 12), Qt::CopyAction, &mimeData,
                                  Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(upload, &dragEnter);
        QVERIFY(dragEnter.isAccepted());
        QDropEvent drop(QPointF(12, 12), Qt::CopyAction, &mimeData,
                        Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(upload, &drop);
        QCOMPARE(droppedFiles.count(), 1);
        QCOMPARE(droppedFiles.takeFirst().at(0).toStringList(),
                 QStringList({QStringLiteral("C:/fixtures/recipe.csv")}));
        upload->resize(upload->sizeHint());
        QTest::keyClick(upload, Qt::Key_Return);
        QCOMPARE(browseRequests.count(), 2);
        commonWidget::FocusIndicator::instance().setEnabledFor(upload, false);
        QVERIFY(!commonWidget::FocusIndicator::instance().enabledFor(upload));
        if (!evidence.isEmpty()) {
            // QPixmap::grab() may leave WA_UnderMouse set in the offscreen
            // platform plugin. Keep state specimens deterministic; keyboard
            // focus and live hover behavior are asserted separately.
            upload->clearFocus();
            upload->setAttribute(Qt::WA_UnderMouse, false);
            const QStringList states{"Default", "Hover", "Uploading", "Success", "Error"};
            for (const auto& state : states) {
                upload->setSnapshotState(state);
                QVERIFY(upload->grab().save(evidence + "/upload-" + state.toLower() + ".png"));
            }
        }
        upload->setSnapshotState("Default");
        commonWidget::FocusIndicator::instance().setEnabledFor(upload, true);
    }
};
int main(int argc, char** argv) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    button_demo::install(app, qEnvironmentVariable("DEMO_PROFILE", "light-comfortable"));
    CompositionTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "CompositionTests.moc"
