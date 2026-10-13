#include "MappingGuide.h"
#include "QtMapping.h"
#include "DesignComponentCatalog.h"
#include <QBoxLayout>
#include <QDesktopServices>
#include <QFile>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QStyle>
#include <QTextBrowser>
#include <QTableWidget>
#include <QTabWidget>
#include <QTreeWidget>
#include <QUrl>
#include <functional>

namespace design_mapping {
void populateGuide(QWidget* page) {
    auto* layout = qobject_cast<QVBoxLayout*>(page->layout());
    auto* toolbar = new QHBoxLayout;
    auto* status = new QLabel(QString::fromUtf8(
        "Figma 73 页 / 123 组件集 / 1471 Variant | 开发映射 85 项 | "
        "85 项可实例化 | 0 项合同待实现 | 设计审批 0 项"));
    status->setObjectName("mappingDesignSourceStatus");
    status->setWordWrap(true);
    toolbar->addWidget(status, 1);
    auto* open = new QPushButton(page->style()->standardIcon(QStyle::SP_DirLinkIcon),
                                QString::fromUtf8("打开 Figma"));
    toolbar->addWidget(open);
    layout->addLayout(toolbar);
    QObject::connect(open, &QPushButton::clicked, page, [] {
        QDesktopServices::openUrl(QUrl("https://www.figma.com/design/w92BKW1EcutKmTXkwzNHbf?node-id=127-2710"));
    });
    auto* split = new QSplitter(page);
    split->setChildrenCollapsible(false);
    auto* explanation = new QTextBrowser(split);
    explanation->setObjectName("mappingImplementationGuide");
    explanation->setFrameShape(QFrame::NoFrame);
    explanation->setOpenExternalLinks(false);
    explanation->setHtml(QString::fromUtf8(read(":/mapping/overview.html")));
    explanation->setMinimumWidth(400);
    auto* variables = new QTreeWidget(split);
    variables->setObjectName("mappingDesignVariables");
    variables->setHeaderLabels({QString::fromUtf8("设计变量 / 本地路径"), QString::fromUtf8("Figma ID")});
    variables->setMinimumWidth(250);
    variables->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    variables->header()->setStretchLastSection(false);
    const auto manifest = parse(read(":/mapping/figma-variables.json"));
    for (const auto& value : manifest["variables"].toArray()) {
        const auto variable = value.toObject();
        auto* item = new QTreeWidgetItem(variables, {variable["sourcePath"].toString(), variable["id"].toString()});
        item->setToolTip(0, variable["sourcePath"].toString());
        const auto values = variable["values"].toObject();
        for (auto it = values.begin(); it != values.end(); ++it)
            new QTreeWidgetItem(item, {it.key(), it.value().toVariant().toString()});
    }
    split->addWidget(explanation);
    split->addWidget(variables);
    split->setSizes({760, 330});
    layout->addWidget(split, 1);
}

void populateCatalog(QWidget* page) {
    auto* layout = qobject_cast<QVBoxLayout*>(page->layout());
    const auto descriptors = commonWidget::designComponentDescriptors();
    const auto categories = commonWidget::designComponentCategories();
    const auto figmaCategories = commonWidget::figmaComponentCategories();
    const auto figmaPages = commonWidget::figmaComponentPages();
    int instantiableCount = 0;
    for (const auto& descriptor : descriptors)
        instantiableCount += descriptor.instantiable ? 1 : 0;
    page->setProperty("componentCount", descriptors.size());
    page->setProperty("instantiableCount", instantiableCount);
    page->setProperty("contractOnlyCount", descriptors.size() - instantiableCount);
    page->setProperty("figmaFileKey", "w92BKW1EcutKmTXkwzNHbf");
    page->setProperty("figmaPageCount", figmaPages.size());
    int componentSetCount = 0;
    int variantCount = 0;
    for (const auto& figmaPage : figmaPages) {
        componentSetCount += figmaPage.componentSets.size();
        for (const auto& componentSet : figmaPage.componentSets)
            variantCount += componentSet.variants.size();
    }
    page->setProperty("figmaComponentSetCount", componentSetCount);
    page->setProperty("figmaVariantCount", variantCount);
    auto* summary = new QLabel(QString::fromUtf8(
        "以当前 Figma 输出为设计事实源。Native 保持 Q<Class> → commonWidget::Z_<Class>；"
        "Design System / Composite 保持 ComponentLab/<Name> → commonWidget::Z_<Name>。"
        "合同态条目只登记计划类型，不宣称已有 Qt 实现。"), page);
    summary->setObjectName("designComponentCatalogSummary");
    summary->setWordWrap(true);
    layout->addWidget(summary);

    auto* tabs = new QTabWidget(page);
    tabs->setObjectName("designCatalogTabs");
    layout->addWidget(tabs, 1);

    auto* figmaStructure = new QWidget(tabs);
    auto* figmaLayout = new QVBoxLayout(figmaStructure);
    auto* figmaStatus = new QLabel(
        QString::fromUtf8("当前 Figma：%1 个规范页 · %2 个组件集 · %3 个 Variant")
            .arg(figmaPages.size()).arg(componentSetCount).arg(variantCount),
        figmaStructure);
    figmaStatus->setObjectName("figmaInventorySummary");
    figmaLayout->addWidget(figmaStatus);
    auto* figmaSplit = new QSplitter(figmaStructure);
    auto* tree = new QTreeWidget(figmaSplit);
    tree->setObjectName("figmaComponentTree");
    tree->setHeaderHidden(true);
    tree->setMinimumWidth(400);
    auto* sourceDetail = new QWidget(figmaSplit);
    auto* sourceLayout = new QVBoxLayout(sourceDetail);
    sourceLayout->setContentsMargins(0,0,0,0);
    auto* sourceStatus = new QLabel(sourceDetail);
    sourceStatus->setObjectName("figmaGallerySourceStatus");
    sourceStatus->setWordWrap(true);
    sourceLayout->addWidget(sourceStatus);
    auto* detailSplit = new QSplitter(Qt::Vertical,sourceDetail);
    sourceLayout->addWidget(detailSplit,1);
    auto* setTable = new QTableWidget(detailSplit);
    setTable->setObjectName("figmaComponentSetTable");
    setTable->setColumnCount(4);
    setTable->setHorizontalHeaderLabels({QString::fromUtf8("组件集"),
                                         QString::fromUtf8("Figma ID"),
                                         QString::fromUtf8("Variant 轴"),
                                         QString::fromUtf8("Variant 数量")});
    setTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    setTable->setSelectionMode(QAbstractItemView::NoSelection);
    setTable->verticalHeader()->setVisible(false);
    setTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    setTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    setTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    setTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    setTable->horizontalHeader()->setStretchLastSection(false);
    figmaSplit->addWidget(tree);
    auto* layoutTree = new QTreeWidget(detailSplit);
    layoutTree->setObjectName("figmaGalleryLayoutTree");
    layoutTree->setHeaderLabels({QString::fromUtf8("画廊 / 组件集 / 变体 / 组合"),"Figma ID","Type",QString::fromUtf8("x, y / 宽 × 高")});
    layoutTree->setColumnWidth(0,330);layoutTree->setColumnWidth(1,100);
    layoutTree->setColumnWidth(2,110);layoutTree->header()->setStretchLastSection(true);
    detailSplit->addWidget(setTable);detailSplit->addWidget(layoutTree);detailSplit->setSizes({180,400});
    figmaSplit->addWidget(sourceDetail);
    figmaSplit->setSizes({430, 690});
    figmaLayout->addWidget(figmaSplit, 1);

    QMap<QString, commonWidget::DesignComponentPage> pagesById;
    QMap<QString,QJsonObject> layoutsById;
    QFile layoutFile(":/figma/gallery-layouts.json");
    if (layoutFile.open(QIODevice::ReadOnly))
        for (const auto& sourcePage:QJsonDocument::fromJson(layoutFile.readAll()).object()["pages"].toArray()) {
            const auto source=sourcePage.toObject();layoutsById.insert(source["id"].toString(),source);
        }
    for (const auto& figmaPage : figmaPages)
        pagesById.insert(figmaPage.id, figmaPage);
    for (const auto& category : figmaCategories) {
        auto* branch = new QTreeWidgetItem(tree, {category.label});
        branch->setData(0, Qt::UserRole, category.id);
        for (const auto& figmaPage : figmaPages) {
            if (figmaPage.categoryId != category.id)
                continue;
            auto* leaf = new QTreeWidgetItem(branch, {figmaPage.name});
            leaf->setData(0, Qt::UserRole, figmaPage.id);
            leaf->setToolTip(0, figmaPage.name);
        }
        branch->setText(0, QString("%1 (%2)").arg(category.label).arg(branch->childCount()));
        branch->setToolTip(0, branch->text(0));
        branch->setExpanded(category.id == "20.1");
    }
    QObject::connect(tree, &QTreeWidget::currentItemChanged, figmaStructure,
                     [setTable, pagesById, layoutsById, layoutTree, sourceStatus](QTreeWidgetItem* current) {
        if (!current || !current->parent())
            return;
        const auto figmaPage = pagesById.value(current->data(0, Qt::UserRole).toString());
        setTable->setRowCount(figmaPage.componentSets.size());
        for (int row = 0; row < figmaPage.componentSets.size(); ++row) {
            const auto& componentSet = figmaPage.componentSets[row];
            QStringList axes;
            for (const auto& axis : componentSet.variantAxes)
                axes.append(axis.name + "=" + axis.values.join("/"));
            setTable->setItem(row, 0, new QTableWidgetItem(componentSet.name));
            setTable->setItem(row, 1, new QTableWidgetItem(componentSet.id));
            setTable->setItem(row, 2, new QTableWidgetItem(axes.join(QString::fromUtf8(" · "))));
            setTable->setItem(row, 3, new QTableWidgetItem(
                QString::number(componentSet.variants.size())));
        }
        setTable->setProperty("figmaPageId", figmaPage.id);
        layoutTree->clear();layoutTree->setProperty("figmaPageId",figmaPage.id);
        const auto source=layoutsById.value(figmaPage.id);
        const bool captured=source["status"]=="captured";
        layoutTree->setProperty("sourceCaptured",captured);
        sourceStatus->setText(captured ? QString::fromUtf8("Variant Gallery 实测结构 · 2026-10-12 · 坐标相对父节点；此处为设计证据，不等于运行时视觉验收。")
            : QString::fromUtf8("本页画廊结构尚未读回：%1。组件集目录仍可查看，不推测组合布局。")
                .arg(source["reason"].toString(QString::fromUtf8("缺少采集记录"))));
        std::function<void(QTreeWidgetItem*,const QJsonArray&)> appendNode;
        appendNode=[&](QTreeWidgetItem* parent,const QJsonArray& node) {
            const QStringList columns{node[1].toString(),node[0].toString(),node[2].toString(),
                QString("%1, %2 / %3 × %4").arg(node[3].toDouble()).arg(node[4].toDouble()).arg(node[5].toDouble()).arg(node[6].toDouble())};
            auto* item=parent?new QTreeWidgetItem(parent,columns):new QTreeWidgetItem(layoutTree,columns);
            item->setData(0,Qt::UserRole,node[0].toString());
            item->setToolTip(0,node[1].toString());
            if(node.size()>7 && node[7].isArray())
                for(const auto& child:node[7].toArray())appendNode(item,child.toArray());
        };
        if(captured)for(const auto& gallery:source["galleries"].toArray())appendNode(nullptr,gallery.toArray());
        layoutTree->expandToDepth(1);
    });
    tree->setCurrentItem(tree->topLevelItem(0)->child(0));
    tabs->addTab(figmaStructure, QString::fromUtf8("Figma 组件结构"));

    QMap<QString, QString> categoryLabels;
    for (const auto& category : categories)
        categoryLabels.insert(category.id, category.label);
    auto* mapping = new QWidget(tabs);
    auto* mappingLayout = new QVBoxLayout(mapping);
    mappingLayout->setContentsMargins(0, 8, 0, 0);
    auto* table = new QTableWidget(descriptors.size(), 8, mapping);
    table->setObjectName("designComponentCatalogTable");
    table->setHorizontalHeaderLabels({QString::fromUtf8("设计身份"),
                                      QString::fromUtf8("SDK 类型"),
                                      QString::fromUtf8("Qt 基类 / 结构"),
                                      QString::fromUtf8("组件类型"),
                                      QString::fromUtf8("分类"),
                                      QString::fromUtf8("Figma 页面 / 组件集"),
                                      QString::fromUtf8("变体"),
                                      QString::fromUtf8("运行状态")});
    for (int row = 0; row < descriptors.size(); ++row) {
        const auto& descriptor = descriptors[row];
        table->setItem(row, 0, new QTableWidgetItem(descriptor.designComponent));
        table->setItem(row, 1, new QTableWidgetItem(descriptor.sdkClass));
        table->setItem(row, 2, new QTableWidgetItem(descriptor.qtClass));
        table->setItem(row, 3, new QTableWidgetItem(descriptor.mappingKind));
        table->setItem(row, 4, new QTableWidgetItem(categoryLabels.value(descriptor.category)));
        table->setItem(row, 5, new QTableWidgetItem(
            descriptor.pageId + (descriptor.componentSetId.isEmpty()
                ? QString() : " / " + descriptor.componentSetId)));
        table->setItem(row, 6, new QTableWidgetItem(QString::number(descriptor.variantCount)));
        table->setItem(row, 7, new QTableWidgetItem(descriptor.runtimeStatus));
    }
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSortingEnabled(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    mappingLayout->addWidget(table);
    tabs->addTab(mapping, QString::fromUtf8("开发映射"));
}
}
