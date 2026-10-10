#include "NativeGalleryPages.h"
#include "MyDesignSystem.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractSlider>
#include <QCalendarWidget>
#include <QCheckBox>
#include <QColorDialog>
#include <QColumnView>
#include <QComboBox>
#include <QCommandLinkButton>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDial>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFontComboBox>
#include <QFontDialog>
#include <QFrame>
#include <QFormLayout>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLCDNumber>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMetaProperty>
#include <QMessageBox>
#include <QOpenGLWidget>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QProgressDialog>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSet>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QStandardItemModel>
#include <QStatusBar>
#include <QTableView>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QTimeEdit>
#include <QTimer>
#include <QToolBar>
#include <QToolBox>
#include <QTreeView>
#include <QTreeWidget>
#include <QUndoCommand>
#include <QUndoStack>
#include <QUndoView>
#include <QVBoxLayout>
#include <QWizard>
#include <QWizardPage>

#include <memory>
#include <functional>
#include <algorithm>

namespace native_gallery {
namespace {

using commonWidget::NativeComponentDescriptor;

struct Category {
    QString id;
    QString label;
    QStringList qtClasses;
};

QList<Category> categories() {
    return {
        {"foundation", QString::fromUtf8("基础与按钮"),
         {"QWidget", "QFrame", "QPushButton", "QToolButton",
          "QCommandLinkButton", "QDialogButtonBox", "QCheckBox", "QRadioButton",
          "QLineEdit"}},
        {"input", QString::fromUtf8("输入与数值"),
         {"QSpinBox", "QDoubleSpinBox", "QSlider", "QScrollBar", "QDial",
          "QComboBox", "QFontComboBox", "QDateTimeEdit", "QDateEdit", "QTimeEdit",
          "QKeySequenceEdit"}},
        {"display", QString::fromUtf8("显示与编辑"),
         {"QLabel", "QLCDNumber", "QProgressBar", "QTextBrowser", "QGraphicsView",
          "QCalendarWidget", "QTextEdit", "QPlainTextEdit"}},
        {"data", QString::fromUtf8("数据视图"),
         {"QListView", "QTreeView", "QTableView", "QColumnView", "QUndoView", "QListWidget",
          "QTreeWidget", "QTableWidget"}},
        {"container", QString::fromUtf8("容器与导航"),
         {"QGroupBox", "QScrollArea", "QTabWidget", "QToolBox", "QStackedWidget", "QSplitter",
          "QDockWidget", "QMdiArea", "QMdiSubWindow", "QWizard", "QWizardPage"}},
        {"window", QString::fromUtf8("菜单、窗口与对话框"),
         {"QMenu", "QMenuBar", "QToolBar", "QStatusBar", "QOpenGLWidget", "QDialog",
          "QMessageBox", "QFileDialog", "QColorDialog", "QFontDialog", "QInputDialog",
          "QProgressDialog"}},
    };
}

QList<NativeComponentDescriptor> visualDescriptors() {
    QList<NativeComponentDescriptor> result;
    for (const auto& descriptor : commonWidget::ComponentFactory::descriptors()) {
        if (descriptor.mappingKind == "visual-component")
            result.append(descriptor);
    }
    return result;
}

QString categoryId(const QString& qtClass) {
    if (qtClass == "semantic")
        return "foundation";
    for (const auto& category : categories()) {
        if (category.qtClasses.contains(qtClass))
            return category.id;
    }
    return "unclassified";
}

void populateItemModel(QAbstractItemView* view) {
    auto* model = new QStandardItemModel(4, 3, view);
    model->setHorizontalHeaderLabels(
        {QString::fromUtf8("设备"), QString::fromUtf8("状态"), QString::fromUtf8("值")});
    const QStringList devices{"ETCH-01", "CVD-02", "CMP-03", "WET-04"};
    for (int row = 0; row < devices.size(); ++row) {
        model->setData(model->index(row, 0), devices[row]);
        model->setData(model->index(row, 1), row == 2 ? QString::fromUtf8("维护")
                                                     : QString::fromUtf8("运行"));
        model->setData(model->index(row, 2), QString::number(42 + row));
    }
    view->setModel(model);
}

void configurePreview(QWidget* widget, const QString& qtClass, const QString& sdkClass) {
    widget->setObjectName("componentRoot_" + sdkClass.section("::", -1));
    if (!qobject_cast<QDialog*>(widget))
        widget->setWindowFlags(Qt::Widget);
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    if (auto* command = qobject_cast<QCommandLinkButton*>(widget)) {
        command->setText(QString::fromUtf8("执行工艺复核"));
        command->setDescription(QString::fromUtf8("检查当前工单参数与设备状态"));
    } else if (auto* buttonBox = qobject_cast<QDialogButtonBox*>(widget)) {
        buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    } else if (auto* button = qobject_cast<QAbstractButton*>(widget)) {
        button->setText(widget->property("designSystemClass").toString().section("::", -1));
        if (qobject_cast<QRadioButton*>(button) || qobject_cast<QCheckBox*>(button))
            button->setChecked(true);
    }

    if (auto* lineEdit = qobject_cast<QLineEdit*>(widget)) {
        lineEdit->setPlaceholderText(QString::fromUtf8("输入批次或工单编号"));
        lineEdit->setText("LOT-2026-0930");
    } else if (auto* spin = qobject_cast<QSpinBox*>(widget)) {
        spin->setRange(0, 1000);
        spin->setValue(128);
    } else if (auto* doubleSpin = qobject_cast<QDoubleSpinBox*>(widget)) {
        doubleSpin->setRange(0.0, 100.0);
        doubleSpin->setSuffix(" %");
        doubleSpin->setValue(72.5);
    } else if (auto* slider = qobject_cast<QAbstractSlider*>(widget)) {
        slider->setOrientation(Qt::Horizontal);
        slider->setRange(0, 100);
        slider->setValue(64);
    }

    if (auto* fontCombo = qobject_cast<QFontComboBox*>(widget)) {
        fontCombo->setCurrentFont(QFont("Arial"));
    } else if (auto* combo = qobject_cast<QComboBox*>(widget)) {
        combo->addItems({QString::fromUtf8("自动"), QString::fromUtf8("手动"),
                         QString::fromUtf8("维护")});
    }

    if (auto* dateTime = qobject_cast<QDateTimeEdit*>(widget))
        dateTime->setDateTime(QDateTime::currentDateTime());
    if (auto* key = qobject_cast<QKeySequenceEdit*>(widget))
        key->setKeySequence(QKeySequence("Ctrl+Shift+R"));
    if (auto* label = qobject_cast<QLabel*>(widget))
        label->setText(QString::fromUtf8("设备状态：运行中"));
    if (auto* lcd = qobject_cast<QLCDNumber*>(widget))
        lcd->display(42.7);
    if (auto* progress = qobject_cast<QProgressBar*>(widget)) {
        progress->setRange(0, 100);
        progress->setValue(68);
        progress->setFormat(QString::fromUtf8("工艺进度 %p%"));
    }
    if (auto* browser = qobject_cast<QTextBrowser*>(widget))
        browser->setHtml(QString::fromUtf8("<b>复核结果</b><br>参数已通过规则校验。"));
    else if (auto* edit = qobject_cast<QTextEdit*>(widget))
        edit->setPlainText(QString::fromUtf8("工艺备注：等待设备确认。"));
    if (auto* edit = qobject_cast<QPlainTextEdit*>(widget))
        edit->setPlainText(QString::fromUtf8("ALARM: none\nSTATE: running"));
    if (auto* calendar = qobject_cast<QCalendarWidget*>(widget))
        calendar->setGridVisible(true);
    if (auto* graphics = qobject_cast<QGraphicsView*>(widget)) {
        auto* scene = new QGraphicsScene(graphics);
        scene->addRect(8, 8, 180, 72, QPen(QColor("#6d4aff")), QBrush(QColor("#f1edff")));
        scene->addText(QString::fromUtf8("工艺腔体 ETCH-01"));
        graphics->setScene(scene);
        graphics->setMinimumHeight(130);
    }

    if (auto* list = qobject_cast<QListWidget*>(widget)) {
        list->addItems({"ETCH-01", "CVD-02", "CMP-03"});
    } else if (auto* tree = qobject_cast<QTreeWidget*>(widget)) {
        tree->setHeaderLabels({QString::fromUtf8("节点"), QString::fromUtf8("状态")});
        auto* root = new QTreeWidgetItem(tree, {QString::fromUtf8("设备组"),
                                               QString::fromUtf8("正常")});
        new QTreeWidgetItem(root, {"ETCH-01", QString::fromUtf8("运行")});
        tree->expandAll();
    } else if (auto* table = qobject_cast<QTableWidget*>(widget)) {
        table->setRowCount(3);
        table->setColumnCount(2);
        table->setHorizontalHeaderLabels({QString::fromUtf8("设备"), QString::fromUtf8("状态")});
        for (int row = 0; row < 3; ++row) {
            table->setItem(row, 0, new QTableWidgetItem(QString("EQ-%1").arg(row + 1)));
            table->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8("正常")));
        }
        table->horizontalHeader()->setStretchLastSection(true);
    } else if (auto* undo = qobject_cast<QUndoView*>(widget)) {
        auto* stack = new QUndoStack(undo);
        stack->push(new QUndoCommand(QString::fromUtf8("修改温度")));
        stack->push(new QUndoCommand(QString::fromUtf8("更新压力")));
        undo->setStack(stack);
    } else if (auto* itemView = qobject_cast<QAbstractItemView*>(widget)) {
        populateItemModel(itemView);
    }

    if (auto* group = qobject_cast<QGroupBox*>(widget)) {
        group->setTitle(QString::fromUtf8("设备联锁"));
        auto* layout = new QVBoxLayout(group);
        layout->addWidget(new QCheckBox(QString::fromUtf8("启用联锁"), group));
    } else if (auto* area = qobject_cast<QScrollArea*>(widget)) {
        auto* content = new QLabel(QString::fromUtf8("可滚动内容\n参数 1\n参数 2\n参数 3"), area);
        content->setMinimumHeight(140);
        area->setWidget(content);
        area->setWidgetResizable(true);
    } else if (auto* tabs = qobject_cast<QTabWidget*>(widget)) {
        tabs->addTab(new QLabel(QString::fromUtf8("概览"), tabs), QString::fromUtf8("概览"));
        tabs->addTab(new QLabel(QString::fromUtf8("详情"), tabs), QString::fromUtf8("详情"));
    } else if (auto* box = qobject_cast<QToolBox*>(widget)) {
        box->addItem(new QLabel(QString::fromUtf8("基础参数"), box), QString::fromUtf8("参数"));
        box->addItem(new QLabel(QString::fromUtf8("告警规则"), box), QString::fromUtf8("规则"));
    } else if (auto* stack = qobject_cast<QStackedWidget*>(widget)) {
        stack->addWidget(new QLabel(QString::fromUtf8("步骤 1：读取工单"), stack));
        stack->addWidget(new QLabel(QString::fromUtf8("步骤 2：确认复核"), stack));
    } else if (auto* splitter = qobject_cast<QSplitter*>(widget)) {
        splitter->setOrientation(Qt::Horizontal);
        splitter->addWidget(new QLabel(QString::fromUtf8("导航"), splitter));
        splitter->addWidget(new QLabel(QString::fromUtf8("内容"), splitter));
    } else if (auto* dock = qobject_cast<QDockWidget*>(widget)) {
        dock->setWindowTitle(QString::fromUtf8("设备详情"));
        dock->setWidget(new QLabel("ETCH-01", dock));
    } else if (auto* mdi = qobject_cast<QMdiArea*>(widget)) {
        auto* child = mdi->addSubWindow(new QLabel(QString::fromUtf8("趋势窗口"), mdi));
        child->show();
    } else if (auto* sub = qobject_cast<QMdiSubWindow*>(widget)) {
        sub->setWidget(new QLabel(QString::fromUtf8("子窗口内容"), sub));
    } else if (auto* wizard = qobject_cast<QWizard*>(widget)) {
        wizard->setWizardStyle(QWizard::ClassicStyle);
        auto* page = new QWizardPage(wizard);
        page->setTitle(QString::fromUtf8("配置设备"));
        wizard->addPage(page);
    } else if (auto* page = qobject_cast<QWizardPage*>(widget)) {
        page->setTitle(QString::fromUtf8("确认参数"));
        auto* layout = new QVBoxLayout(page);
        layout->addWidget(new QLabel(QString::fromUtf8("压力：1.2 Pa"), page));
    }

    if (auto* menu = qobject_cast<QMenu*>(widget)) {
        menu->setTitle(QString::fromUtf8("操作"));
        menu->addAction(QString::fromUtf8("新建工单"));
        menu->addAction(QString::fromUtf8("导出报告"));
    } else if (auto* menuBar = qobject_cast<QMenuBar*>(widget)) {
        menuBar->addMenu(QString::fromUtf8("文件"))->addAction(QString::fromUtf8("打开"));
    } else if (auto* toolBar = qobject_cast<QToolBar*>(widget)) {
        toolBar->addAction(QString::fromUtf8("刷新"));
        toolBar->addAction(QString::fromUtf8("停止"));
    } else if (auto* status = qobject_cast<QStatusBar*>(widget)) {
        status->showMessage(QString::fromUtf8("设备连接正常"));
    }

    if (auto* file = qobject_cast<QFileDialog*>(widget)) {
        file->setOption(QFileDialog::DontUseNativeDialog, true);
        file->setNameFilter(QString::fromUtf8("工单文件 (*.json *.xml)"));
    } else if (auto* color = qobject_cast<QColorDialog*>(widget)) {
        color->setOption(QColorDialog::DontUseNativeDialog, true);
        color->setCurrentColor(QColor("#6d4aff"));
    } else if (auto* font = qobject_cast<QFontDialog*>(widget)) {
        font->setOption(QFontDialog::DontUseNativeDialog, true);
    } else if (auto* message = qobject_cast<QMessageBox*>(widget)) {
        message->setIcon(QMessageBox::Information);
        message->setText(QString::fromUtf8("工单复核完成"));
        message->setStandardButtons(QMessageBox::Ok);
    } else if (auto* input = qobject_cast<QInputDialog*>(widget)) {
        input->setLabelText(QString::fromUtf8("输入工单号"));
        input->setTextValue("WO-2026-0930");
    } else if (auto* progressDialog = qobject_cast<QProgressDialog*>(widget)) {
        progressDialog->setLabelText(QString::fromUtf8("正在校验参数"));
        progressDialog->setRange(0, 100);
        progressDialog->setValue(64);
    } else if (auto* dialog = qobject_cast<QDialog*>(widget)) {
        dialog->setWindowTitle(qtClass);
        if (!dialog->layout()) {
            auto* layout = new QVBoxLayout(dialog);
            layout->addWidget(new QLabel(QString::fromUtf8("对话框内容区域"), dialog));
        }
    }

    widget->setMinimumWidth(180);
    widget->setMaximumHeight(qtClass.endsWith("Dialog") || qtClass == "QWizard" ? 250 : 210);
}

void appendObjectTree(QTreeWidgetItem* parent, QWidget* widget) {
    auto* item = new QTreeWidgetItem(parent,
        {widget->objectName().isEmpty() ? QString::fromLatin1(widget->metaObject()->className())
                                        : widget->objectName(),
         QString::fromLatin1(widget->metaObject()->className())});
    item->setToolTip(0, item->text(0));
    item->setToolTip(1, item->text(1));
    const QRect geometry = widget->geometry();
    new QTreeWidgetItem(item,
        {"geometry", QString("%1,%2 / %3x%4").arg(geometry.x()).arg(geometry.y())
                                                  .arg(geometry.width()).arg(geometry.height())});
    const auto* meta = widget->metaObject();
    for (const auto& name : {"enabled", "visible", "text", "checked", "value",
                             "currentIndex", "placeholderText", "readOnly", "title"}) {
        const int index = meta->indexOfProperty(name);
        if (index >= 0) {
            const auto value = meta->property(index).read(widget);
            new QTreeWidgetItem(item, {QString::fromLatin1(name), value.toString()});
        }
    }
    for (const auto& property : widget->dynamicPropertyNames()) {
        if (!property.startsWith('_')) {
            auto* value = new QTreeWidgetItem(item,
                {QString::fromLatin1(property), widget->property(property).toString()});
            value->setToolTip(0, value->text(0));
            value->setToolTip(1, value->text(1));
        }
    }
    for (auto* child : widget->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
        appendObjectTree(item, child);
}

void watchObjectTree(QWidget* page, QTreeWidget* objectTree, QWidget* preview) {
    auto refresh = [objectTree, preview] {
        const bool initial = objectTree->topLevelItemCount() == 0;
        QSet<QString> expanded;
        std::function<void(QTreeWidgetItem*, const QString&, bool)> visit =
            [&](QTreeWidgetItem* parent, const QString& path, bool restore) {
                for (int i = 0; i < parent->childCount(); ++i) {
                    auto* item = parent->child(i);
                    const QString key = path + "/" + item->text(0);
                    if (restore)
                        item->setExpanded(expanded.contains(key));
                    else if (item->isExpanded())
                        expanded.insert(key);
                    visit(item, key, restore);
                }
            };
        if (!initial)
            visit(objectTree->invisibleRootItem(), {}, false);
        objectTree->clear();
        appendObjectTree(objectTree->invisibleRootItem(), preview);
        if (initial)
            objectTree->expandToDepth(1);
        else
            visit(objectTree->invisibleRootItem(), {}, true);
    };
    refresh();
    auto* timer = new QTimer(page);
    QObject::connect(timer, &QTimer::timeout, page, [page, refresh] {
        if (page->isVisible()) refresh();
    });
    timer->start(400);
}

QWidget* createComponentPage(const NativeComponentDescriptor& descriptor,
                             const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                             QWidget* parent) {
    auto* page = new QWidget(parent);
    page->setObjectName("componentPage_" + descriptor.sdkClass.section("::", -1));
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 8, 8, 8);
    auto* sections = new QTabWidget(page);
    sections->setObjectName("componentSections");
    layout->addWidget(sections);
    auto* appearanceTab = new QWidget(sections);
    auto* appearanceLayout = new QVBoxLayout(appearanceTab);
    appearanceLayout->setContentsMargins(0, 8, 0, 0);
    auto* splitter = new QSplitter(appearanceTab);
    splitter->setObjectName("componentDetailSplitter");
    appearanceLayout->addWidget(splitter);
    sections->addTab(appearanceTab, QString::fromUtf8("外观"));

    auto* settings = new QWidget(splitter);
    settings->setObjectName("componentSettings");
    auto* controls = new QFormLayout(settings);
    settings->setMinimumWidth(210);
    auto* enabled = new QCheckBox(QString::fromUtf8("启用"), settings);
    enabled->setObjectName("componentEnabledControl");
    enabled->setChecked(true);
    controls->addRow(enabled);
    auto* content = new QLineEdit(settings);
    content->setObjectName("componentTextControl");
    controls->addRow(QString::fromUtf8("文本"), content);

    auto* area = new QScrollArea(splitter);
    area->setObjectName("componentPreviewArea");
    area->setWidgetResizable(true);
    area->setFrameShape(QFrame::NoFrame);
    auto* host = new QWidget(area);
    host->setObjectName("componentPreviewHost");
    auto* hostLayout = new QVBoxLayout(host);
    hostLayout->setContentsMargins(24, 20, 24, 20);
    auto* title = new QLabel(descriptor.designComponent.section('/', -1), host);
    title->setObjectName("componentPageTitle");
    auto* mapping = new QLabel(descriptor.sdkClass, host);
    mapping->setObjectName("componentPageMapping");
    mapping->setWordWrap(true);
    hostLayout->addWidget(title);
    hostLayout->addWidget(mapping);
    auto* preview = factory->create(descriptor.sdkClass, host);
    configurePreview(preview, descriptor.qtClass, descriptor.sdkClass);
    if (auto* dialog = qobject_cast<QDialog*>(preview)) {
        auto* open = new QPushButton(QString::fromUtf8("打开") +
                                     descriptor.designComponent.section('/', -1), host);
        open->setObjectName("componentOpenDialog");
        QObject::connect(open, &QPushButton::clicked, dialog, [dialog] { dialog->show(); });
        hostLayout->addWidget(open, 0, Qt::AlignLeft);
    } else {
        hostLayout->addWidget(preview);
    }
    hostLayout->addStretch(1);
    area->setWidget(host);

    auto* objectTree = new QTreeWidget(splitter);
    objectTree->setObjectName("componentObjectTree");
    objectTree->setHeaderLabels({QString::fromUtf8("节点 / 属性"),
                                 QString::fromUtf8("类型 / 值")});
    objectTree->setMinimumWidth(290);
    objectTree->setColumnWidth(0, 145);
    splitter->addWidget(settings);
    splitter->addWidget(area);
    splitter->addWidget(objectTree);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setSizes({220, 500, 330});

    QObject::connect(enabled, &QCheckBox::toggled, preview, &QWidget::setEnabled);
    const int textProperty = preview->metaObject()->indexOfProperty("text");
    const bool hasText = textProperty >= 0 &&
        preview->metaObject()->property(textProperty).isWritable();
    content->setEnabled(hasText);
    if (hasText) {
        content->setText(preview->property("text").toString());
        QObject::connect(content, &QLineEdit::textChanged, preview,
                         [preview](const QString& text) { preview->setProperty("text", text); });
    }
    watchObjectTree(page, objectTree, preview);

    auto* stateTab = new QWidget(sections);
    auto* stateLayout = new QVBoxLayout(stateTab);
    stateLayout->setContentsMargins(0, 8, 0, 0);
    auto* stateSplitter = new QSplitter(stateTab);
    stateSplitter->setObjectName("componentStateSplitter");
    stateLayout->addWidget(stateSplitter);
    auto* stateSettings = new QWidget(stateSplitter);
    auto* stateControls = new QFormLayout(stateSettings);
    stateSettings->setMinimumWidth(210);
    auto* stateChoice = new QComboBox(stateSettings);
    stateChoice->setObjectName("componentStateControl");
    stateChoice->addItem(QString::fromUtf8("默认"), "default");
    stateChoice->addItem(QString::fromUtf8("禁用"), "disabled");
    stateControls->addRow(QString::fromUtf8("状态"), stateChoice);

    auto* stateArea = new QScrollArea(stateSplitter);
    stateArea->setWidgetResizable(true);
    stateArea->setFrameShape(QFrame::NoFrame);
    auto* stateHost = new QWidget(stateArea);
    auto* stateHostLayout = new QVBoxLayout(stateHost);
    stateHostLayout->setContentsMargins(24, 20, 24, 20);
    stateHostLayout->addWidget(new QLabel(descriptor.designComponent.section('/', -1), stateHost));
    auto* statePreview = factory->create(descriptor.sdkClass, stateHost);
    configurePreview(statePreview, descriptor.qtClass, descriptor.sdkClass);
    statePreview->setObjectName("statePreview_" + descriptor.sdkClass.section("::", -1));
    if (auto* dialog = qobject_cast<QDialog*>(statePreview)) {
        auto* openState = new QPushButton(QString::fromUtf8("打开") +
                                          descriptor.designComponent.section('/', -1), stateHost);
        openState->setObjectName("componentOpenStateDialog");
        QObject::connect(openState, &QPushButton::clicked, dialog, [dialog] { dialog->show(); });
        stateHostLayout->addWidget(openState, 0, Qt::AlignLeft);
        QObject::connect(stateChoice, qOverload<int>(&QComboBox::currentIndexChanged), openState,
                         [stateChoice, openState] { openState->setEnabled(stateChoice->currentIndex() == 0); });
    } else {
        stateHostLayout->addWidget(statePreview);
    }
    stateHostLayout->addStretch(1);
    stateArea->setWidget(stateHost);

    auto* stateTree = new QTreeWidget(stateSplitter);
    stateTree->setObjectName("componentStateObjectTree");
    stateTree->setHeaderLabels({QString::fromUtf8("节点 / 属性"),
                                QString::fromUtf8("类型 / 值")});
    stateTree->setMinimumWidth(290);
    stateTree->setColumnWidth(0, 145);
    stateSplitter->addWidget(stateSettings);
    stateSplitter->addWidget(stateArea);
    stateSplitter->addWidget(stateTree);
    stateSplitter->setStretchFactor(1, 1);
    stateSplitter->setSizes({220, 500, 330});
    QObject::connect(stateChoice, qOverload<int>(&QComboBox::currentIndexChanged), statePreview,
                     [stateChoice, statePreview] { statePreview->setEnabled(stateChoice->currentIndex() == 0); });
    watchObjectTree(page, stateTree, statePreview);
    sections->addTab(stateTab, QString::fromUtf8("分组与状态"));
    return page;
}

} // namespace

QWidget* createGallery(const QString& profile, const QMap<QString, QWidget*>& detailedPages,
                       QWidget* parent) {
    auto* root = new QWidget(parent);
    root->setObjectName("nativeGalleryPage");
    const auto descriptors = visualDescriptors();
    const auto groups = categories();
    root->setProperty("descriptorCount", descriptors.size());
    root->setProperty("componentCount", descriptors.size() + 3);
    root->setProperty("categoryCount", groups.size());

    auto* layout = new QHBoxLayout(root);
    auto* componentTree = new QTreeWidget(root);
    componentTree->setObjectName("componentTree");
    componentTree->setHeaderHidden(true);
    componentTree->setMinimumWidth(210);
    componentTree->setMaximumWidth(260);
    layout->addWidget(componentTree);

    auto* pages = new QStackedWidget(root);
    pages->setObjectName("componentPages");
    layout->addWidget(pages, 1);

    QMap<QString, NativeComponentDescriptor> byClass;
    for (const auto& descriptor : commonWidget::ComponentFactory::descriptors())
        byClass.insert(descriptor.sdkClass, descriptor);
    QList<NativeComponentDescriptor> entries = descriptors;
    for (const auto& sdkClass : {"commonWidget::Z_InputField", "commonWidget::Z_Select",
                                 "commonWidget::Z_OtpInput"})
        entries.append(byClass.value(sdkClass));
    const QStringList first{"commonWidget::Z_PushButton", "commonWidget::Z_CheckBox",
                            "commonWidget::Z_InputField", "commonWidget::Z_Select"};
    std::sort(entries.begin(), entries.end(), [&first](const auto& left, const auto& right) {
        const int leftRank = first.indexOf(left.sdkClass);
        const int rightRank = first.indexOf(right.sdkClass);
        if (leftRank >= 0 || rightRank >= 0)
            return (leftRank < 0 ? first.size() : leftRank) <
                   (rightRank < 0 ? first.size() : rightRank);
        return left.designComponent < right.designComponent;
    });
    for (const auto& group : groups) {
        auto* branch = new QTreeWidgetItem(componentTree, {group.label});
        branch->setData(0, Qt::UserRole, group.id);
        for (const auto& descriptor : entries) {
            if (categoryId(descriptor.qtClass) != group.id)
                continue;
            auto* leaf = new QTreeWidgetItem(branch,
                {descriptor.designComponent.section('/', -1)});
            leaf->setData(0, Qt::UserRole, descriptor.sdkClass);
        }
        branch->setText(0, QString("%1 (%2)").arg(group.label).arg(branch->childCount()));
        branch->setExpanded(group.id == "foundation");
    }

    auto factory = std::make_shared<commonWidget::ComponentFactory>(profile);
    auto pageByClass = std::make_shared<QMap<QString, QWidget*>>();
    QObject::connect(componentTree, &QTreeWidget::currentItemChanged, root,
                     [root, pages, byClass, factory, detailedPages, pageByClass](QTreeWidgetItem* current) {
        if (!current || !current->parent())
            return;
        const QString sdkClass = current->data(0, Qt::UserRole).toString();
        QWidget* page = pageByClass->value(sdkClass, nullptr);
        if (!page) {
            page = detailedPages.value(sdkClass, nullptr);
            if (!page)
                page = createComponentPage(byClass.value(sdkClass), factory, pages);
            pages->addWidget(page);
            pageByClass->insert(sdkClass, page);
        }
        pages->setCurrentWidget(page);
        root->setProperty("activeComponent", sdkClass);
        root->setProperty("visibleDescriptorCount", 1);
    });
    auto* firstGroup = componentTree->topLevelItem(0);
    componentTree->setCurrentItem(firstGroup->child(0));
    return root;
}

QWidget* createRegistry(QWidget* parent) {
    auto* root = new QWidget(parent);
    root->setObjectName("nativeRegistryPage");
    auto* layout = new QVBoxLayout(root);
    auto* summary = new QLabel(root);
    summary->setWordWrap(true);
    summary->setText(QString::fromUtf8(
        "Z-DesignSystem 组件与 Qt 基类的对应关系。组件由 GeneratedComponentSDK 提供。"));
    layout->addWidget(summary);

    const auto descriptors = commonWidget::ComponentFactory::descriptors();
    auto* table = new QTableWidget(descriptors.size(), 4, root);
    table->setObjectName("nativeRegistryTable");
    table->setHorizontalHeaderLabels({QString::fromUtf8("Z-DesignSystem 组件"),
                                      QString::fromUtf8("设计组件"), QString::fromUtf8("Qt 基类"),
                                      QString::fromUtf8("分类")});
    for (int row = 0; row < descriptors.size(); ++row) {
        const auto& descriptor = descriptors[row];
        QString categoryLabel;
        for (const auto& category : categories()) {
            if (category.id == categoryId(descriptor.qtClass)) {
                categoryLabel = category.label;
                break;
            }
        }
        table->setItem(row, 0, new QTableWidgetItem(descriptor.sdkClass));
        table->setItem(row, 1, new QTableWidgetItem(descriptor.designComponent));
        table->setItem(row, 2, new QTableWidgetItem(
            descriptor.qtClass == "semantic" ? QString::fromUtf8("复合组件")
                                             : descriptor.qtClass));
        table->setItem(row, 3, new QTableWidgetItem(categoryLabel));
    }
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSortingEnabled(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(table, 1);
    return root;
}

} // namespace native_gallery
