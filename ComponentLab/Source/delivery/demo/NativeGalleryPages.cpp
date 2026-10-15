#include "NativeGalleryPages.h"
#include "NavigationGallery.h"
#include "CheckboxGallery.h"
#include "ButtonGallery.h"
#include "DisplayGallery.h"
#include "ActionGallery.h"
#include "TabsGallery.h"
#include "ToolBoxGallery.h"
#include "DockWidgetGallery.h"
#include "MdiGallery.h"
#include "MdiAppearance.h"
#include <QTabBar>
#include "LabelAppearance.h"
#include "ProgressAppearance.h"
#include "RadioAppearance.h"
#include "SelectionGallery.h"
#include "InputGallery.h"
#include "PageFlowGallery.h"
#include "CompositeGallery.h"
#include "DataViewsGallery.h"
#include "WindowsGallery.h"
#include "ContainersGallery.h"
#include "NativeDisplayGallery.h"
#include "EvidenceGallery.h"
#include "MyDesignSystem.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractItemDelegate>
#include <QFontDatabase>
#include <QAbstractSlider>
#include <QButtonGroup>
#include <QBoxLayout>
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
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLCDNumber>
#include <QLineEdit>
#include <QIntValidator>
#include <QRegularExpressionValidator>
#include <QListView>
#include <QListWidget>
#include <QPainter>
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
#include <QStyle>
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
#include <QSignalBlocker>
#include <QWizard>
#include <QWizardPage>

#include <memory>
#include <functional>
#include <algorithm>

namespace native_gallery {
namespace {

using commonWidget::DesignComponentDescriptor;
using commonWidget::DesignLibraryEntry;
using commonWidget::DesignComponentPage;
using commonWidget::DesignComponentSet;

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

class PreviewLayoutWatcher final : public QObject {
public:
    PreviewLayoutWatcher(QWidget* preview,const std::function<void()>& refresh)
        :QObject(preview),refresh_(refresh) {preview->installEventFilter(this);}
protected:
    bool eventFilter(QObject*,QEvent* event) override {
        if(!queued_ && (event->type()==QEvent::Show || event->type()==QEvent::Hide ||
                        event->type()==QEvent::Resize || event->type()==QEvent::Move)) {
            queued_=true;
            QTimer::singleShot(0,this,[this]{queued_=false;refresh_();});
        }
        return false;
    }
private:
    std::function<void()> refresh_;
    bool queued_=false;
};

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
    new PreviewLayoutWatcher(preview,refresh);
    auto* timer = new QTimer(page);
    QObject::connect(timer, &QTimer::timeout, page, [page, refresh] {
        if (page->isVisible()) refresh();
    });
    timer->start(400);
}

void applyVariant(QWidget* widget, const DesignComponentSet& componentSet, const QString& variant);

void configureDialogActions(QDialogButtonBox* box,const QString& configuration) {
    if(box->property("dialogConfiguration")==configuration)return;
    box->clear();
    if(configuration=="Apply")box->setStandardButtons(QDialogButtonBox::Reset|QDialogButtonBox::Cancel|QDialogButtonBox::Apply);
    else if(configuration=="Help")box->setStandardButtons(QDialogButtonBox::Help|QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    else if(configuration=="Destructive") {
        box->setStandardButtons(QDialogButtonBox::Cancel);
        auto* action=box->addButton(QString::fromUtf8("Delete"),QDialogButtonBox::DestructiveRole);
        action->setObjectName("dialogDeleteAction");
        // Demo safety policy, not a Figma fact: Delete is never the default action.
        action->setAutoDefault(false);action->setDefault(false);
        box->button(QDialogButtonBox::Cancel)->setDefault(true);
    } else box->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    box->setProperty("dialogConfiguration",configuration);
    commonWidget::applyFigmaDialogButtonBoxAppearance(box);
}

QWidget* createNativeAppearance(const DesignComponentDescriptor& descriptor,
                             const DesignComponentSet& componentSet,
                             const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                             QWidget* parent) {
    auto* page = new QWidget(parent);
    page->setObjectName("componentPage_" + descriptor.sdkClass.section("::", -1));
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 8, 8, 8);
    auto* appearanceTab = new QWidget(page);
    auto* appearanceLayout = new QVBoxLayout(appearanceTab);
    appearanceLayout->setContentsMargins(0, 8, 0, 0);
    auto* detailViewport = new QScrollArea(appearanceTab);
    detailViewport->setObjectName("componentDetailViewport");
    detailViewport->setFrameShape(QFrame::NoFrame);
    detailViewport->setWidgetResizable(true);
    auto* splitter = new QSplitter(detailViewport);
    splitter->setObjectName("componentDetailSplitter");
    detailViewport->setWidget(splitter);
    appearanceLayout->addWidget(detailViewport);
    layout->addWidget(appearanceTab);

    auto* settingsScroll = new QScrollArea(splitter);
    settingsScroll->setObjectName("componentSettingsScroll");
    settingsScroll->setWidgetResizable(true);
    settingsScroll->setFrameShape(QFrame::NoFrame);
    settingsScroll->setMinimumWidth(230);
    settingsScroll->setMaximumWidth(340);
    auto* settings = new QWidget(settingsScroll);
    settings->setObjectName("componentSettings");
    auto* controls = new QFormLayout(settings);
    controls->setRowWrapPolicy(QFormLayout::WrapLongRows);
    controls->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    settingsScroll->setWidget(settings);
    auto* variant = new QComboBox(settings);
    variant->setObjectName("componentVariantControl");
    variant->addItems(componentSet.variants);
    variant->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    variant->setMinimumContentsLength(18);
    variant->view()->setMinimumWidth(520);
    variant->setToolTip(variant->currentText());
    QObject::connect(variant,&QComboBox::currentTextChanged,variant,&QWidget::setToolTip);
    controls->addRow("Figma Variant",variant);
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
    area->setMinimumWidth(320);
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
    const bool pageFlow = componentSet.id=="516:62" || componentSet.id=="516:113" || componentSet.id=="516:669";
    auto* preview = pageFlow ? createPageFlowPreview(descriptor.sdkClass, factory, host)
                             : factory->create(descriptor.sdkClass, host);
    if(!pageFlow)
        configurePreview(preview, descriptor.qtClass, descriptor.sdkClass);
    if(componentSet.id=="414:39")preview->setProperty("text",QString::fromUtf8("设备状态：运行中"));
    preview->setObjectName("componentInteractivePreview");
    if (auto* dialog = qobject_cast<QDialog*>(preview); dialog && !pageFlow) {
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
    objectTree->setMinimumWidth(230);
    objectTree->setColumnWidth(0, 115);
    splitter->addWidget(settingsScroll);
    splitter->addWidget(area);
    splitter->addWidget(objectTree);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setSizes({230, 400, 270});

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
    if(auto* dock=qobject_cast<QDockWidget*>(preview);dock&&componentSet.id=="507:443")
        configureDockWidgetEditor(dock,controls,content,hostLayout,settings,variant);
    if(componentSet.id=="511:124"||componentSet.id=="511:280")
        configureMdiEditor(preview,controls,content,hostLayout,settings,variant,factory);
    if(pageFlow)
        configurePageFlowEditor(preview,controls,content,hostLayout,settings,variant,factory);
    if (auto* box=qobject_cast<QToolBox*>(preview);box&&componentSet.id=="505:553") {
        controls->setRowWrapPolicy(QFormLayout::WrapAllRows);controls->setContentsMargins(12,12,12,12);variant->setMinimumContentsLength(1);variant->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        commonWidget::populateFigmaToolBox(box);box->setMinimumSize(300,350);box->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);hostLayout->setStretchFactor(box,1);
        auto* target=new QSpinBox(settings);target->setObjectName("toolBoxTargetControl");target->setRange(0,box->count()-1);controls->addRow(QString::fromUtf8("编辑的条目索引"),target);
        auto* current=new QSpinBox(settings);current->setObjectName("toolBoxCurrentControl");current->setRange(0,box->count()-1);controls->addRow("currentIndex",current);
        auto* itemEnabled=new QCheckBox("itemEnabled(index)",settings);itemEnabled->setObjectName("toolBoxEnabledControl");controls->addRow(itemEnabled);
        auto* icon=new QCheckBox("Show icon",settings);icon->setObjectName("toolBoxIconControl");controls->addRow(icon);
        auto* hint=new QCheckBox("Show mnemonic",settings);hint->setObjectName("toolBoxMnemonicControl");hint->setChecked(true);controls->addRow(hint);
        auto* tooltip=new QLineEdit(settings);tooltip->setObjectName("toolBoxTooltipControl");controls->addRow("itemToolTip",tooltip);
        auto* keyboard=new QCheckBox(QString::fromUtf8("允许条目键盘聚焦"),settings);keyboard->setObjectName("toolBoxKeyboardControl");controls->addRow(keyboard);
        auto* rtl=new QCheckBox("Right to left",settings);rtl->setObjectName("toolBoxRtlControl");controls->addRow(rtl);
        auto* state=new QComboBox(settings);state->setObjectName("toolBoxSnapshotControl");state->addItem(QString::fromUtf8("跟随真实交互"),QString());for(const auto* s:{"Default","Hover","Selected","Focus","Disabled"})state->addItem(s,s);state->setMinimumContentsLength(1);state->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);controls->addRow(QString::fromUtf8("目标条目外观预览"),state);
        auto* readout=new QLabel(settings);readout->setObjectName("toolBoxNativeReadout");readout->setWordWrap(true);readout->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);controls->addRow(readout);
        auto* note=new QLabel(QString::fromUtf8("ToolBox 仅显示唯一当前页，不提供多开或全收起。ToolBoxItem 是内部结构。& 字符创建原生助记键（&& 显示字面 &）；Show mnemonic 仅显示快捷键提示。Qt 默认条目 NoFocus，可显式开启键盘聚焦。固定 Selected/Focus 不选择页面或伪造焦点；禁用条目时 Qt 可能切换当前页。removeItem 不删除页面。"),settings);note->setWordWrap(true);controls->addRow(note);
        content->setEnabled(true);if(auto* label=qobject_cast<QLabel*>(controls->labelForField(content)))label->setText("Label (target)");
        const auto sync=[=]{const QSignalBlocker a(content),b(current),c(itemEnabled),d(icon),e(tooltip),f(state);const int i=target->value();content->setText(box->itemText(i));current->setValue(box->currentIndex());itemEnabled->setChecked(box->isItemEnabled(i));icon->setChecked(!box->itemIcon(i).isNull());tooltip->setText(box->itemToolTip(i));state->setCurrentIndex(state->findData(box->property("figmaToolBoxSnapshotState").toString()));readout->setText(QString("count=%1 / currentIndex=%2\ncurrent page=%3\ncurrentChanged=%4").arg(box->count()).arg(box->currentIndex()).arg(box->currentWidget()?box->currentWidget()->objectName():QString()).arg(box->property("toolBoxChanges").toInt()));};
        const auto appearance=[=]{commonWidget::applyFigmaToolBoxAppearance(box,state->currentData().toString(),target->value(),hint->isChecked());sync();};
        QObject::connect(target,qOverload<int>(&QSpinBox::valueChanged),box,[=](int n){box->setProperty("toolBoxDemoTarget",n);appearance();});
        QObject::connect(current,qOverload<int>(&QSpinBox::valueChanged),box,[=](int n){box->setCurrentIndex(n);sync();});
        QObject::connect(content,&QLineEdit::textChanged,box,[=](const QString& text){box->setItemText(target->value(),text);});
        QObject::connect(tooltip,&QLineEdit::textChanged,box,[=](const QString& text){box->setItemToolTip(target->value(),text);});
        QObject::connect(itemEnabled,&QCheckBox::toggled,box,[=](bool on){box->setItemEnabled(target->value(),on);sync();});
        QObject::connect(icon,&QCheckBox::toggled,box,[=](bool on){box->setItemIcon(target->value(),on?commonWidget::figmaToolBoxSettingsIcon(true):QIcon());});
        QObject::connect(hint,&QCheckBox::toggled,box,[=]{appearance();});QObject::connect(state,qOverload<int>(&QComboBox::currentIndexChanged),box,[=]{appearance();});
        QObject::connect(keyboard,&QCheckBox::toggled,box,[=](bool on){for(auto* button:box->findChildren<QAbstractButton*>("qt_toolbox_toolboxbutton",Qt::FindDirectChildrenOnly))button->setFocusPolicy(on?Qt::StrongFocus:Qt::NoFocus);});
        QObject::connect(rtl,&QCheckBox::toggled,box,[=](bool on){box->setLayoutDirection(on?Qt::RightToLeft:Qt::LeftToRight);});
        QObject::connect(box,&QToolBox::currentChanged,box,[=]{box->setProperty("toolBoxChanges",box->property("toolBoxChanges").toInt()+1);sync();});
        QObject::connect(variant,&QComboBox::currentTextChanged,box,[=]{QTimer::singleShot(0,box,sync);});appearance();
    }
    if (auto* tabs = qobject_cast<QTabWidget*>(preview); tabs && componentSet.id == "500:847") {
        controls->setRowWrapPolicy(QFormLayout::WrapAllRows);
        controls->setContentsMargins(12,12,12,12);
        variant->setMinimumContentsLength(1);variant->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        commonWidget::populateFigmaTabWidget(tabs);
        tabs->setMinimumSize(300,300);
        tabs->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        hostLayout->setStretchFactor(tabs,1);
        auto* target=new QSpinBox(settings);target->setObjectName("tabTargetControl");target->setRange(0,tabs->count()-1);controls->addRow(QString::fromUtf8("编辑的页签索引"),target);
        auto* current=new QSpinBox(settings);current->setObjectName("tabCurrentControl");current->setRange(0,tabs->count()-1);controls->addRow("currentIndex",current);
        auto* position=new QComboBox(settings);position->setObjectName("tabPositionControl");position->addItem("Top",QTabWidget::North);position->addItem("Bottom",QTabWidget::South);position->addItem("Left",QTabWidget::West);position->addItem("Right",QTabWidget::East);controls->addRow("Position",position);
        auto* tabEnabled=new QCheckBox("tabEnabled(index)",settings);tabEnabled->setObjectName("tabEnabledControl");controls->addRow(tabEnabled);
        auto* icon=new QCheckBox("Show icon",settings);icon->setObjectName("tabIconControl");controls->addRow(icon);
        auto* close=new QComboBox(settings);close->setObjectName("tabClosePolicyControl");close->addItems({QString::fromUtf8("仅 Behavior（显式组合）"),QString::fromUtf8("全部可请求关闭"),QString::fromUtf8("全部不可关闭")});controls->addRow("Closable",close);
        auto* movable=new QCheckBox("movable",settings);movable->setObjectName("tabMovableControl");controls->addRow(movable);
        auto* scroll=new QCheckBox("usesScrollButtons",settings);scroll->setObjectName("tabScrollControl");scroll->setChecked(true);tabs->setUsesScrollButtons(true);controls->addRow(scroll);
        auto* elide=new QComboBox(settings);elide->setObjectName("tabElideControl");elide->addItem("None",Qt::ElideNone);elide->addItem("Right",Qt::ElideRight);elide->addItem("Middle",Qt::ElideMiddle);elide->addItem("Left",Qt::ElideLeft);tabs->setElideMode(Qt::ElideNone);controls->addRow("elideMode",elide);
        auto* rtl=new QCheckBox("Right to left",settings);rtl->setObjectName("tabRtlControl");controls->addRow(rtl);
        auto* state=new QComboBox(settings);state->setObjectName("tabSnapshotControl");state->addItem(QString::fromUtf8("跟随真实交互"),QString());for(const auto* s:{"Default","Hover","Selected","Focus","Disabled"})state->addItem(s,s);controls->addRow(QString::fromUtf8("目标页签外观预览"),state);
        for(auto* choice:{position,close,elide,state}){choice->setMinimumContentsLength(1);choice->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);choice->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);choice->view()->setMinimumWidth(260);}
        auto* readout=new QLabel(settings);readout->setObjectName("tabNativeReadout");readout->setWordWrap(true);readout->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);controls->addRow(readout);
        auto* note=new QLabel(QString::fromUtf8("TabItem 是内部结构，不新增公开类型。20 个变体只固定目标页签的外观；currentIndex、焦点和页面所有权仍由 QTabWidget 管理。关闭按钮仅发出 tabCloseRequested 并记录，不自动移除或删除页面。禁用页签与禁用整个组件分开。可拖动页签以验证 tabMoved 与页面顺序。"),settings);note->setWordWrap(true);controls->addRow(note);
        content->setEnabled(true);
        if(auto* label=qobject_cast<QLabel*>(controls->labelForField(content)))label->setText("Label (target)");
        const auto sync=[=]{
            const QSignalBlocker a(content),b(current),c(position),d(tabEnabled),e(icon),f(state);
            const int i=target->value();content->setText(tabs->tabText(i));current->setValue(tabs->currentIndex());position->setCurrentIndex(position->findData(tabs->tabPosition()));tabEnabled->setChecked(tabs->isTabEnabled(i));icon->setChecked(!tabs->tabIcon(i).isNull());state->setCurrentIndex(state->findData(tabs->tabBar()->property("figmaTabSnapshotState").toString()));
            readout->setText(QString("count=%1 / currentIndex=%2\ncurrent page=%3\nclose requests=%4 / last=%5\nmoves=%6").arg(tabs->count()).arg(tabs->currentIndex()).arg(tabs->currentWidget()?tabs->currentWidget()->objectName():QString()).arg(tabs->property("tabCloseRequests").toInt()).arg(tabs->property("tabLastClose").isValid()?tabs->property("tabLastClose").toInt():-1).arg(tabs->property("tabMoveCount").toInt()));
        };
        const auto appearance=[=]{commonWidget::applyFigmaTabWidgetAppearance(tabs,state->currentData().toString(),target->value());sync();};
        QObject::connect(target,qOverload<int>(&QSpinBox::valueChanged),tabs,[=](int n){tabs->setProperty("tabDemoTarget",n);appearance();});
        QObject::connect(current,qOverload<int>(&QSpinBox::valueChanged),tabs,[=](int n){tabs->setCurrentIndex(n);sync();});
        QObject::connect(position,qOverload<int>(&QComboBox::currentIndexChanged),tabs,[=]{tabs->setTabPosition(static_cast<QTabWidget::TabPosition>(position->currentData().toInt()));appearance();});
        QObject::connect(content,&QLineEdit::textChanged,tabs,[=](const QString& text){tabs->setTabText(target->value(),text);});
        QObject::connect(tabEnabled,&QCheckBox::toggled,tabs,[=](bool on){tabs->setTabEnabled(target->value(),on);sync();});
        QObject::connect(icon,&QCheckBox::toggled,tabs,[=](bool on){tabs->setTabIcon(target->value(),on?commonWidget::figmaTabDocumentIcon(true):QIcon());});
        QObject::connect(close,qOverload<int>(&QComboBox::currentIndexChanged),tabs,[=](int n){commonWidget::setFigmaTabClosePolicy(tabs,n!=2,n==0);appearance();});
        QObject::connect(movable,&QCheckBox::toggled,tabs,&QTabWidget::setMovable);
        QObject::connect(scroll,&QCheckBox::toggled,tabs,&QTabWidget::setUsesScrollButtons);
        QObject::connect(elide,qOverload<int>(&QComboBox::currentIndexChanged),tabs,[=]{tabs->setElideMode(static_cast<Qt::TextElideMode>(elide->currentData().toInt()));});
        QObject::connect(rtl,&QCheckBox::toggled,tabs,[=](bool on){tabs->setLayoutDirection(on?Qt::RightToLeft:Qt::LeftToRight);});
        QObject::connect(state,qOverload<int>(&QComboBox::currentIndexChanged),tabs,[=]{appearance();});
        QObject::connect(tabs,&QTabWidget::currentChanged,tabs,[=]{sync();});
        QObject::connect(tabs,&QTabWidget::tabCloseRequested,tabs,[=](int n){tabs->setProperty("tabCloseRequests",tabs->property("tabCloseRequests").toInt()+1);tabs->setProperty("tabLastClose",n);sync();});
        QObject::connect(tabs->tabBar(),&QTabBar::tabMoved,tabs,[=]{tabs->setProperty("tabMoveCount",tabs->property("tabMoveCount").toInt()+1);sync();});
        QObject::connect(variant,&QComboBox::currentTextChanged,tabs,[=]{QTimer::singleShot(0,tabs,sync);});
        appearance();
    }
    if(auto* box=qobject_cast<QDialogButtonBox*>(preview);box&&componentSet.id=="447:593") {
        controls->setRowWrapPolicy(QFormLayout::WrapAllRows);controls->setContentsMargins(12,12,12,12);variant->setMinimumContentsLength(1);variant->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        content->hide();if(auto* label=controls->labelForField(content))label->hide();
        auto* configuration=new QComboBox(settings);configuration->setObjectName("dialogConfigurationControl");configuration->addItems({"Confirm","Apply","Help","Destructive"});controls->addRow("Configuration",configuration);
        auto* orientation=new QComboBox(settings);orientation->setObjectName("dialogOrientationControl");orientation->addItems({"Horizontal","Vertical"});controls->addRow("orientation",orientation);
        auto* centered=new QCheckBox("centerButtons",settings);centered->setObjectName("dialogCenteredControl");controls->addRow(centered);
        auto* rtl=new QCheckBox("Right to left",settings);rtl->setObjectName("dialogRtlControl");controls->addRow(rtl);
        auto* readout=new QLabel(settings);readout->setObjectName("dialogNativeReadout");readout->setWordWrap(true);readout->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);controls->addRow(readout);
        auto* open=new QPushButton(QString::fromUtf8("打开键盘测试对话框"),settings);open->setObjectName("dialogOpenControl");controls->addRow(open);
        auto* note=new QLabel(QString::fromUtf8("32 个 Figma 变体；画廊顺序仅为设计示意。外观保留 QStyle 平台排序、原生 standardButtons、角色与信号、本地化、方向和居中。420×160 不是运行时固定尺寸。Apply、Reset、Delete 只记录 clicked + role，不触发 accepted；Delete 不执行文件操作，也不设为默认按钮。Enter/Escape 在测试对话框中验证，QDialogButtonBox 自身不会关闭窗口。"),settings);note->setWordWrap(true);controls->addRow(note);
        const auto sync=[=]{const QSignalBlocker a(configuration),b(orientation),c(centered);configuration->setCurrentText(box->property("dialogConfiguration").toString());orientation->setCurrentIndex(box->orientation()==Qt::Vertical?1:0);centered->setChecked(box->centerButtons());readout->setText(QString("standardButtons=0x%1\nclicked=%2 / accepted=%3\nrejected=%4 / help=%5\nlast role=%6\nQStyle layout=%7").arg(uint(box->standardButtons()),0,16).arg(box->property("dialogClicks").toInt()).arg(box->property("dialogAccepted").toInt()).arg(box->property("dialogRejected").toInt()).arg(box->property("dialogHelp").toInt()).arg(box->property("dialogLastRole").toInt()).arg(box->style()->styleHint(QStyle::SH_DialogButtonLayout,nullptr,box)));};
        QObject::connect(configuration,&QComboBox::currentTextChanged,box,[=](const QString& value){configureDialogActions(box,value);sync();});
        QObject::connect(orientation,qOverload<int>(&QComboBox::currentIndexChanged),box,[=](int n){box->setOrientation(n?Qt::Vertical:Qt::Horizontal);commonWidget::applyFigmaDialogButtonBoxAppearance(box);sync();});
        QObject::connect(centered,&QCheckBox::toggled,box,[=](bool on){box->setCenterButtons(on);sync();});
        QObject::connect(rtl,&QCheckBox::toggled,box,[=](bool on){box->setLayoutDirection(on?Qt::RightToLeft:Qt::LeftToRight);});
        QObject::connect(box,&QDialogButtonBox::clicked,box,[=](QAbstractButton* action){box->setProperty("dialogClicks",box->property("dialogClicks").toInt()+1);box->setProperty("dialogLastRole",int(box->buttonRole(action)));sync();});
        const auto count=[=](const char* key){box->setProperty(key,box->property(key).toInt()+1);sync();};
        QObject::connect(box,&QDialogButtonBox::accepted,box,[=]{count("dialogAccepted");});QObject::connect(box,&QDialogButtonBox::rejected,box,[=]{count("dialogRejected");});QObject::connect(box,&QDialogButtonBox::helpRequested,box,[=]{count("dialogHelp");});
        QObject::connect(open,&QPushButton::clicked,box,[=]{auto* dialog=new QDialog(page);dialog->setObjectName("dialogKeyboardTest");dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowTitle(QString::fromUtf8("原生对话框键盘测试（无实际操作）"));auto* layout=new QVBoxLayout(dialog);layout->addWidget(new QLabel(QString::fromUtf8("Enter 使用原生默认按钮；Escape 取消。Apply / Reset / Delete 仅作演示。"),dialog));auto* actions=new commonWidget::Z_DialogButtonBox(dialog);configureDialogActions(actions,configuration->currentText());actions->setOrientation(box->orientation());actions->setCenterButtons(box->centerButtons());actions->setLayoutDirection(box->layoutDirection());actions->setEnabled(box->isEnabled());commonWidget::applyFigmaDialogButtonBoxAppearance(actions);layout->addWidget(actions);QObject::connect(actions,&QDialogButtonBox::accepted,dialog,&QDialog::accept);QObject::connect(actions,&QDialogButtonBox::rejected,dialog,&QDialog::reject);dialog->show();});
        QObject::connect(variant,&QComboBox::currentTextChanged,box,[=]{QTimer::singleShot(0,box,sync);});configureDialogActions(box,"Confirm");sync();
    }
    if(auto* tool=qobject_cast<QToolButton*>(preview);tool&&componentSet.id=="443:222") {
        controls->setRowWrapPolicy(QFormLayout::WrapAllRows);controls->setContentsMargins(12,12,12,12);variant->setMinimumContentsLength(1);variant->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        tool->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);hostLayout->setAlignment(tool,Qt::AlignLeft);
        tool->setStyleSheet("QToolButton { min-height:0px; }");
        QObject::disconnect(content,nullptr,preview,nullptr);QObject::disconnect(enabled,&QCheckBox::toggled,preview,&QWidget::setEnabled);
        content->setText("Format");tool->setText(content->text());tool->setIcon(commonWidget::figmaToolButtonIcon());tool->setIconSize(QSize(16,16));tool->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        auto* action=new QAction(tool);action->setObjectName("toolDemoAction");
        auto* menu=new QMenu(tool);menu->setObjectName("toolDemoMenu");menu->addAction("Format document");menu->addAction("Format selection");
        auto* actionSource=new QCheckBox("Use defaultAction",settings);actionSource->setObjectName("toolActionSourceControl");controls->addRow(actionSource);
        auto* menuAttached=new QCheckBox("Attach QMenu",settings);menuAttached->setObjectName("toolMenuAttachedControl");menuAttached->setChecked(true);tool->setMenu(menu);controls->addRow(menuAttached);
        auto* popup=new QComboBox(settings);popup->setObjectName("toolPopupControl");popup->addItem("DelayedPopup",QToolButton::DelayedPopup);popup->addItem("MenuButtonPopup",QToolButton::MenuButtonPopup);popup->addItem("InstantPopup",QToolButton::InstantPopup);controls->addRow("popupMode",popup);
        auto* style=new QComboBox(settings);style->setObjectName("toolStyleControl");for(const auto& item:QList<QPair<QString,int>>{{"Icon only",Qt::ToolButtonIconOnly},{"Text only",Qt::ToolButtonTextOnly},{"Text beside icon",Qt::ToolButtonTextBesideIcon},{"Text under icon",Qt::ToolButtonTextUnderIcon},{"Follow style",Qt::ToolButtonFollowStyle}})style->addItem(item.first,item.second);style->setCurrentIndex(2);controls->addRow("toolButtonStyle",style);
        auto* iconSize=new QSpinBox(settings);iconSize->setObjectName("toolIconSizeControl");iconSize->setRange(8,64);iconSize->setValue(16);controls->addRow("iconSize",iconSize);
        auto* autoRaise=new QCheckBox("autoRaise",settings);autoRaise->setObjectName("toolAutoRaiseControl");controls->addRow(autoRaise);
        auto* checkable=new QCheckBox("checkable",settings);checkable->setObjectName("toolCheckableControl");controls->addRow(checkable);
        auto* checked=new QCheckBox("checked",settings);checked->setObjectName("toolCheckedControl");controls->addRow(checked);
        auto* state=new QComboBox(settings);state->setObjectName("toolSnapshotControl");state->addItem(QString::fromUtf8("跟随真实交互"),QString());for(const auto* name:{"Default","Hover","Pressed","Focus","Checked","CheckedHover","CheckedFocus","Open"})state->addItem(name,name);state->setCurrentIndex(1);controls->addRow(QString::fromUtf8("状态预览"),state);
        auto* priority=new QComboBox(settings);priority->setObjectName("toolPriorityControl");priority->addItem("Normal",QAction::NormalPriority);priority->addItem("Low",QAction::LowPriority);priority->addItem("High",QAction::HighPriority);controls->addRow("QAction priority",priority);
        auto* rtl=new QCheckBox("Right to left",settings);rtl->setObjectName("toolRtlControl");controls->addRow(rtl);
        auto* open=new QPushButton(QString::fromUtf8("打开菜单"),settings);open->setObjectName("toolOpenMenuControl");controls->addRow(open);
        auto* readout=new QLabel(settings);readout->setObjectName("toolNativeReadout");readout->setWordWrap(true);readout->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);controls->addRow(readout);
        auto* note=new QLabel(QString::fromUtf8("Menu=None 仅表示无分割槽，不禁止 QMenu；DelayedPopup 保留原生长按，InstantPopup 不触发主命令。Open 仅适用于 Split，固定预览不真正打开菜单。绑定 defaultAction 后，文本、启用和勾选统一修改 QAction；Low priority 可由 Qt 隐藏并排文本。Show label 对应编辑原生显示模式，不是新的 Qt 属性。源稿图标槽实际 18px，原始 SVG 16px；旧说明的 20px 未覆盖图层事实。其余原生显示模式不冒充新增 Figma 变体。"),settings);note->setWordWrap(true);controls->addRow(note);
        const auto sync=[=]{const QSignalBlocker a(content),b(enabled),c(autoRaise),d(checkable),e(checked),f(popup),g(style),h(state);content->setText(tool->text());enabled->setChecked(tool->isEnabled());autoRaise->setChecked(tool->autoRaise());checkable->setChecked(tool->isCheckable());checked->setChecked(tool->isChecked());checked->setEnabled(tool->isCheckable());popup->setCurrentIndex(popup->findData(tool->popupMode()));style->setCurrentIndex(style->findData(tool->toolButtonStyle()));state->setCurrentIndex(state->findData(tool->property("figmaToolSnapshotState").toString()));priority->setEnabled(tool->defaultAction());open->setEnabled(tool->isEnabled()&&menuAttached->isChecked());readout->setText(QString("source=%1\nchecked=%2 / down=%3\nclicks=%4 / triggered=%5\nlast action:\n%6").arg(tool->defaultAction()?"QAction":"QToolButton").arg(tool->isChecked()).arg(tool->isDown()).arg(tool->property("toolClicks").toInt()).arg(tool->property("toolTriggers").toInt()).arg(tool->property("toolLastAction").toString()));};
        const auto appearance=[=]{auto fixed=state->currentData().toString();if(fixed=="Open"&&tool->popupMode()!=QToolButton::MenuButtonPopup)fixed="";commonWidget::applyFigmaToolButtonAppearance(tool,fixed);tool->setMinimumSize(0,0);sync();};
        QObject::connect(content,&QLineEdit::textChanged,tool,[=](const QString& text){if(tool->defaultAction()){action->setText(text);action->setIconText(text);}else tool->setText(text);});
        QObject::connect(enabled,&QCheckBox::toggled,tool,[=](bool on){if(tool->defaultAction())action->setEnabled(on);else tool->setEnabled(on);sync();});
        QObject::connect(actionSource,&QCheckBox::toggled,tool,[=](bool on){if(on){action->setText(tool->text());action->setIconText(tool->text());action->setIcon(tool->icon());action->setEnabled(tool->isEnabled());action->setCheckable(tool->isCheckable());action->setChecked(tool->isChecked());tool->setDefaultAction(action);}else{tool->setDefaultAction(nullptr);tool->removeAction(action);}appearance();});
        QObject::connect(menuAttached,&QCheckBox::toggled,tool,[=](bool on){tool->setMenu(on?menu:nullptr);appearance();});
        QObject::connect(popup,qOverload<int>(&QComboBox::currentIndexChanged),tool,[=]{tool->setPopupMode(static_cast<QToolButton::ToolButtonPopupMode>(popup->currentData().toInt()));appearance();});
        QObject::connect(style,qOverload<int>(&QComboBox::currentIndexChanged),tool,[=]{tool->setToolButtonStyle(static_cast<Qt::ToolButtonStyle>(style->currentData().toInt()));appearance();});
        QObject::connect(iconSize,qOverload<int>(&QSpinBox::valueChanged),tool,[=](int size){tool->setIconSize(QSize(size,size));appearance();});
        QObject::connect(autoRaise,&QCheckBox::toggled,tool,[=](bool on){tool->setAutoRaise(on);appearance();});
        QObject::connect(checkable,&QCheckBox::toggled,tool,[=](bool on){if(tool->defaultAction())action->setCheckable(on);else tool->setCheckable(on);sync();});
        QObject::connect(checked,&QCheckBox::toggled,tool,[=](bool on){if(tool->defaultAction())action->setChecked(on);else tool->setChecked(on);sync();});
        QObject::connect(priority,qOverload<int>(&QComboBox::currentIndexChanged),tool,[=]{action->setPriority(static_cast<QAction::Priority>(priority->currentData().toInt()));appearance();});
        QObject::connect(state,qOverload<int>(&QComboBox::currentIndexChanged),tool,[=]{appearance();});
        QObject::connect(rtl,&QCheckBox::toggled,tool,[=](bool on){tool->setLayoutDirection(on?Qt::RightToLeft:Qt::LeftToRight);appearance();});
        QObject::connect(open,&QPushButton::clicked,tool,[=]{tool->showMenu();sync();});
        QObject::connect(tool,&QAbstractButton::clicked,tool,[=]{tool->setProperty("toolClicks",tool->property("toolClicks").toInt()+1);sync();});
        QObject::connect(tool,&QToolButton::triggered,tool,[=](QAction* fired){tool->setProperty("toolTriggers",tool->property("toolTriggers").toInt()+1);tool->setProperty("toolLastAction",fired?fired->text():QString());sync();});
        QObject::connect(tool,&QAbstractButton::toggled,tool,[=]{sync();});QObject::connect(action,&QAction::changed,tool,[=]{sync();});
        QObject::connect(variant,&QComboBox::currentTextChanged,tool,[=]{QTimer::singleShot(0,tool,sync);});appearance();
    }
    if(auto* command=qobject_cast<QCommandLinkButton*>(preview);command&&componentSet.id=="450:102") {
        controls->setRowWrapPolicy(QFormLayout::WrapAllRows);controls->setContentsMargins(12,12,12,12);variant->setMinimumContentsLength(1);variant->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        content->setText("Create a new workspace");command->setDescription("Start with an empty workspace and configure it later.");command->setIcon(commonWidget::figmaCommandLinkIcon());command->setIconSize(QSize(16,16));command->setMinimumWidth(360);
        auto* description=new QLineEdit(command->description(),settings);description->setObjectName("commandDescriptionControl");controls->addRow("description",description);
        auto* flat=new QCheckBox("flat",settings);flat->setObjectName("commandFlatControl");controls->addRow(flat);
        auto* checkable=new QCheckBox("checkable",settings);checkable->setObjectName("commandCheckableControl");controls->addRow(checkable);
        auto* checked=new QCheckBox("checked",settings);checked->setObjectName("commandCheckedControl");controls->addRow(checked);
        auto* state=new QComboBox(settings);state->setObjectName("commandSnapshotControl");state->addItem(QString::fromUtf8("跟随真实交互"),QString());
        for(const auto* name:{"Default","Hover","Pressed","Focus","Checked","CheckedHover","CheckedFocus"})state->addItem(name,name);state->setCurrentIndex(1);controls->addRow(QString::fromUtf8("状态预览"),state);
        auto* showDescription=new QCheckBox("Show description",settings);showDescription->setObjectName("commandShowDescriptionControl");showDescription->setChecked(true);controls->addRow(showDescription);
        auto* showIcon=new QCheckBox("Show icon",settings);showIcon->setObjectName("commandShowIconControl");showIcon->setChecked(true);controls->addRow(showIcon);
        auto* automatic=new QCheckBox("autoDefault",settings);automatic->setObjectName("commandAutoDefaultControl");automatic->setChecked(command->autoDefault());controls->addRow(automatic);
        auto* defaultButton=new QCheckBox("default",settings);defaultButton->setObjectName("commandDefaultControl");defaultButton->setChecked(command->isDefault());controls->addRow(defaultButton);
        auto* rtl=new QCheckBox("Right to left",settings);rtl->setObjectName("commandRtlControl");controls->addRow(rtl);
        auto* readout=new QLabel(settings);readout->setObjectName("commandNativeReadout");readout->setWordWrap(true);readout->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);controls->addRow(readout);
        auto* note=new QLabel(QString::fromUtf8("保留原生 QCommandLinkButton 的 text、description、flat、checkable、checked、default 与点击信号。固定 Pressed/Focus 仅为绘制预览，不伪造按下或焦点。Checked 设计变体在真实编辑中设置 checkable/checked；单独状态预览不改写这些属性。没有内置互斥分组或向导跳转。Show description/icon 是适配器显隐，保留原文和 QIcon；图标 16px 放入 24px 源稿槽位。画廊保留原稿内容框 52px 的说明裁切，真实预览按说明换行提供足够高度，此处待设计确认。"),settings);note->setWordWrap(true);controls->addRow(note);
        const auto sync=[=]{const QSignalBlocker a(flat),b(checkable),c(checked),d(state);flat->setChecked(command->isFlat());checkable->setChecked(command->isCheckable());checked->setChecked(command->isChecked());checked->setEnabled(command->isCheckable());state->setCurrentIndex(state->findData(command->property("figmaCommandSnapshotState").toString()));readout->setText(QString("checked=%1 / down=%2\nclicks=%3\nlast signal:\n%4").arg(command->isChecked()).arg(command->isDown()).arg(command->property("commandClicks").toInt()).arg(command->property("commandLastSignal").toString()));};
        const auto appearance=[=]{commonWidget::applyFigmaCommandLinkAppearance(command,state->currentData().toString(),showDescription->isChecked(),showIcon->isChecked());sync();};
        QObject::connect(description,&QLineEdit::textChanged,command,[=](const QString& text){command->setDescription(text);appearance();});
        QObject::connect(flat,&QCheckBox::toggled,command,[=](bool on){command->setFlat(on);appearance();});
        QObject::connect(checkable,&QCheckBox::toggled,command,[=](bool on){command->setCheckable(on);sync();});QObject::connect(checked,&QCheckBox::toggled,command,&QAbstractButton::setChecked);
        QObject::connect(state,qOverload<int>(&QComboBox::currentIndexChanged),command,[=]{appearance();});QObject::connect(showDescription,&QCheckBox::toggled,command,[=]{appearance();});QObject::connect(showIcon,&QCheckBox::toggled,command,[=]{appearance();});
        QObject::connect(automatic,&QCheckBox::toggled,command,&QPushButton::setAutoDefault);QObject::connect(defaultButton,&QCheckBox::toggled,command,&QPushButton::setDefault);
        QObject::connect(rtl,&QCheckBox::toggled,command,[=](bool on){command->setLayoutDirection(on?Qt::RightToLeft:Qt::LeftToRight);appearance();});
        QObject::connect(command,&QAbstractButton::clicked,command,[=]{command->setProperty("commandClicks",command->property("commandClicks").toInt()+1);command->setProperty("commandLastSignal","clicked");sync();});
        QObject::connect(command,&QAbstractButton::toggled,command,[=]{command->setProperty("commandLastSignal","toggled");sync();});
        QObject::connect(variant,&QComboBox::currentTextChanged,command,[=]{QTimer::singleShot(0,command,sync);});appearance();
    }
    if(auto* combo=qobject_cast<QComboBox*>(preview);combo&&(componentSet.id=="1120:18"||componentSet.id=="1121:18")) {
        controls->setRowWrapPolicy(QFormLayout::WrapAllRows);controls->setContentsMargins(12,12,12,12);
        variant->setMinimumContentsLength(1);variant->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        combo->setMinimumWidth(240);auto* fonts=qobject_cast<QFontComboBox*>(combo);
        if(!fonts){combo->setPlaceholderText("Please select");combo->setCurrentIndex(-1);}
        content->setEnabled(combo->isEditable());content->setText(combo->currentText());
        auto* size=new QComboBox(settings);size->setObjectName("comboSizeControl");size->addItems({"Small","Middle","Large"});controls->addRow("Size",size);
        auto* state=new QComboBox(settings);state->setObjectName("comboSnapshotControl");state->addItem(QString::fromUtf8("跟随真实交互"),QString());
        for(const auto* name:{"Default","Hover","Focus"})state->addItem(name,name);state->setCurrentIndex(1);controls->addRow(QString::fromUtf8("状态预览"),state);
        auto* editable=new QCheckBox("editable",settings);editable->setObjectName("comboEditableControl");editable->setChecked(combo->isEditable());controls->addRow(editable);
        auto* current=new QSpinBox(settings);current->setObjectName("comboIndexControl");current->setRange(-1,combo->count()-1);current->setValue(combo->currentIndex());controls->addRow("currentIndex",current);
        auto* placeholder=new QLineEdit("Please select",settings);placeholder->setObjectName("comboPlaceholderControl");controls->addRow("placeholderText",placeholder);
        auto* policy=new QComboBox(settings);policy->setObjectName("comboInsertPolicyControl");policy->addItems({"NoInsert","InsertAtTop","InsertAtCurrent","InsertAtBottom","InsertAfterCurrent","InsertBeforeCurrent","InsertAlphabetically"});policy->setCurrentIndex(int(combo->insertPolicy()));controls->addRow("insertPolicy",policy);
        auto* duplicates=new QCheckBox("duplicatesEnabled",settings);duplicates->setObjectName("comboDuplicatesControl");duplicates->setChecked(combo->duplicatesEnabled());controls->addRow(duplicates);
        auto* visibleItems=new QSpinBox(settings);visibleItems->setObjectName("comboMaxVisibleControl");visibleItems->setRange(1,50);visibleItems->setValue(combo->maxVisibleItems());controls->addRow("maxVisibleItems",visibleItems);
        auto* rtl=new QCheckBox("Right to left",settings);rtl->setObjectName("comboRtlControl");controls->addRow(rtl);
        auto* open=new QPushButton(QString::fromUtf8("打开原生列表"),settings);open->setObjectName("comboOpenControl");controls->addRow(open);
        auto* reset=new QPushButton(QString::fromUtf8("清空当前选择（不删选项）"),settings);reset->setObjectName("comboResetControl");controls->addRow(reset);
        auto* clearEdit=new QPushButton(QString::fromUtf8("清空编辑文本"),settings);clearEdit->setObjectName("comboClearEditControl");clearEdit->setToolTip("QComboBox::clearEditText; retains the option model");controls->addRow(clearEdit);
        auto* readout=new QLabel(settings);readout->setObjectName("comboNativeReadout");readout->setWordWrap(true);readout->setMinimumWidth(0);readout->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);controls->addRow(readout);
        const auto sync=[=]{const QSignalBlocker a(current),b(content),c(size),d(state);current->setRange(-1,combo->count()-1);current->setValue(combo->currentIndex());
            if(!content->hasFocus())content->setText(combo->currentText());content->setEnabled(combo->isEditable());clearEdit->setEnabled(combo->isEditable());
            size->setCurrentText(combo->property("figmaComboSize").toString());state->setCurrentIndex(state->findData(combo->property("figmaComboSnapshotState").toString()));
            readout->setText(QString("count=%1 / index=%2\ncurrentText:\n%3\nmodel:\n%4\ndelegate:\n%5\nlast signal:\n%6%7").arg(combo->count()).arg(combo->currentIndex()).arg(combo->currentText(),combo->model()->metaObject()->className(),combo->itemDelegate()->metaObject()->className(),combo->property("comboLastSignal").toString(),fonts?"\ncurrentFont:\n"+fonts->currentFont().family():QString()));};
        const auto appearance=[=]{commonWidget::applyFigmaComboBoxAppearance(combo,size->currentText(),state->currentData().toString());sync();};
        QObject::connect(size,qOverload<int>(&QComboBox::currentIndexChanged),combo,[=]{appearance();});QObject::connect(state,qOverload<int>(&QComboBox::currentIndexChanged),combo,[=]{appearance();});
        QObject::connect(editable,&QCheckBox::toggled,combo,[=](bool on){combo->setEditable(on);if(combo->lineEdit())combo->lineEdit()->setPlaceholderText(placeholder->text());appearance();});
        QObject::connect(current,qOverload<int>(&QSpinBox::valueChanged),combo,&QComboBox::setCurrentIndex);
        QObject::connect(content,&QLineEdit::textEdited,combo,&QComboBox::setEditText);
        QObject::connect(placeholder,&QLineEdit::textChanged,combo,[=](const QString& text){combo->setPlaceholderText(text);if(combo->lineEdit())combo->lineEdit()->setPlaceholderText(text);sync();});
        QObject::connect(policy,qOverload<int>(&QComboBox::currentIndexChanged),combo,[=](int i){combo->setInsertPolicy(QComboBox::InsertPolicy(i));});
        QObject::connect(duplicates,&QCheckBox::toggled,combo,&QComboBox::setDuplicatesEnabled);QObject::connect(visibleItems,qOverload<int>(&QSpinBox::valueChanged),combo,&QComboBox::setMaxVisibleItems);
        QObject::connect(rtl,&QCheckBox::toggled,combo,[=](bool on){combo->setLayoutDirection(on?Qt::RightToLeft:Qt::LeftToRight);});
        QObject::connect(open,&QPushButton::clicked,combo,[=]{if(combo->isEnabled()){combo->setFocus();combo->showPopup();}});
        QObject::connect(reset,&QPushButton::clicked,combo,[=]{combo->setCurrentIndex(-1);sync();});QObject::connect(clearEdit,&QPushButton::clicked,combo,[=]{combo->clearEditText();sync();});
        QObject::connect(combo,qOverload<int>(&QComboBox::currentIndexChanged),combo,[=]{combo->setProperty("comboLastSignal","currentIndexChanged");sync();});
        QObject::connect(combo,&QComboBox::currentTextChanged,combo,[=]{sync();});QObject::connect(combo,qOverload<int>(&QComboBox::activated),combo,[=]{combo->setProperty("comboLastSignal","activated");sync();});
        QObject::connect(variant,&QComboBox::currentTextChanged,combo,[=]{QTimer::singleShot(0,combo,sync);});
        if(fonts) {
            auto* family=new QLineEdit(fonts->currentFont().family(),settings);family->setObjectName("fontFamilyControl");controls->addRow("currentFont.family",family);
            auto* filters=new QComboBox(settings);filters->setObjectName("fontFiltersControl");filters->addItem("AllFonts",int(QFontComboBox::AllFonts));filters->addItem("ScalableFonts",int(QFontComboBox::ScalableFonts));filters->addItem("NonScalableFonts",int(QFontComboBox::NonScalableFonts));filters->addItem("MonospacedFonts",int(QFontComboBox::MonospacedFonts));filters->addItem("ProportionalFonts",int(QFontComboBox::ProportionalFonts));controls->addRow("fontFilters",filters);
            auto* writing=new QComboBox(settings);writing->setObjectName("fontWritingSystemControl");writing->addItem("Any",int(QFontDatabase::Any));
            for(const auto system:QFontDatabase().writingSystems())if(system!=QFontDatabase::Any)writing->addItem(QFontDatabase::writingSystemName(system),int(system));controls->addRow("writingSystem",writing);
            QObject::connect(family,&QLineEdit::editingFinished,fonts,[=]{auto selected=fonts->currentFont();selected.setFamily(family->text());fonts->setCurrentFont(selected);family->setText(fonts->currentFont().family());sync();});
            QObject::connect(filters,qOverload<int>(&QComboBox::currentIndexChanged),fonts,[=]{fonts->setFontFilters(QFontComboBox::FontFilters(filters->currentData().toInt()));sync();});
            QObject::connect(writing,qOverload<int>(&QComboBox::currentIndexChanged),fonts,[=]{fonts->setWritingSystem(QFontDatabase::WritingSystem(writing->currentData().toInt()));sync();});
            QObject::connect(fonts,&QFontComboBox::currentFontChanged,fonts,[=]{if(!family->hasFocus())family->setText(fonts->currentFont().family());fonts->setProperty("comboLastSignal","currentFontChanged");sync();});
        }
        auto* note=new QLabel(QString::fromUtf8("独立原生 ComboBox / FontComboBox，不等同语义 Select。Size 使用 Figma 的 Middle；Focus 外观不会打开列表。当前画廊 Show prefix=false、Show clear=false、Show suffix=true；Prefix/Clear/任意 Suffix 是未确认的视觉适配器提案，未伪装成原生属性。占位文案不作为选项或当前值。可编辑时使用原生 QLineEdit；关闭 editable 会按 Qt 规则移除 validator/completer。字体列表、预览、过滤和缺失字体回退由 Qt 管理；画廊 Arial 仅为固定设计样例。"),settings);note->setWordWrap(true);controls->addRow(note);
        for(auto* field:settings->findChildren<QComboBox*>()){field->setMinimumContentsLength(1);field->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);field->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);}
        appearance();
    }
    if(auto* shortcut=qobject_cast<QKeySequenceEdit*>(preview);shortcut&&componentSet.id=="478:439") {
        controls->setRowWrapPolicy(QFormLayout::WrapAllRows);controls->setContentsMargins(12,12,12,12);
        variant->setMinimumContentsLength(1);variant->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        content->hide();if(auto* label=controls->labelForField(content))label->hide();
        shortcut->setMinimumWidth(280);shortcut->setKeySequence(QKeySequence("Ctrl+Shift+P"));
        auto* sequence=new QLineEdit("Ctrl+Shift+P",settings);sequence->setObjectName("shortcutSequenceControl");controls->addRow("PortableText",sequence);
        auto* applySequence=new QPushButton(QString::fromUtf8("应用候选值"),settings);applySequence->setObjectName("shortcutApplyControl");controls->addRow(applySequence);
        auto* focus=new QPushButton(QString::fromUtf8("聚焦并录制"),settings);focus->setObjectName("shortcutFocusControl");controls->addRow(focus);
        auto* clear=new QPushButton(QString::fromUtf8("清除候选值"),settings);clear->setObjectName("shortcutClearControl");controls->addRow(clear);
        auto* state=new QComboBox(settings);state->setObjectName("shortcutSnapshotControl");state->addItem(QString::fromUtf8("跟随真实交互"),QString());
        for(const auto* name:{"Default","Focus","Recording","Conflict"})state->addItem(name,name);controls->addRow(QString::fromUtf8("状态外观"),state);
        auto* showClear=new QCheckBox("Show clear",settings);showClear->setObjectName("shortcutShowClearControl");showClear->setChecked(true);controls->addRow(showClear);
        auto* showHint=new QCheckBox("Show hint",settings);showHint->setObjectName("shortcutShowHintControl");showHint->setChecked(true);controls->addRow(showHint);
        auto* registry=new QCheckBox(QString::fromUtf8("检查示例注册表"),settings);registry->setObjectName("shortcutRegistryControl");registry->setChecked(true);controls->addRow(registry);
        auto* registered=new QLineEdit("Ctrl+S",settings);registered->setObjectName("shortcutRegisteredControl");controls->addRow(QString::fromUtf8("Save 已占用"),registered);
        auto* save=new QPushButton(QString::fromUtf8("保存到演示内存"),settings);save->setObjectName("shortcutSaveControl");controls->addRow(save);
        auto* restore=new QPushButton(QString::fromUtf8("从演示内存恢复"),settings);restore->setObjectName("shortcutRestoreControl");restore->setEnabled(false);controls->addRow(restore);
        auto* readout=new QLabel(settings);readout->setObjectName("shortcutNativeReadout");readout->setWordWrap(true);controls->addRow(readout);
        auto* note=new QLabel(QString::fromUtf8("只采集候选快捷键，不绑定 QAction/QShortcut 或系统命令。Qt 5.15.2 原生录制最多四段并使用 1 秒结束计时；保留原生键盘解析、修饰键、焦点和信号。画廊保留源稿带空格的 Sequence 及 Focus 提示；真实编辑显示 Qt NativeText，保存用 PortableText。清除适配器调用 QKeySequenceEdit::clear，不仅清空内部文本。固定 Recording/Conflict 仅为外观预览；示例注册表检查与真实原生状态分开显示，未实现生产环境的作用域、保留键和替换政策。Empty 由清空实际候选值进入。"),settings);note->setWordWrap(true);controls->addRow(note);
        state->setMinimumContentsLength(1);state->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        const auto validate=[=]{
            const auto candidate=shortcut->keySequence();const auto occupied=QKeySequence::fromString(registered->text(),QKeySequence::PortableText);QString message;
            if(registry->isChecked()&&!candidate.isEmpty()&&!occupied.isEmpty()) {
                if(candidate.matches(occupied)==QKeySequence::ExactMatch)message="Conflict: Save (demo registry)";
                else if(candidate.matches(occupied)==QKeySequence::PartialMatch||occupied.matches(candidate)==QKeySequence::PartialMatch)message="Prefix conflict: Save (demo registry)";
            }
            commonWidget::setFigmaShortcutConflict(shortcut,message);
        };
        const auto sync=[=]{
            if(!sequence->hasFocus()){const QSignalBlocker blocker(sequence);sequence->setText(shortcut->keySequence().toString(QKeySequence::PortableText));}
            readout->setText(QString("candidate = %1\nchords = %2 / 4\nregistry = %3\nlast signal = %4\nsaved PortableText = %5")
                .arg(shortcut->keySequence().toString(QKeySequence::PortableText)).arg(shortcut->keySequence().count())
                .arg(shortcut->property("figmaShortcutConflict").toString().isEmpty()?"no demo conflict":shortcut->property("figmaShortcutConflict").toString())
                .arg(shortcut->property("shortcutLastSignal").toString(),shortcut->property("shortcutSavedPortableText").toString()));
        };
        const auto appearance=[=]{commonWidget::applyFigmaShortcutAppearance(shortcut,state->currentData().toString(),showClear->isChecked(),showHint->isChecked());sync();};
        QObject::connect(state,qOverload<int>(&QComboBox::currentIndexChanged),shortcut,[=]{appearance();});
        QObject::connect(showClear,&QCheckBox::toggled,shortcut,[=]{appearance();});QObject::connect(showHint,&QCheckBox::toggled,shortcut,[=]{appearance();});
        QObject::connect(applySequence,&QPushButton::clicked,shortcut,[=]{
            const auto parsed=QKeySequence::fromString(sequence->text(),QKeySequence::PortableText);bool valid=sequence->text().trimmed().isEmpty()||!parsed.isEmpty();
            for(int i=0;i<parsed.count();++i)if((parsed[i]&~int(Qt::KeyboardModifierMask))==Qt::Key_unknown)valid=false;
            if(!valid){readout->setText(QString::fromUtf8("PortableText 无法解析，保留原候选值。"));return;}
            shortcut->setKeySequence(parsed);validate();sync();
        });
        QObject::connect(focus,&QPushButton::clicked,shortcut,[=]{shortcut->setFocus(Qt::OtherFocusReason);});
        QObject::connect(clear,&QPushButton::clicked,shortcut,&QKeySequenceEdit::clear);
        QObject::connect(registry,&QCheckBox::toggled,shortcut,[=]{validate();sync();});QObject::connect(registered,&QLineEdit::editingFinished,shortcut,[=]{validate();sync();});
        QObject::connect(save,&QPushButton::clicked,shortcut,[=]{shortcut->setProperty("shortcutSavedPortableText",shortcut->keySequence().toString(QKeySequence::PortableText));restore->setEnabled(true);sync();});
        QObject::connect(restore,&QPushButton::clicked,shortcut,[=]{shortcut->setKeySequence(QKeySequence::fromString(shortcut->property("shortcutSavedPortableText").toString(),QKeySequence::PortableText));validate();sync();});
        QObject::connect(shortcut,&QKeySequenceEdit::keySequenceChanged,shortcut,[=]{shortcut->setProperty("shortcutLastSignal","keySequenceChanged");validate();sync();});
        QObject::connect(shortcut,&QKeySequenceEdit::editingFinished,shortcut,[=]{shortcut->setProperty("shortcutLastSignal","editingFinished");validate();sync();});
        appearance();validate();sync();
    }
    if(auto* line=qobject_cast<QLineEdit*>(preview);line&&componentSet.id=="1116:18") {
        controls->setRowWrapPolicy(QFormLayout::WrapAllRows);controls->setContentsMargins(12,12,12,12);
        variant->setMinimumContentsLength(1);variant->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        line->clear();line->setPlaceholderText("Input text");content->clear();line->setMinimumWidth(320);
        auto* size=new QComboBox(settings);size->setObjectName("lineSizeControl");size->addItems({"Small","Medium","Large"});controls->addRow("Size",size);
        auto* state=new QComboBox(settings);state->setObjectName("lineSnapshotControl");state->addItem(QString::fromUtf8("跟随真实交互"),QString());
        for(const auto* name:{"Default","Hover","Focus","Filled","Error"})state->addItem(name,name);controls->addRow(QString::fromUtf8("状态外观"),state);
        auto* placeholder=new QLineEdit("Input text",settings);placeholder->setObjectName("linePlaceholderControl");controls->addRow("placeholderText",placeholder);
        auto* readOnly=new QCheckBox("readOnly",settings);readOnly->setObjectName("lineReadOnlyControl");controls->addRow(readOnly);
        auto* clear=new QCheckBox("clearButtonEnabled",settings);clear->setObjectName("lineClearControl");controls->addRow(clear);
        auto* prefix=new QCheckBox(QString::fromUtf8("显示前图标 QAction"),settings);prefix->setObjectName("linePrefixControl");controls->addRow(prefix);
        auto* suffix=new QCheckBox(QString::fromUtf8("显示后图标 QAction"),settings);suffix->setObjectName("lineSuffixControl");controls->addRow(suffix);
        auto* maximum=new QSpinBox(settings);maximum->setRange(0,32767);maximum->setValue(line->maxLength());maximum->setObjectName("lineMaxLengthControl");controls->addRow("maxLength",maximum);
        auto* echo=new QComboBox(settings);echo->setObjectName("lineEchoControl");echo->addItems({"Normal","NoEcho","Password","PasswordEchoOnEdit"});controls->addRow("echoMode",echo);
        auto* validation=new QComboBox(settings);validation->setObjectName("lineValidatorControl");validation->addItems({"None","Integer 0..9999","Hex: 4 characters"});controls->addRow("validator",validation);
        auto* integer=new QIntValidator(0,9999,line);auto* hex=new QRegularExpressionValidator(QRegularExpression("[0-9A-Fa-f]{4}"),line);
        auto* mask=new QLineEdit(settings);mask->setObjectName("lineMaskControl");controls->addRow("inputMask",mask);
        auto* alignment=new QComboBox(settings);alignment->setObjectName("lineAlignmentControl");alignment->addItems({"Leading","Center","Trailing"});controls->addRow("alignment",alignment);
        auto* rtl=new QCheckBox("RightToLeft",settings);rtl->setObjectName("lineRtlControl");controls->addRow(rtl);
        auto* selectAll=new QPushButton(QString::fromUtf8("全选"),settings);selectAll->setObjectName("lineSelectAllControl");controls->addRow(selectAll);
        auto* undo=new QPushButton("Undo",settings);undo->setObjectName("lineUndoControl");controls->addRow(undo);
        auto* redo=new QPushButton("Redo",settings);redo->setObjectName("lineRedoControl");controls->addRow(redo);
        auto* readout=new QLabel(settings);readout->setObjectName("lineNativeReadout");readout->setWordWrap(true);controls->addRow(readout);
        auto* note=new QLabel(QString::fromUtf8("独立 QLineEdit，不是语义 Input。Filled 是文本占用；固定 Error 仅为设计外观，不代表 validator 判定。Qt 5 清除按钮随空文本隐藏、只读时禁用；前后图标通过 QAction 接入。原始 SVG 为 16px，动作间距与命中区保持 Qt 原生规则，未宣称等于 Figma 隐藏的 14px 槽位。编辑器、IME、选区、撤销和密码模式保持原生。输入掩码在编辑结束后应用，可能按 Qt 规则重排已有文本并接管 maxLength。"),settings);note->setWordWrap(true);controls->addRow(note);
        for(auto* combo:{size,state,echo,validation,alignment}){combo->setMinimumContentsLength(1);combo->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);}
        const auto sync=[=]{const QSignalBlocker a(content),b(size),c(readOnly),d(maximum);content->setText(line->text());size->setCurrentText(line->property("figmaLineEditSize").toString());readOnly->setChecked(line->isReadOnly());maximum->setValue(line->maxLength());maximum->setEnabled(line->inputMask().isEmpty());
            undo->setEnabled(line->isUndoAvailable()&&!line->isReadOnly());redo->setEnabled(line->isRedoAvailable()&&!line->isReadOnly());
            readout->setText(QString("acceptableInput = %1\ncursorPosition = %2\nselectionStart = %3\nlast signal = %4").arg(line->hasAcceptableInput()?"true":"false").arg(line->cursorPosition()).arg(line->selectionStart()).arg(line->property("lineLastSignal").toString()));};
        const auto appearance=[=]{commonWidget::applyFigmaLineEditAppearance(line,size->currentText(),state->currentData().toString());sync();};
        QObject::connect(size,qOverload<int>(&QComboBox::currentIndexChanged),line,[=]{appearance();});QObject::connect(state,qOverload<int>(&QComboBox::currentIndexChanged),line,[=]{appearance();});
        QObject::connect(placeholder,&QLineEdit::textChanged,line,&QLineEdit::setPlaceholderText);
        QObject::connect(readOnly,&QCheckBox::toggled,line,[=](bool on){line->setReadOnly(on);sync();});
        QObject::connect(clear,&QCheckBox::toggled,line,&QLineEdit::setClearButtonEnabled);
        const auto actions=[=]{commonWidget::applyFigmaLineEditActions(line,prefix->isChecked(),suffix->isChecked());
            for(const auto& name:{"Prefix","Suffix"})if(auto* action=line->findChild<QAction*>(QString("figmaLineEdit%1Action").arg(name))) {
                if(!action->property("demoSignalConnected").toBool()){action->setProperty("demoSignalConnected",true);QObject::connect(action,&QAction::triggered,line,[=]{line->setProperty("lineLastSignal",QString("%1 action").arg(name));sync();});}
            }};
        QObject::connect(prefix,&QCheckBox::toggled,line,[=]{actions();});QObject::connect(suffix,&QCheckBox::toggled,line,[=]{actions();});
        QObject::connect(maximum,qOverload<int>(&QSpinBox::valueChanged),line,&QLineEdit::setMaxLength);
        QObject::connect(echo,qOverload<int>(&QComboBox::currentIndexChanged),line,[=](int i){line->setEchoMode(QLineEdit::EchoMode(i));});
        QObject::connect(validation,qOverload<int>(&QComboBox::currentIndexChanged),line,[=](int i){line->setValidator(i==1?static_cast<QValidator*>(integer):i==2?static_cast<QValidator*>(hex):nullptr);sync();});
        QObject::connect(mask,&QLineEdit::editingFinished,line,[=]{line->setInputMask(mask->text());sync();});
        QObject::connect(alignment,qOverload<int>(&QComboBox::currentIndexChanged),line,[=](int i){line->setAlignment(i==1?Qt::AlignHCenter:i==2?Qt::AlignTrailing:Qt::AlignLeading);});
        QObject::connect(rtl,&QCheckBox::toggled,line,[=](bool on){line->setLayoutDirection(on?Qt::RightToLeft:Qt::LeftToRight);});
        QObject::connect(selectAll,&QPushButton::clicked,line,&QLineEdit::selectAll);QObject::connect(undo,&QPushButton::clicked,line,&QLineEdit::undo);QObject::connect(redo,&QPushButton::clicked,line,&QLineEdit::redo);
        QObject::connect(line,&QLineEdit::textChanged,line,[=]{sync();});QObject::connect(line,&QLineEdit::cursorPositionChanged,line,[=]{sync();});QObject::connect(line,&QLineEdit::selectionChanged,line,[=]{sync();});
        QObject::connect(line,&QLineEdit::textEdited,line,[=]{line->setProperty("lineLastSignal","textEdited");sync();});QObject::connect(line,&QLineEdit::editingFinished,line,[=]{line->setProperty("lineLastSignal","editingFinished");sync();});
        QObject::connect(variant,&QComboBox::currentTextChanged,line,[=]{QTimer::singleShot(0,line,sync);});appearance();
    }
    if(auto* dial=qobject_cast<QDial*>(preview);dial&&componentSet.id=="475:551") {
        controls->setRowWrapPolicy(QFormLayout::WrapAllRows);controls->setContentsMargins(12,12,12,12);
        variant->setMinimumContentsLength(1);variant->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        content->hide();if(auto* label=controls->labelForField(content))label->hide();
        dial->setRange(0,100);dial->setValue(40);dial->setNotchesVisible(true);dial->setFixedSize(180,180);
        auto* state=new QComboBox(settings);state->setObjectName("dialSnapshotControl");
        state->addItem(QString::fromUtf8("跟随真实交互"),QString());for(const auto* name:{"Default","Focus","Dragging"})state->addItem(name,name);
        state->setMinimumContentsLength(1);state->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);controls->addRow(QString::fromUtf8("状态外观"),state);
        const auto number=[=](const char* name){auto* spin=new QSpinBox(settings);spin->setObjectName(name);spin->setRange(-1000000,1000000);spin->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);return spin;};
        auto* minimum=number("dialMinimumControl");auto* maximum=number("dialMaximumControl");auto* value=number("dialValueControl");auto* position=number("dialPositionControl");
        auto* single=number("dialSingleStepControl");single->setRange(0,10000);auto* pageStep=number("dialPageStepControl");pageStep->setRange(0,10000);
        controls->addRow("minimum",minimum);controls->addRow("maximum",maximum);controls->addRow("value",value);controls->addRow("sliderPosition",position);controls->addRow("singleStep",single);controls->addRow("pageStep",pageStep);
        auto* wrapping=new QCheckBox("wrapping",settings);wrapping->setObjectName("dialWrappingControl");controls->addRow(wrapping);
        auto* notches=new QCheckBox("notchesVisible",settings);notches->setObjectName("dialNotchesControl");notches->setChecked(true);controls->addRow(notches);
        auto* target=new QDoubleSpinBox(settings);target->setObjectName("dialNotchTargetControl");target->setRange(0.1,100);target->setDecimals(1);target->setSingleStep(0.1);controls->addRow("notchTarget (px)",target);
        auto* tracking=new QCheckBox("tracking",settings);tracking->setObjectName("dialTrackingControl");tracking->setChecked(true);controls->addRow(tracking);
        auto* inverted=new QCheckBox("invertedAppearance",settings);inverted->setObjectName("dialInvertedAppearanceControl");controls->addRow(inverted);
        auto* keys=new QCheckBox("invertedControls",settings);keys->setObjectName("dialInvertedControlsControl");controls->addRow(keys);
        auto* showValue=new QCheckBox(QString::fromUtf8("显示数值标签"),settings);showValue->setObjectName("dialShowValueControl");showValue->setChecked(true);controls->addRow(showValue);
        auto* automatic=new QCheckBox(QString::fromUtf8("标签跟随当前位置"),settings);automatic->setObjectName("dialAutomaticLabelControl");automatic->setChecked(true);controls->addRow(automatic);
        auto* label=new QLineEdit("40",settings);label->setObjectName("dialLabelControl");label->setEnabled(false);controls->addRow("Value label",label);
        auto* readout=new QLabel(settings);readout->setObjectName("dialNativeReadout");readout->setWordWrap(true);controls->addRow(readout);
        auto* note=new QLabel(QString::fromUtf8("画廊保留 Figma 角度与固定 12 个刻度。外观页按 Qt 原生角度取值：Off 为底部缺口的 300°，On 从底部起算；指针随 sliderPosition。notchSize 为只读派生值，标签是独立组合文字，固定 Dragging 不设置 sliderDown。Qt 5.15.2 的非零 minimum + invertedAppearance 鼠标映射存在原生偏差，未擅改。演示编辑范围限制为 ±1000000；Wrapping 必须有非零范围，以规避 Qt 的取模零边界。"),settings);note->setWordWrap(true);controls->addRow(note);
        const auto sync=[=]{
            const QSignalBlocker a(minimum),b(maximum),c(value),d(position),e(single),f(pageStep),g(wrapping),h(target);
            minimum->setRange(-1000000,dial->wrapping()?dial->maximum()-1:1000000);
            maximum->setRange(dial->wrapping()?dial->minimum()+1:-1000000,1000000);
            minimum->setValue(dial->minimum());maximum->setValue(dial->maximum());value->setRange(dial->minimum(),dial->maximum());value->setValue(dial->value());
            position->setRange(dial->minimum(),dial->maximum());position->setValue(dial->sliderPosition());single->setValue(dial->singleStep());pageStep->setValue(dial->pageStep());wrapping->setChecked(dial->wrapping());target->setValue(dial->notchTarget());
            readout->setText(QString("value = %1\nposition = %2\nsliderDown = %3\nnotchSize = %4 (read-only)\nlast signal = %5").arg(dial->value()).arg(dial->sliderPosition()).arg(dial->isSliderDown()?"true":"false").arg(dial->notchSize()).arg(dial->property("dialLastSignal").toString()));
        };
        const auto appearance=[=]{commonWidget::applyFigmaDialAppearance(dial,state->currentData().toString(),false,showValue->isChecked(),automatic->isChecked()?QString():label->text());label->setEnabled(!automatic->isChecked());sync();};
        for(const auto& pair:QList<QPair<QSpinBox*,const char*>>{{minimum,"minimum"},{maximum,"maximum"},{value,"value"},{position,"sliderPosition"},{single,"singleStep"},{pageStep,"pageStep"}})
            QObject::connect(pair.first,qOverload<int>(&QSpinBox::valueChanged),dial,[=](int n){dial->setProperty(pair.second,n);sync();dial->update();});
        QObject::connect(wrapping,&QCheckBox::toggled,dial,[=](bool on){if(on&&dial->minimum()==dial->maximum()){if(dial->maximum()==1000000)dial->setMinimum(999999);else dial->setMaximum(dial->minimum()+1);}dial->setWrapping(on);sync();});
        QObject::connect(notches,&QCheckBox::toggled,dial,&QDial::setNotchesVisible);
        QObject::connect(target,qOverload<double>(&QDoubleSpinBox::valueChanged),dial,[=](double n){dial->setNotchTarget(n);sync();});
        QObject::connect(tracking,&QCheckBox::toggled,dial,&QAbstractSlider::setTracking);
        QObject::connect(inverted,&QCheckBox::toggled,dial,&QAbstractSlider::setInvertedAppearance);
        QObject::connect(keys,&QCheckBox::toggled,dial,&QAbstractSlider::setInvertedControls);
        QObject::connect(state,qOverload<int>(&QComboBox::currentIndexChanged),dial,[=]{appearance();});
        QObject::connect(showValue,&QCheckBox::toggled,dial,[=]{appearance();});QObject::connect(automatic,&QCheckBox::toggled,dial,[=]{appearance();});QObject::connect(label,&QLineEdit::textChanged,dial,[=]{appearance();});
        QObject::connect(dial,&QAbstractSlider::valueChanged,dial,[=](int n){dial->setProperty("dialLastSignal",QString("valueChanged(%1)").arg(n));sync();});
        QObject::connect(dial,&QAbstractSlider::sliderMoved,dial,[=](int n){dial->setProperty("dialLastSignal",QString("sliderMoved(%1)").arg(n));sync();});
        QObject::connect(dial,&QAbstractSlider::sliderPressed,dial,[=]{dial->setProperty("dialLastSignal","sliderPressed");sync();});
        QObject::connect(dial,&QAbstractSlider::sliderReleased,dial,[=]{dial->setProperty("dialLastSignal","sliderReleased");sync();});
        QObject::connect(dial,&QAbstractSlider::rangeChanged,dial,[=]{sync();});QObject::connect(variant,&QComboBox::currentTextChanged,dial,[=]{QTimer::singleShot(0,dial,sync);});appearance();
    }
    if(auto* bar=qobject_cast<QScrollBar*>(preview);bar&&componentSet.id=="471:477") {
        controls->setRowWrapPolicy(QFormLayout::WrapAllRows);controls->setContentsMargins(12,12,12,12);
        variant->setMinimumContentsLength(1);variant->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        content->hide();if(auto* label=controls->labelForField(content))label->hide();
        bar->setValue(40);bar->setPageStep(20);bar->setProperty("figmaScrollBarShowLineButtons",true);
        auto* orientation=new QComboBox(settings);orientation->setObjectName("scrollOrientationControl");orientation->addItems({"Horizontal","Vertical"});controls->addRow("Orientation",orientation);
        auto* state=new QComboBox(settings);state->setObjectName("scrollSnapshotControl");state->addItem(QString::fromUtf8("跟随真实交互"),QString());
        for(const auto* name:{"Default","Hover","Dragging"})state->addItem(name,name);controls->addRow(QString::fromUtf8("状态外观"),state);
        for(auto* combo:{orientation,state}){combo->setMinimumContentsLength(1);combo->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);}
        const auto number=[=](const char* name){auto* spin=new QSpinBox(settings);spin->setObjectName(name);spin->setRange(-2147483647,2147483647);spin->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);return spin;};
        auto* minimum=number("scrollMinimumControl");auto* maximum=number("scrollMaximumControl");auto* value=number("scrollValueControl");auto* position=number("scrollPositionControl");
        auto* single=number("scrollSingleStepControl");single->setMinimum(0);auto* pageStep=number("scrollPageStepControl");pageStep->setMinimum(0);
        controls->addRow("minimum",minimum);controls->addRow("maximum",maximum);controls->addRow("value",value);controls->addRow("sliderPosition",position);
        controls->addRow("singleStep",single);controls->addRow("pageStep",pageStep);
        auto* showLines=new QCheckBox("Show line buttons",settings);showLines->setObjectName("scrollShowLinesControl");showLines->setChecked(true);controls->addRow(showLines);
        auto* tracking=new QCheckBox("tracking",settings);tracking->setObjectName("scrollTrackingControl");tracking->setChecked(bar->hasTracking());controls->addRow(tracking);
        auto* invertedAppearance=new QCheckBox("invertedAppearance",settings);invertedAppearance->setObjectName("scrollInvertedAppearanceControl");controls->addRow(invertedAppearance);
        auto* invertedControls=new QCheckBox("invertedControls",settings);invertedControls->setObjectName("scrollInvertedControlsControl");invertedControls->setChecked(bar->invertedControls());controls->addRow(invertedControls);
        auto* rtl=new QCheckBox("RightToLeft",settings);rtl->setObjectName("scrollRtlControl");controls->addRow(rtl);
        auto* keyboard=new QCheckBox(QString::fromUtf8("允许键盘聚焦"),settings);keyboard->setObjectName("scrollKeyboardControl");controls->addRow(keyboard);
        auto* readout=new QLabel(settings);readout->setObjectName("scrollNativeReadout");readout->setWordWrap(true);controls->addRow(readout);
        auto* note=new QLabel(QString::fromUtf8("画廊保留源稿 72px 滑块，外观页按 range、pageStep、position 计算。Show line buttons 是样式适配选项；隐藏时同步重算轨道和命中区。默认 NoFocus，键盘聚焦是运行时扩展。固定 Dragging 不设置 sliderDown。关闭 tracking 时释放或禁用会提交待定位置。"),settings);note->setWordWrap(true);controls->addRow(note);
        const auto sync=[=]{
            const QSignalBlocker a(minimum),b(maximum),c(value),d(position),e(single),f(pageStep),g(orientation);
            minimum->setValue(bar->minimum());maximum->setValue(bar->maximum());value->setRange(bar->minimum(),bar->maximum());value->setValue(bar->value());
            position->setRange(bar->minimum(),bar->maximum());position->setValue(bar->sliderPosition());single->setValue(bar->singleStep());pageStep->setValue(bar->pageStep());
            orientation->setCurrentIndex(bar->orientation()==Qt::Vertical?1:0);
            readout->setText(QString("value = %1\nposition = %2\nsliderDown = %3\nlast signal = %4").arg(bar->value()).arg(bar->sliderPosition()).arg(bar->isSliderDown()?"true":"false",bar->property("scrollLastSignal").toString()));
        };
        const auto appearance=[=]{commonWidget::applyFigmaScrollBarAppearance(bar,state->currentData().toString(),false,showLines->isChecked());
            if(bar->orientation()==Qt::Horizontal){bar->setMinimumSize(280,56);bar->setMaximumSize(QWIDGETSIZE_MAX,56);bar->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);}
            else{bar->setMinimumSize(100,260);bar->setMaximumSize(100,QWIDGETSIZE_MAX);bar->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Expanding);}sync();};
        for(const auto& pair:QList<QPair<QSpinBox*,const char*>>{{minimum,"minimum"},{maximum,"maximum"},{value,"value"},{position,"sliderPosition"},{single,"singleStep"},{pageStep,"pageStep"}})
            QObject::connect(pair.first,qOverload<int>(&QSpinBox::valueChanged),bar,[=](int n){bar->setProperty(pair.second,n);sync();bar->update();});
        QObject::connect(orientation,qOverload<int>(&QComboBox::currentIndexChanged),bar,[=](int i){bar->setOrientation(i?Qt::Vertical:Qt::Horizontal);appearance();});
        QObject::connect(state,qOverload<int>(&QComboBox::currentIndexChanged),bar,[=]{appearance();});QObject::connect(showLines,&QCheckBox::toggled,bar,[=]{appearance();});
        QObject::connect(tracking,&QCheckBox::toggled,bar,&QAbstractSlider::setTracking);
        QObject::connect(invertedAppearance,&QCheckBox::toggled,bar,&QAbstractSlider::setInvertedAppearance);QObject::connect(invertedControls,&QCheckBox::toggled,bar,&QAbstractSlider::setInvertedControls);
        QObject::connect(rtl,&QCheckBox::toggled,bar,[=](bool on){bar->setLayoutDirection(on?Qt::RightToLeft:Qt::LeftToRight);bar->update();});
        QObject::connect(keyboard,&QCheckBox::toggled,bar,[=](bool on){bar->setFocusPolicy(on?Qt::StrongFocus:Qt::NoFocus);});
        QObject::connect(bar,&QAbstractSlider::valueChanged,bar,[=](int n){bar->setProperty("scrollLastSignal",QString("valueChanged(%1)").arg(n));sync();});
        QObject::connect(bar,&QAbstractSlider::sliderMoved,bar,[=](int n){bar->setProperty("scrollLastSignal",QString("sliderMoved(%1)").arg(n));sync();});
        QObject::connect(bar,&QAbstractSlider::sliderPressed,bar,[=]{bar->setProperty("scrollLastSignal","sliderPressed");sync();});
        QObject::connect(bar,&QAbstractSlider::sliderReleased,bar,[=]{bar->setProperty("scrollLastSignal","sliderReleased");sync();});
        QObject::connect(bar,&QAbstractSlider::rangeChanged,bar,[=]{sync();});
        QObject::connect(variant,&QComboBox::currentTextChanged,bar,[=]{QTimer::singleShot(0,bar,sync);});appearance();
    }
    if(auto* slider=qobject_cast<QSlider*>(preview);slider&&componentSet.id=="467:195") {
        controls->setRowWrapPolicy(QFormLayout::WrapAllRows);controls->setContentsMargins(12,12,12,12);
        variant->setMinimumContentsLength(1);variant->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        content->hide();if(auto* label=controls->labelForField(content))label->hide();
        slider->setValue(40);slider->setTickPosition(QSlider::TicksBelow);slider->setTickInterval(10);
        slider->setProperty("figmaSliderShowValue",true);slider->setProperty("figmaSliderAutomaticLabel",true);
        auto* orientation=new QComboBox(settings);orientation->setObjectName("sliderOrientationControl");orientation->addItems({"Horizontal","Vertical"});controls->addRow("Orientation",orientation);
        auto* state=new QComboBox(settings);state->setObjectName("sliderSnapshotControl");
        state->addItem(QString::fromUtf8("跟随真实交互"),QString());for(const auto* name:{"Default","Focus","Dragging"})state->addItem(name,name);
        controls->addRow(QString::fromUtf8("状态外观"),state);
        const auto number=[=](const char* name){auto* spin=new QSpinBox(settings);spin->setObjectName(name);spin->setRange(-2147483647,2147483647);spin->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);return spin;};
        auto* minimum=number("sliderMinimumControl");auto* maximum=number("sliderMaximumControl");auto* value=number("sliderValueControl");auto* position=number("sliderPositionControl");
        auto* single=number("sliderSingleStepControl");single->setMinimum(0);auto* pageStep=number("sliderPageStepControl");pageStep->setMinimum(0);
        controls->addRow("minimum",minimum);controls->addRow("maximum",maximum);controls->addRow("value",value);controls->addRow("sliderPosition",position);
        controls->addRow("singleStep",single);controls->addRow("pageStep",pageStep);
        auto* tracking=new QCheckBox("tracking",settings);tracking->setObjectName("sliderTrackingControl");tracking->setChecked(true);controls->addRow(tracking);
        auto* invertedAppearance=new QCheckBox(QString::fromUtf8("反转视觉方向"),settings);invertedAppearance->setObjectName("sliderInvertedAppearanceControl");invertedAppearance->setToolTip("invertedAppearance");controls->addRow(invertedAppearance);
        auto* invertedControls=new QCheckBox(QString::fromUtf8("反转键盘／滚轮"),settings);invertedControls->setObjectName("sliderInvertedControlsControl");invertedControls->setToolTip("invertedControls");controls->addRow(invertedControls);
        auto* rtl=new QCheckBox("RightToLeft",settings);rtl->setObjectName("sliderRtlControl");controls->addRow(rtl);
        auto* ticks=new QComboBox(settings);ticks->setObjectName("sliderTicksControl");ticks->addItems({"NoTicks","TicksAbove / Left","TicksBelow / Right","TicksBothSides"});ticks->setCurrentIndex(2);controls->addRow("tickPosition",ticks);
        auto* tickInterval=number("sliderTickIntervalControl");tickInterval->setMinimum(0);controls->addRow("tickInterval",tickInterval);
        auto* showValue=new QCheckBox(QString::fromUtf8("显示数值气泡"),settings);showValue->setObjectName("sliderShowValueControl");showValue->setChecked(true);controls->addRow(showValue);
        auto* automatic=new QCheckBox(QString::fromUtf8("标签跟随当前位置"),settings);automatic->setObjectName("sliderAutomaticLabelControl");automatic->setChecked(true);controls->addRow(automatic);
        auto* label=new QLineEdit("40",settings);label->setObjectName("sliderLabelControl");label->setEnabled(false);controls->addRow("Value label",label);
        for(auto* combo:{orientation,state,ticks}) {combo->setMinimumContentsLength(1);combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);combo->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);}
        auto* readout=new QLabel(settings);readout->setObjectName("sliderNativeReadout");readout->setWordWrap(true);controls->addRow(readout);
        auto* note=new QLabel(QString::fromUtf8("Figma 标签文本与轨道、手柄位置独立；画廊保留原布局，外观页按原生数值定位。固定 Dragging 仅改变外观，不设置 sliderDown。tracking 关闭时拖动只改变位置，释放后提交 value；禁用也会按 Qt 原生规则提交待定位置。密集刻度按整数倍间隔抽稀，最多 2049 个。"),settings);note->setWordWrap(true);controls->addRow(note);
        const auto sync=[=] {
            const QSignalBlocker a(minimum),b(maximum),c(value),d(position),e(single),f(pageStep),g(tickInterval),h(orientation);
            minimum->setValue(slider->minimum());maximum->setValue(slider->maximum());value->setRange(slider->minimum(),slider->maximum());value->setValue(slider->value());
            position->setRange(slider->minimum(),slider->maximum());position->setValue(slider->sliderPosition());single->setValue(slider->singleStep());pageStep->setValue(slider->pageStep());tickInterval->setValue(slider->tickInterval());
            orientation->setCurrentIndex(slider->orientation()==Qt::Vertical?1:0);
            readout->setText(QString("value = %1\nposition = %2\nsliderDown = %3\nlast signal = %4").arg(slider->value()).arg(slider->sliderPosition()).arg(slider->isSliderDown()?"true":"false",slider->property("sliderLastSignal").toString()));
        };
        const auto appearance=[=] {
            commonWidget::applyFigmaSliderAppearance(slider,state->currentData().toString(),false,showValue->isChecked(),automatic->isChecked()?QString():label->text());
            label->setEnabled(!automatic->isChecked());sync();
        };
        for(const auto& pair:QList<QPair<QSpinBox*,const char*>>{{minimum,"minimum"},{maximum,"maximum"},{value,"value"},{position,"sliderPosition"},{single,"singleStep"},{pageStep,"pageStep"},{tickInterval,"tickInterval"}})
            QObject::connect(pair.first,qOverload<int>(&QSpinBox::valueChanged),slider,[=](int n){slider->setProperty(pair.second,n);sync();slider->update();});
        QObject::connect(orientation,qOverload<int>(&QComboBox::currentIndexChanged),slider,[=](int i){slider->setOrientation(i?Qt::Vertical:Qt::Horizontal);appearance();});
        QObject::connect(state,qOverload<int>(&QComboBox::currentIndexChanged),slider,[=]{appearance();});
        QObject::connect(tracking,&QCheckBox::toggled,slider,&QAbstractSlider::setTracking);
        QObject::connect(invertedAppearance,&QCheckBox::toggled,slider,&QAbstractSlider::setInvertedAppearance);
        QObject::connect(invertedControls,&QCheckBox::toggled,slider,&QAbstractSlider::setInvertedControls);
        QObject::connect(rtl,&QCheckBox::toggled,slider,[=](bool on){slider->setLayoutDirection(on?Qt::RightToLeft:Qt::LeftToRight);slider->update();});
        QObject::connect(ticks,qOverload<int>(&QComboBox::currentIndexChanged),slider,[=](int i){slider->setTickPosition(QSlider::TickPosition(i));});
        QObject::connect(showValue,&QCheckBox::toggled,slider,[=]{appearance();});QObject::connect(automatic,&QCheckBox::toggled,slider,[=]{appearance();});QObject::connect(label,&QLineEdit::textChanged,slider,[=]{appearance();});
        QObject::connect(slider,&QAbstractSlider::valueChanged,slider,[=](int n){slider->setProperty("sliderLastSignal",QString("valueChanged(%1)").arg(n));sync();});
        QObject::connect(slider,&QAbstractSlider::sliderMoved,slider,[=](int n){slider->setProperty("sliderLastSignal",QString("sliderMoved(%1)").arg(n));sync();});
        QObject::connect(slider,&QAbstractSlider::sliderPressed,slider,[=]{slider->setProperty("sliderLastSignal","sliderPressed");sync();});
        QObject::connect(slider,&QAbstractSlider::sliderReleased,slider,[=]{slider->setProperty("sliderLastSignal","sliderReleased");sync();});
        QObject::connect(slider,&QAbstractSlider::rangeChanged,slider,[=]{sync();});
        QObject::connect(variant,&QComboBox::currentTextChanged,slider,[=]{QTimer::singleShot(0,slider,sync);});appearance();
    }
    if(auto* date=qobject_cast<QDateTimeEdit*>(preview);
       date && (componentSet.id=="463:136" || componentSet.id=="464:111" || componentSet.id=="465:189")) {
        const bool timeOnly=componentSet.id=="465:189",dateOnly=componentSet.id=="464:111";
        controls->setRowWrapPolicy(QFormLayout::WrapAllRows);controls->setContentsMargins(12,12,12,12);
        variant->setMinimumContentsLength(1);variant->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        content->hide();if(auto* label=controls->labelForField(content))label->hide();
        date->setLocale(QLocale::c());
        date->setDisplayFormat(timeOnly?"HH:mm:ss":dateOnly?"yyyy-MM-dd":QString::fromUtf8("yyyy-MM-dd '·' HH:mm"));
        if(timeOnly)date->setTime(QTime(2,30,45));else date->setDateTime(QDateTime(QDate(2026,10,7),QTime(14,30)));
        date->setProperty("figmaDateShowTime",!dateOnly);date->setProperty("figmaDateShowSeconds",true);
        date->setProperty("figmaDateShowSpec",!timeOnly&&!dateOnly);date->setProperty("figmaDateSpecText"," CST");
        const auto makeValue=[=](const char* name) {
            QDateTimeEdit* editor=timeOnly?static_cast<QDateTimeEdit*>(new QTimeEdit(settings)):
                dateOnly?static_cast<QDateTimeEdit*>(new QDateEdit(settings)):new QDateTimeEdit(settings);
            editor->setObjectName(name);editor->setLocale(QLocale::c());editor->setCalendarPopup(!timeOnly);
            editor->setDisplayFormat(timeOnly?"HH:mm:ss":dateOnly?"yyyy-MM-dd":"yyyy-MM-dd HH:mm:ss");
            editor->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);return editor;
        };
        auto* value=makeValue("dateValueControl");controls->addRow(QString::fromUtf8("当前值"),value);
        auto* minimum=makeValue("dateMinimumControl");controls->addRow(QString::fromUtf8("最小值"),minimum);
        auto* maximum=makeValue("dateMaximumControl");controls->addRow(QString::fromUtf8("最大值"),maximum);
        auto* rangeNote=new QLabel(settings);rangeNote->setObjectName("dateRangeNotice");rangeNote->setWordWrap(true);controls->addRow(rangeNote);
        auto* calendar=new QCheckBox(QString::fromUtf8("允许日历弹出"),settings);calendar->setObjectName("dateCalendarControl");
        if(!timeOnly)controls->addRow(calendar);else calendar->hide();
        auto* readOnly=new QCheckBox("readOnly",settings);readOnly->setObjectName("dateReadOnlyControl");controls->addRow(readOnly);
        auto* snapshot=new QCheckBox(QString::fromUtf8("固定 Figma 焦点外观"),settings);snapshot->setObjectName("dateSnapshotControl");controls->addRow(snapshot);
        auto* showTime=new QCheckBox(QString::fromUtf8("显示时间"),settings);showTime->setObjectName("dateShowTimeControl");showTime->setChecked(true);
        if(!dateOnly&&!timeOnly)controls->addRow(showTime);else showTime->hide();
        auto* showSpec=new QCheckBox(QString::fromUtf8("显示时区注释"),settings);showSpec->setObjectName("dateShowSpecControl");showSpec->setChecked(true);
        auto* spec=new QLineEdit(" CST",settings);spec->setObjectName("dateSpecControl");
        if(!dateOnly&&!timeOnly){controls->addRow(showSpec);controls->addRow("Time spec (text)",spec);}else{showSpec->hide();spec->hide();}
        auto* format=new QComboBox(settings);format->setObjectName("dateFormatControl");format->addItems({"24h","12h"});
        auto* seconds=new QCheckBox(QString::fromUtf8("显示秒"),settings);seconds->setObjectName("dateShowSecondsControl");seconds->setChecked(true);
        if(timeOnly){controls->addRow("Format",format);controls->addRow(seconds);}else{format->hide();seconds->hide();}
        auto* locale=new QComboBox(settings);locale->setObjectName("dateLocaleControl");locale->addItems({"C","en_US","zh_CN","de_DE"});controls->addRow("locale",locale);
        auto* section=new QComboBox(settings);section->setObjectName("dateSectionControl");controls->addRow("currentSection",section);
        auto* wrapping=new QCheckBox("wrapping",settings);wrapping->setObjectName("dateWrappingControl");controls->addRow(wrapping);
        auto* tracking=new QCheckBox(QString::fromUtf8("输入时即时提交"),settings);tracking->setObjectName("dateTrackingControl");tracking->setToolTip("keyboardTracking");tracking->setChecked(true);controls->addRow(tracking);
        for(auto* combo:{format,locale,section}) {
            combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);combo->setMinimumContentsLength(1);
            combo->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        }
        auto* readout=new QLabel(settings);readout->setObjectName("dateNativeReadout");readout->setWordWrap(true);controls->addRow(readout);
        auto* note=new QLabel(QString::fromUtf8("CST 仅为 Figma 注释，不修改 Qt timeSpec。隐藏字段和切换 12/24 小时制不改写当前值；日历与编辑选区保留 Qt 原生行为。活动编辑使用原生字级。"),settings);
        note->setWordWrap(true);controls->addRow(note);
        const auto sync=[=] {
            const QSignalBlocker a(value),b(minimum),c(maximum),d(calendar),e(readOnly),f(format),g(section);
            minimum->setDateTime(date->minimumDateTime());maximum->setDateTime(date->maximumDateTime());
            value->setDateTimeRange(date->minimumDateTime(),date->maximumDateTime());value->setDateTime(date->dateTime());
            calendar->setChecked(date->calendarPopup());readOnly->setChecked(date->isReadOnly());
            format->setCurrentText(date->displayedSections()&QDateTimeEdit::AmPmSection?"12h":"24h");
            section->clear();
            for(const auto& pair:QList<QPair<QString,QDateTimeEdit::Section>>{{"Year",QDateTimeEdit::YearSection},{"Month",QDateTimeEdit::MonthSection},
                {"Day",QDateTimeEdit::DaySection},{"Hour",QDateTimeEdit::HourSection},{"Minute",QDateTimeEdit::MinuteSection},
                {"Second",QDateTimeEdit::SecondSection},{"AM/PM",QDateTimeEdit::AmPmSection}})
                if(date->displayedSections()&pair.second)section->addItem(pair.first,int(pair.second));
            section->setCurrentIndex(section->findData(int(date->currentSection())));
            readout->setText(QString("value = %1\nformat = %2\ntimeSpec = %3\nacceptableInput = %4")
                .arg(timeOnly?date->time().toString("HH:mm:ss"):dateOnly?date->date().toString(Qt::ISODate):date->dateTime().toString(Qt::ISODate),
                    date->displayFormat(),QString::number(int(date->timeSpec())),date->hasAcceptableInput()?"true":"false"));
        };
        const auto appearance=[=] {
            commonWidget::applyFigmaDateTimeAppearance(date,snapshot->isChecked()?"Focus":"",
                !timeOnly&&!dateOnly&&showTime->isChecked()&&showSpec->isChecked()?spec->text():QString());sync();
        };
        const auto changeFormat=[=] {
            date->setProperty("figmaDateShowTime",showTime->isChecked());date->setProperty("figmaDateShowSeconds",seconds->isChecked());
            date->setProperty("figmaDateShowSpec",showSpec->isChecked());date->setProperty("figmaDateSpecText",spec->text());
            const QString next=timeOnly?(format->currentText()=="12h"?(seconds->isChecked()?"hh:mm:ss AP":"hh:mm AP"):
                (seconds->isChecked()?"HH:mm:ss":"HH:mm")):dateOnly||!showTime->isChecked()?"yyyy-MM-dd":QString::fromUtf8("yyyy-MM-dd '·' HH:mm");
            commonWidget::setDateTimeDisplayFormatPreservingValue(date,next);
            showSpec->setEnabled(showTime->isChecked());spec->setEnabled(showTime->isChecked()&&showSpec->isChecked());appearance();
        };
        QObject::connect(value,&QDateTimeEdit::dateTimeChanged,date,[=](const QDateTime& next){
            if(timeOnly)date->setTime(next.time());else if(dateOnly)date->setDate(next.date());else date->setDateTime(next);sync();
        });
        const auto range=[=] {
            const bool ordered=timeOnly?minimum->time()<=maximum->time():minimum->dateTime()<=maximum->dateTime();
            if(!ordered){rangeNote->setText(QString::fromUtf8("范围未应用：最小值不能大于最大值。"));return;}
            rangeNote->clear();if(timeOnly)date->setTimeRange(minimum->time(),maximum->time());
            else if(dateOnly)date->setDateRange(minimum->date(),maximum->date());else date->setDateTimeRange(minimum->dateTime(),maximum->dateTime());sync();
        };
        QObject::connect(minimum,&QAbstractSpinBox::editingFinished,date,range);QObject::connect(maximum,&QAbstractSpinBox::editingFinished,date,range);
        QObject::connect(calendar,&QCheckBox::toggled,date,[=](bool on){date->setCalendarPopup(on);appearance();});
        QObject::connect(readOnly,&QCheckBox::toggled,date,[=](bool on){date->setReadOnly(on);appearance();});
        QObject::connect(snapshot,&QCheckBox::toggled,date,[=](bool on){date->setProperty("figmaDateFixedFocus",on);appearance();});
        QObject::connect(showTime,&QCheckBox::toggled,date,[=]{changeFormat();});QObject::connect(showSpec,&QCheckBox::toggled,date,[=]{changeFormat();});
        QObject::connect(spec,&QLineEdit::textChanged,date,[=]{changeFormat();});QObject::connect(seconds,&QCheckBox::toggled,date,[=]{changeFormat();});
        QObject::connect(format,&QComboBox::currentTextChanged,date,[=]{changeFormat();});
        QObject::connect(locale,&QComboBox::currentTextChanged,date,[=](const QString& name){date->setLocale(QLocale(name));appearance();});
        QObject::connect(section,qOverload<int>(&QComboBox::currentIndexChanged),date,[=](int i){if(i>=0)date->setCurrentSection(QDateTimeEdit::Section(section->itemData(i).toInt()));});
        QObject::connect(wrapping,&QCheckBox::toggled,date,&QAbstractSpinBox::setWrapping);QObject::connect(tracking,&QCheckBox::toggled,date,&QAbstractSpinBox::setKeyboardTracking);
        QObject::connect(date,&QDateTimeEdit::dateTimeChanged,date,[=]{sync();});
        QObject::connect(variant,&QComboBox::currentTextChanged,date,[=]{QTimer::singleShot(0,date,sync);});
        appearance();
    }
    if(auto* spin=qobject_cast<QAbstractSpinBox*>(preview);
       spin && (componentSet.id=="458:155" || componentSet.id=="460:175")) {
        const bool decimal=qobject_cast<QDoubleSpinBox*>(spin)!=nullptr;
        // This inspector is deliberately narrow. Keep every field below its label;
        // long variant names remain available in the popup and tooltip.
        controls->setRowWrapPolicy(QFormLayout::WrapAllRows);
        controls->setContentsMargins(12,12,12,12);
        variant->setMinimumContentsLength(1);
        variant->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        content->hide();if(auto* label=controls->labelForField(content))label->hide();
        auto* mode=new QComboBox(settings);mode->setObjectName("spinControlsControl");
        mode->addItems({"UpDown","PlusMinus","SplitPlusMinus","None"});controls->addRow("Controls",mode);
        auto* snapshot=new QCheckBox(QString::fromUtf8("固定 Figma 焦点外观"),settings);
        snapshot->setObjectName("spinSnapshotControl");controls->addRow(snapshot);
        auto* readOnly=new QCheckBox("readOnly",settings);readOnly->setObjectName("spinReadOnlyControl");controls->addRow(readOnly);
        auto* minimum=new QDoubleSpinBox(settings);minimum->setObjectName("spinMinimumControl");
        auto* maximum=new QDoubleSpinBox(settings);maximum->setObjectName("spinMaximumControl");
        auto* value=new QDoubleSpinBox(settings);value->setObjectName("spinValueControl");
        auto* step=new QDoubleSpinBox(settings);step->setObjectName("spinStepControl");
        for(auto* number:{minimum,maximum,value,step}) {
            number->setDecimals(decimal?2:0);number->setRange(decimal?-1e12:-2147483647.0,decimal?1e12:2147483647.0);
            number->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
        }
        step->setMinimum(0);
        controls->addRow(QString::fromUtf8("最小值"),minimum);controls->addRow(QString::fromUtf8("最大值"),maximum);
        controls->addRow(QString::fromUtf8("当前值"),value);controls->addRow(QString::fromUtf8("步长"),step);
        auto* decimals=new QSpinBox(settings);decimals->setObjectName("spinDecimalsControl");decimals->setRange(0,323);
        if(decimal)controls->addRow("decimals",decimals);else decimals->hide();
        auto* prefix=new QLineEdit(decimal?"$":QString::fromUtf8("¥"),settings);prefix->setObjectName("spinPrefixControl");
        auto* suffix=new QLineEdit(decimal?" %":" units",settings);suffix->setObjectName("spinSuffixControl");
        auto* showPrefix=new QCheckBox(QString::fromUtf8("显示前缀"),settings);showPrefix->setChecked(true);showPrefix->setObjectName("spinShowPrefixControl");
        auto* showSuffix=new QCheckBox(QString::fromUtf8("显示后缀"),settings);showSuffix->setChecked(true);showSuffix->setObjectName("spinShowSuffixControl");
        controls->addRow(showPrefix,prefix);controls->addRow(showSuffix,suffix);
        auto* locale=new QComboBox(settings);locale->setObjectName("spinLocaleControl");locale->addItems({"C","en_US","de_DE"});
        controls->addRow("locale",locale);spin->setLocale(QLocale::c());
        auto* wrapping=new QCheckBox("wrapping",settings);wrapping->setObjectName("spinWrappingControl");controls->addRow(wrapping);
        auto* grouping=new QCheckBox(QString::fromUtf8("显示千位分隔符"),settings);grouping->setToolTip("groupSeparatorShown");
        grouping->setObjectName("spinGroupingControl");controls->addRow(grouping);
        auto* tracking=new QCheckBox(QString::fromUtf8("输入时即时提交"),settings);tracking->setToolTip("keyboardTracking");
        tracking->setChecked(true);controls->addRow(tracking);
        auto* stepType=new QComboBox(settings);stepType->setObjectName("spinStepTypeControl");
        stepType->addItems({"DefaultStepType","AdaptiveDecimalStepType"});controls->addRow("stepType",stepType);
        for(auto* combo:{mode,locale,stepType}) {
            combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
            combo->setMinimumContentsLength(1);
            combo->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
            combo->setToolTip(combo->currentText());
            QObject::connect(combo,&QComboBox::currentTextChanged,combo,&QWidget::setToolTip);
        }
        auto* special=new QLineEdit(settings);special->setObjectName("spinSpecialValueTextControl");controls->addRow("specialValueText",special);
        auto* readout=new QLabel(settings);readout->setObjectName("spinNativeReadout");readout->setWordWrap(true);controls->addRow(readout);
        auto* note=new QLabel(QString::fromUtf8("SplitPlusMinus 使用 Qt 原生步进子控件；不新增 buttonSymbols 枚举。数值仍为 int/double，不是精确十进制金额。原生编辑中的光标、选区、输入法由 Qt 绘制；非编辑态显示源稿的前后缀字级。"),settings);
        note->setWordWrap(true);controls->addRow(note);
        const auto sync=[=] {
            const QSignalBlocker a(minimum),b(maximum),c(value),d(step),e(readOnly),f(mode),g(decimals);
            const int digits=decimal?spin->property("decimals").toInt():0;
            for(auto* number:{minimum,maximum,value,step})number->setDecimals(digits);
            minimum->setValue(spin->property("minimum").toDouble());maximum->setValue(spin->property("maximum").toDouble());
            value->setRange(minimum->value(),maximum->value());value->setValue(spin->property("value").toDouble());
            step->setValue(spin->property("singleStep").toDouble());decimals->setValue(digits);
            readOnly->setChecked(spin->isReadOnly());mode->setCurrentText(spin->property("figmaSpinControls").toString());
            readout->setText(QString("raw value = %1\ntext = %2\nacceptableInput = %3")
                .arg(spin->property("value").toString(),spin->text(),spin->hasAcceptableInput()?"true":"false"));
        };
        const auto appearance=[=] {
            commonWidget::applyFigmaSpinBoxAppearance(spin,mode->currentText(),snapshot->isChecked()?"Focus":"");sync();
        };
        for(const auto& pair:QList<QPair<QDoubleSpinBox*,const char*>>{{minimum,"minimum"},{maximum,"maximum"},{value,"value"},{step,"singleStep"}})
            QObject::connect(pair.first,qOverload<double>(&QDoubleSpinBox::valueChanged),spin,[=](double n){spin->setProperty(pair.second,n);sync();});
        QObject::connect(decimals,qOverload<int>(&QSpinBox::valueChanged),spin,[=](int n){spin->setProperty("decimals",n);sync();});
        const auto affixes=[=] {
            spin->setProperty("prefix",showPrefix->isChecked()?prefix->text():QString());
            spin->setProperty("suffix",showSuffix->isChecked()?suffix->text():QString());sync();
        };
        QObject::connect(prefix,&QLineEdit::textChanged,spin,[=]{affixes();});QObject::connect(suffix,&QLineEdit::textChanged,spin,[=]{affixes();});
        QObject::connect(showPrefix,&QCheckBox::toggled,spin,[=]{affixes();});QObject::connect(showSuffix,&QCheckBox::toggled,spin,[=]{affixes();});
        QObject::connect(mode,&QComboBox::currentTextChanged,spin,[=]{appearance();});
        QObject::connect(snapshot,&QCheckBox::toggled,spin,[=](bool on){spin->setProperty("figmaSpinFixedFocus",on);appearance();});
        QObject::connect(readOnly,&QCheckBox::toggled,spin,[=](bool on){spin->setReadOnly(on);appearance();});
        QObject::connect(locale,&QComboBox::currentTextChanged,spin,[=](const QString& name){spin->setLocale(QLocale(name));sync();});
        QObject::connect(wrapping,&QCheckBox::toggled,spin,&QAbstractSpinBox::setWrapping);
        QObject::connect(grouping,&QCheckBox::toggled,spin,[=](bool on){spin->setGroupSeparatorShown(on);sync();});
        QObject::connect(tracking,&QCheckBox::toggled,spin,&QAbstractSpinBox::setKeyboardTracking);
        QObject::connect(stepType,qOverload<int>(&QComboBox::currentIndexChanged),spin,[=](int n){spin->setProperty("stepType",n);});
        QObject::connect(special,&QLineEdit::textChanged,spin,[=](const QString& text){spin->setSpecialValueText(text);sync();});
        QObject::connect(spin->findChild<QLineEdit*>(),&QLineEdit::textChanged,spin,[=]{sync();});
        QObject::connect(variant,&QComboBox::currentTextChanged,spin,[=]{QTimer::singleShot(0,spin,sync);});
        affixes();
    }
    if (auto* radio = qobject_cast<QRadioButton*>(preview); radio && componentSet.id == "341:53") {
        radio->setText("Radio option");content->setText(radio->text());
        auto* checked = new QCheckBox(QString::fromUtf8("选中"), settings);
        checked->setObjectName("radioCheckedControl");controls->addRow(checked);
        QObject::connect(checked, &QCheckBox::toggled, radio, [radio](bool value) {
            const bool exclusive = radio->autoExclusive();
            radio->setAutoExclusive(false);radio->setChecked(value);radio->setAutoExclusive(exclusive);
        });
        QObject::connect(radio, &QRadioButton::toggled, checked, [checked](bool value) {
            const QSignalBlocker block(checked);checked->setChecked(value);
        });
        auto* showLabel = new QCheckBox(QString::fromUtf8("显示文本"), settings);
        showLabel->setObjectName("radioShowLabelControl");showLabel->setChecked(true);controls->addRow(showLabel);
        auto* snapshot = new QCheckBox(QString::fromUtf8("固定状态预览"), settings);
        snapshot->setObjectName("radioSnapshotControl");controls->addRow(snapshot);
        const auto refresh = [=] {
            commonWidget::applyFigmaRadioAppearance(radio, snapshot->isChecked() ? variant->currentText().section("State=",1) : "", showLabel->isChecked());
        };
        QObject::connect(showLabel,&QCheckBox::toggled,radio,[=]{refresh();});
        QObject::connect(snapshot,&QCheckBox::toggled,radio,[=]{refresh();});
        QObject::connect(variant,&QComboBox::currentTextChanged,radio,[=]{refresh();});
        auto* note = new QLabel(QString::fromUtf8("默认跟随鼠标与键盘；固定状态仅用于外观比对，不伪造 pressed/focus 信号。"),settings);
        note->setWordWrap(true);controls->addRow(note);
        refresh();
    }
    if (auto* progress = qobject_cast<QProgressBar*>(preview)) {
        progress->setFormat("%p%");
        content->setEnabled(true);content->setText(progress->format());
        if(auto* fieldLabel=qobject_cast<QLabel*>(controls->labelForField(content)))
            fieldLabel->setText(QString::fromUtf8("文本格式"));
        QObject::connect(content,&QLineEdit::textChanged,progress,&QProgressBar::setFormat);
        QObject::connect(content,&QLineEdit::textEdited,progress,[progress]{progress->setProperty("progressFormatEdited",true);});
        auto* minimum=new QSpinBox(settings);minimum->setObjectName("progressMinimumControl");
        auto* maximum=new QSpinBox(settings);maximum->setObjectName("progressMaximumControl");
        auto* value=new QSpinBox(settings);value->setObjectName("progressValueControl");
        for(auto* number:{minimum,maximum,value})number->setRange(-2147483647,2147483647);
        controls->addRow(QString::fromUtf8("最小值"),minimum);
        controls->addRow(QString::fromUtf8("最大值"),maximum);
        controls->addRow(QString::fromUtf8("当前值"),value);
        auto* showText=new QCheckBox(QString::fromUtf8("显示文本"),settings);
        showText->setObjectName("progressTextVisibleControl");showText->setChecked(true);controls->addRow(showText);
        progress->setProperty("progressShowLabelPreference",true);
        QObject::connect(showText,&QCheckBox::toggled,progress,[progress](bool on){
            progress->setProperty("progressShowLabelPreference",on);
            const auto state=progress->property("figmaProgressState").toString();
            progress->setTextVisible(state!="Indeterminate" && on);
        });
        auto* reduce=new QCheckBox(QString::fromUtf8("减少动态效果"),settings);
        reduce->setObjectName("progressReducedMotionControl");controls->addRow(reduce);
        QObject::connect(reduce,&QCheckBox::toggled,progress,[progress](bool on){progress->setProperty("reducedMotion",on);progress->update();});
        auto* note=new QLabel(QString::fromUtf8("busy 不显示百分比；动画节奏为运行时预览，非设计验收。Completed 为外观状态，不改写数值。"),settings);
        note->setWordWrap(true);controls->addRow(note);
        const auto sync=[=]{
            const QSignalBlocker a(minimum),b(maximum),c(value),d(content);
            minimum->setValue(progress->minimum());maximum->setValue(progress->maximum());
            value->setValue(progress->value());content->setText(progress->format());
        };
        QObject::connect(minimum,qOverload<int>(&QSpinBox::valueChanged),progress,[=](int n){progress->setMinimum(n);sync();});
        QObject::connect(maximum,qOverload<int>(&QSpinBox::valueChanged),progress,[=](int n){progress->setMaximum(n);sync();});
        QObject::connect(value,qOverload<int>(&QSpinBox::valueChanged),progress,[=](int n){progress->setValue(n);sync();});
        QObject::connect(progress,&QProgressBar::valueChanged,page,[=]{sync();});
        QObject::connect(variant,&QComboBox::currentTextChanged,page,[=]{QTimer::singleShot(0,page,sync);});
        sync();
    }
    watchObjectTree(page, objectTree, preview);
    auto apply=[=] {
        if(!variant->currentText().isEmpty())applyVariant(preview,componentSet,variant->currentText());
        const QSignalBlocker block(enabled);
        enabled->setChecked(preview->isEnabled());
    };
    QObject::connect(variant,&QComboBox::currentTextChanged,page,[=]{apply();});
    apply();
    return page;
}

QWidget* createRadioGroupAppearance(const DesignComponentSet& componentSet,
                                   const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                                   QWidget* parent) {
    auto* page = new QWidget(parent);
    auto* layout = new QVBoxLayout(page);
    auto* note = new QLabel(QString::fromUtf8(
        "ComponentLab/RadioGroup：QButtonGroup + 布局 + commonWidget::Z_RadioButton。"
        "这是可运行的组合示例；计划公共适配器仍为合同态，设计未审批。"),page);
    note->setWordWrap(true);layout->addWidget(note);
    auto* viewport = new QScrollArea(page);viewport->setWidgetResizable(true);
    auto* splitter = new QSplitter(viewport);viewport->setWidget(splitter);layout->addWidget(viewport,1);
    auto* settingsScroll = new QScrollArea(splitter);settingsScroll->setWidgetResizable(true);
    settingsScroll->setMinimumWidth(230);settingsScroll->setMaximumWidth(340);
    auto* settings = new QWidget(settingsScroll);settingsScroll->setWidget(settings);
    auto* form = new QFormLayout(settings);form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    auto* variant = new QComboBox(settings);variant->setObjectName("radioGroupVariantControl");
    variant->addItems(componentSet.variants);variant->setMinimumContentsLength(15);
    variant->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    form->addRow("Figma Variant",variant);
    auto* enabled = new QCheckBox(QString::fromUtf8("启用组合"),settings);
    enabled->setObjectName("radioGroupEnabledControl");enabled->setChecked(true);form->addRow(enabled);
    auto* selected = new QSpinBox(settings);selected->setObjectName("radioGroupSelectedIdControl");
    selected->setRange(1,3);selected->setValue(1);form->addRow("checkedId",selected);
    auto* area = new QScrollArea(splitter);area->setMinimumWidth(320);area->setWidgetResizable(true);
    auto* host = new QWidget(area);auto* hostLayout = new QVBoxLayout(host);
    auto* preview = createRadioGroupPreview(factory,false,true,false,host);
    preview->setObjectName("radioGroupInteractivePreview");
    preview->setProperty("figmaComponentSetId",componentSet.id);
    hostLayout->addWidget(preview,0,Qt::AlignTop|Qt::AlignLeft);hostLayout->addStretch();area->setWidget(host);
    auto* group = preview->findChild<QButtonGroup*>("radioExclusiveGroup");
    for (int id=1; id<=3; ++id) {
        auto* label = new QLineEdit(group->button(id)->text(),settings);
        label->setObjectName(QString("radioGroupLabel%1Control").arg(id));
        form->addRow(QString("Option %1").arg(id),label);
        QObject::connect(label,&QLineEdit::textChanged,group->button(id),&QAbstractButton::setText);
    }
    auto* objectTree = new QTreeWidget(splitter);objectTree->setObjectName("componentObjectTree");
    objectTree->setHeaderLabels({QString::fromUtf8("节点 / 属性"),QString::fromUtf8("类型 / 值")});
    objectTree->setMinimumWidth(230);objectTree->setColumnWidth(0,115);
    splitter->setSizes({230,600,270});
    QObject::connect(selected,qOverload<int>(&QSpinBox::valueChanged),preview,[=](int id){group->button(id)->setChecked(true);});
    QObject::connect(group,&QButtonGroup::idToggled,selected,[=](int,bool){
        const QSignalBlocker block(selected);selected->setValue(group->checkedId());
    });
    QObject::connect(enabled,&QCheckBox::toggled,preview,&QWidget::setEnabled);
    const auto apply = [=] {
        const bool vertical=variant->currentText().contains("Vertical");
        static_cast<QBoxLayout*>(preview->layout())->setDirection(vertical ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
        preview->setMinimumSize(vertical ? QSize(196,136) : QSize(580,48));
        preview->setMaximumSize(vertical ? QSize(196,136) : QSize(580,48));
        enabled->setChecked(!variant->currentText().contains("Disabled"));
        preview->setProperty("figmaVariant",variant->currentText());
    };
    QObject::connect(variant,&QComboBox::currentTextChanged,page,[=]{apply();});apply();
    watchObjectTree(page,objectTree,preview);
    return page;
}

QWidget* createContractPage(const DesignComponentDescriptor& descriptor, QWidget* parent) {
    auto* page = new QWidget(parent);
    const auto stableName = descriptor.sdkClass.isEmpty()
        ? descriptor.designComponent.section('/', -1)
        : descriptor.sdkClass.section("::", -1);
    page->setObjectName("componentContractPage_" + stableName);
    page->setProperty("contractOnly", true);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 24, 24, 24);
    auto* title = new QLabel(descriptor.designComponent.section('/', -1), page);
    title->setObjectName("componentPageTitle");
    layout->addWidget(title);
    auto* notice = new QLabel(QString::fromUtf8(
        "Figma 设计合同已登记；Qt 运行时组件尚未实现。本页不创建占位控件，避免把设计完成误报为可运行。"), page);
    notice->setObjectName("componentContractNotice");
    notice->setWordWrap(true);
    layout->addWidget(notice);
    auto* formHost = new QWidget(page);
    auto* form = new QFormLayout(formHost);
    form->addRow(QString::fromUtf8("设计身份"), new QLabel(descriptor.designComponent, formHost));
    form->addRow(QString::fromUtf8("计划 SDK 类"), new QLabel(descriptor.sdkClass, formHost));
    form->addRow(QString::fromUtf8("结构类型"), new QLabel(descriptor.mappingKind, formHost));
    form->addRow(QString::fromUtf8("Qt 基类 / 结构"), new QLabel(descriptor.qtClass, formHost));
    form->addRow(QString::fromUtf8("Figma 页面"), new QLabel(descriptor.pageId, formHost));
    form->addRow(QString::fromUtf8("组件集"), new QLabel(descriptor.componentSetId, formHost));
    form->addRow(QString::fromUtf8("变体数量"), new QLabel(QString::number(descriptor.variantCount), formHost));
    form->addRow(QString::fromUtf8("运行状态"), new QLabel(descriptor.runtimeStatus, formHost));
    form->addRow(QString::fromUtf8("审批状态"), new QLabel(descriptor.approvalStatus, formHost));
    layout->addWidget(formHost);
    layout->addStretch(1);
    return page;
}

QMap<QString, QString> variantProperties(const QString& variant) {
    QMap<QString, QString> properties;
    for (const auto& part : variant.split(", ", Qt::SkipEmptyParts)) {
        const int separator = part.indexOf('=');
        if (separator > 0)
            properties.insert(part.left(separator), part.mid(separator + 1));
    }
    return properties;
}

QString mappedDesignComponent(const QString& componentSetName) {
    QString name = componentSetName;
    while (name.startsWith('.') || name.startsWith('_'))
        name.remove(0, 1);
    if (name.startsWith("DSM/Button/"))
        return "ComponentLab/Button";
    if (name == "DSM/Checkbox")
        return "ComponentLab/Checkbox";
    if (name == "DSM/Input/ActionSlot")
        return {};
    if (name == "DSM/Input" || name.startsWith("DSM/Input/"))
        return "ComponentLab/Input";
    if (name.startsWith("DSM/Select/Outlined") || name.startsWith("DSM/Select/Filled") ||
        name.startsWith("DSM/Select/Borderless") || name.startsWith("DSM/Select/Underlined"))
        return "ComponentLab/Select";
    if (name == "DSM/Radio")
        return "ComponentLab/RadioButton";
    if (name == "DSM/Switch")
        return "ComponentLab/Switch";
    if (name == "DSM/Chip")
        return "ComponentLab/Chip";
    if (name == "DSM/Card")
        return "ComponentLab/Card";
    if (name == "DSM/Navbar")
        return "ComponentLab/Navbar";
    if (name == "DSM/Sidebar")
        return "ComponentLab/Sidebar";
    if (!name.startsWith("ComponentLab/"))
        return {};
    return name.count('/') == 1 ? name : QString();
}

DesignComponentDescriptor descriptorForSet(
    const DesignComponentSet& componentSet,
    const QList<DesignComponentDescriptor>& descriptors,
    bool* found) {
    // These item sets are internal variant anatomy owned by public composite nodes,
    // not additional SDK identities.
    const QString designComponent = componentSet.id == "482:490"
        ? QStringLiteral("ComponentLab/Menu") : componentSet.id == "487:22"
        ? QStringLiteral("ComponentLab/MenuBar") : componentSet.id == "500:847"
        ? QStringLiteral("ComponentLab/TabWidget") : componentSet.id == "505:553"
        ? QStringLiteral("ComponentLab/ToolBox") : mappedDesignComponent(componentSet.name);
    for (const auto& descriptor : descriptors) {
        if (descriptor.designComponent == designComponent) {
            *found = true;
            return descriptor;
        }
    }
    *found = false;
    return {};
}

void applyVariant(QWidget* widget, const DesignComponentSet& componentSet,
                  const QString& variant) {
    const auto properties = variantProperties(variant);
    widget->setProperty("figmaComponentSetId", componentSet.id);
    widget->setProperty("figmaComponentSet", componentSet.name);
    widget->setProperty("figmaVariant", variant);
    for (auto it = properties.cbegin(); it != properties.cend(); ++it) {
        const QByteArray propertyName = ("figma" + it.key()).toUtf8();
        widget->setProperty(propertyName.constData(), it.value());
    }

    const QString state = properties.value("State").toLower();
    if(componentSet.id!="500:847"&&componentSet.id!="505:553"&&componentSet.id!="511:124"&&componentSet.id!="511:280")widget->setEnabled(state != "disabled");
    if(componentSet.id=="1116:18")if(auto* line=qobject_cast<QLineEdit*>(widget))
        commonWidget::applyFigmaLineEditAppearance(line,properties.value("Size"),line->property("figmaLineEditSnapshotState").toString());
    if(componentSet.id=="463:136" || componentSet.id=="464:111" || componentSet.id=="465:189")
        if(auto* date=qobject_cast<QDateTimeEdit*>(widget)) {
            date->setReadOnly(state=="readonly");
            if(componentSet.id=="465:189") {
                const bool seconds=date->property("figmaDateShowSeconds").toBool();
                commonWidget::setDateTimeDisplayFormatPreservingValue(date,properties.value("Format")=="12h"?
                    (seconds?"hh:mm:ss AP":"hh:mm AP"):(seconds?"HH:mm:ss":"HH:mm"));
            } else date->setCalendarPopup(properties.value("Calendar")=="Available");
            commonWidget::applyFigmaDateTimeAppearance(date,date->property("figmaDateFixedFocus").toBool()?"Focus":"",
                componentSet.id=="463:136"&&date->property("figmaDateShowTime").toBool()&&date->property("figmaDateShowSpec").toBool()?
                    date->property("figmaDateSpecText").toString():QString());
        }
    if((componentSet.id=="458:155" || componentSet.id=="460:175"))
        if(auto* spin=qobject_cast<QAbstractSpinBox*>(widget)) {
            spin->setReadOnly(state=="readonly");
            commonWidget::applyFigmaSpinBoxAppearance(spin,properties.value("Controls"),
                spin->property("figmaSpinFixedFocus").toBool()?"Focus":"");
        }
    if(componentSet.id=="414:39") {
        if(auto* label=qobject_cast<QLabel*>(widget))
            commonWidget::configureFigmaLabel(label,properties);
    }
    if (auto* button = qobject_cast<QAbstractButton*>(widget))
        if (!qobject_cast<QRadioButton*>(button) && componentSet.id!="450:102" && componentSet.id!="443:222") button->setDown(state == "pressed" || state == "active");
    if (auto* edit = qobject_cast<QLineEdit*>(widget);edit&&componentSet.id!="1116:18")
        edit->setReadOnly(state == "readonly");
    if (auto* edit = qobject_cast<QTextEdit*>(widget))
        edit->setReadOnly(state == "readonly");
    if (auto* edit = qobject_cast<QPlainTextEdit*>(widget))
        edit->setReadOnly(state == "readonly");

    if (auto* button = qobject_cast<QAbstractButton*>(widget)) {
        const QString value = properties.value("Value").toLower();
        const bool checked = value == "checked" || value == "selected" || value == "on" ||
                             state == "checked" || state.startsWith("checked");
        if (componentSet.id!="443:222" && (!value.isEmpty() || state.startsWith("checked"))) {
            button->setCheckable(true);
            const bool exclusive = button->autoExclusive();
            if (qobject_cast<QRadioButton*>(button)) button->setAutoExclusive(false);
            button->setChecked(checked);
            button->setAutoExclusive(exclusive);
        }
        if (auto* checkbox = qobject_cast<QCheckBox*>(button); checkbox && !qobject_cast<commonWidget::Z_CheckBox*>(button)) {
            const bool indeterminate = value == "indeterminate";
            checkbox->setTristate(indeterminate);
            if (indeterminate)
                checkbox->setCheckState(Qt::PartiallyChecked);
        }
    }

    if (auto* button = qobject_cast<commonWidget::Z_PushButton*>(widget)) {
        const QString setName = componentSet.name;
        button->setDanger(setName.endsWith("/Danger"));
        const bool iconOnly = setName.endsWith("/IconOnly");
        if (iconOnly) {
            button->setLeadingIconSource(":/icons/arrow-left.svg");
            button->setLeadingIconVisible(true);
        }
        button->setIconOnly(iconOnly);
        if (setName.endsWith("/Primary") || setName.endsWith("/Danger"))
            button->setType(commonWidget::ButtonType::Primary);
        else if (setName.endsWith("/Dashed"))
            button->setType(commonWidget::ButtonType::Dashed);
        else if (setName.endsWith("/Text"))
            button->setType(commonWidget::ButtonType::Text);
        else if (setName.endsWith("/Link"))
            button->setType(commonWidget::ButtonType::Link);
        else
            button->setType(commonWidget::ButtonType::Default);
        const QString size = properties.value("Size").toLower();
        if (size == "xs") button->setSize(commonWidget::ButtonSize::XS);
        else if (size == "small") button->setSize(commonWidget::ButtonSize::S);
        else if (size == "large") button->setSize(commonWidget::ButtonSize::L);
        else button->setSize(commonWidget::ButtonSize::M);
        button->setLoading(state == "loading");
    }
    if (auto* checkbox = qobject_cast<commonWidget::Z_CheckBox*>(widget)) {
        checkbox->setControlSize(properties.value("Size").toLower());
        checkbox->setIndeterminate(
            properties.value("Value").compare("Indeterminate", Qt::CaseInsensitive) == 0);
    }
    if (auto* input = qobject_cast<commonWidget::Z_InputField*>(widget)) {
        const QString size = properties.value("Size").toLower();
        input->setControlSize(size == "small" ? "sm" : size == "large" ? "lg" : "md");
        if (componentSet.name.contains("Password", Qt::CaseInsensitive))
            input->setKind(commonWidget::Z_InputField::Kind::Password);
        else if (componentSet.name.contains("Search", Qt::CaseInsensitive))
            input->setKind(commonWidget::Z_InputField::Kind::Search);
        else if (componentSet.name.contains("TextArea", Qt::CaseInsensitive))
            input->setKind(commonWidget::Z_InputField::Kind::TextArea);
        const QString lowerName = componentSet.name.toLower();
        for (const auto& appearance : {"outlined", "filled", "borderless", "underlined"}) {
            if (lowerName.contains(appearance))
                input->setVariant(appearance);
        }
        input->setStatus(lowerName.contains("error") || state == "error" ? "error" :
                         lowerName.contains("warning") ? "warning" : QString());
        input->setReadOnly(state == "readonly");
        input->setLoading(state == "loading");
        if (state == "filled")
            input->setValue("LOT-2026-0930");
    }
    if (auto* select = qobject_cast<commonWidget::Z_Select*>(widget)) {
        const QString size = properties.value("Size").toLower();
        select->setControlSize(size == "small" ? "sm" : size == "large" ? "lg" : "md");
        const QString lowerName = componentSet.name.toLower();
        for (const auto& appearance : {"outlined", "filled", "borderless", "underlined"}) {
            if (lowerName.contains(appearance))
                select->setVariant(appearance);
        }
        select->setStatus(lowerName.contains("error") ? "error" :
                          lowerName.contains("warning") ? "warning" : QString());
        select->setOptions({{"etch-01", "ETCH-01", {}, false},
                            {"cvd-02", "CVD-02", {}, false},
                            {"cmp-03", "CMP-03", {}, false}});
        select->setValues({"etch-01"});
    }
    if (auto* badge = qobject_cast<commonWidget::Z_Badge*>(widget)) {
        badge->setKind(properties.value("Kind"));
        badge->setTone(properties.value("Tone"));
        badge->resize(badge->sizeHint());
    }
    if (auto* indicator = qobject_cast<commonWidget::Z_StatusIndicator*>(widget)) {
        indicator->setSize(properties.value("Size"));
        indicator->setTone(properties.value("Tone"));
        indicator->resize(indicator->sizeHint());
    }
    if (auto* link = qobject_cast<commonWidget::Z_Link*>(widget)) {
        link->setSnapshotState(properties.value("State"));
        link->resize(link->sizeHint());
    }
    if (auto* tag = qobject_cast<commonWidget::Z_Tag*>(widget)) {
        tag->setSize(properties.value("Size"));
        tag->setTone(properties.value("Tone"));
        tag->setDismissible(properties.value("Dismissible").compare(
                                "True", Qt::CaseInsensitive) == 0);
        tag->resize(tag->sizeHint());
    }
    if (auto* tooltip = qobject_cast<commonWidget::Z_Tooltip*>(widget)) {
        tooltip->setPlacement(properties.value("Placement"));
        tooltip->resize(tooltip->sizeHint());
    }
    if (auto* search = qobject_cast<commonWidget::Z_SearchInput*>(widget)) {
        search->setSnapshotState(properties.value("State"));
        search->setValue(properties.value("State") == "Filled"
                             ? QStringLiteral("ETCH-01") : QString());
        search->resize(search->sizeHint());
    }
    if (auto* number = qobject_cast<commonWidget::Z_NumberInput*>(widget)) {
        number->setType(properties.value("Type"));
        number->setSnapshotState(properties.value("State"));
        number->setValue(properties.value("Type") == "Decimal" ? 12.5 : 12.0);
        number->resize(number->sizeHint());
    }
    if (auto* colorPicker = qobject_cast<commonWidget::Z_ColorPicker*>(widget)) {
        colorPicker->setColor(QColor(QStringLiteral("#5B5BD6")));
        colorPicker->setSnapshotState(properties.value("State"));
        colorPicker->resize(colorPicker->sizeHint());
    }
    if (auto* transfer = qobject_cast<commonWidget::Z_Transfer*>(widget)) {
        auto* available = new QStandardItemModel(transfer);
        for (const auto& label : {QStringLiteral("□ Chamber A"),
                                  QStringLiteral("□ Chamber B"),
                                  QStringLiteral("□ Chamber C")})
            available->appendRow(new QStandardItem(label));
        auto* selected = new QStandardItemModel(transfer);
        selected->appendRow(new QStandardItem(
            properties.value("State") == QStringLiteral("Loading")
                ? QStringLiteral("Synchronizing")
                : QStringLiteral("□ Chamber D")));
        transfer->setAvailableModel(available);
        transfer->setSelectedModel(selected);
        transfer->setSnapshotState(properties.value("State"));
        transfer->resize(transfer->sizeHint());
    }
    if (auto* upload = qobject_cast<commonWidget::Z_Upload*>(widget)) {
        upload->setSnapshotState(properties.value("State"));
        upload->setFileName(QStringLiteral("recipe"));
        upload->setProgress(48);
        upload->setHelperText(QStringLiteral("CSV, JSON · max 20 MB"));
        upload->resize(upload->sizeHint());
    }
    if (auto* keypad = qobject_cast<commonWidget::Z_NumericKeypad*>(widget)) {
        keypad->setDensity(properties.value("Density"));
        keypad->resize(keypad->sizeHint());
    }
    if (auto* overflow = qobject_cast<commonWidget::Z_OverflowMenuButton*>(widget)) {
        overflow->setSnapshotState(properties.value("State"));
        overflow->resize(overflow->sizeHint());
    }
    if (auto* segmented = qobject_cast<commonWidget::Z_SegmentedControl*>(widget)) {
        const int count = properties.value("Segments").toInt();
        segmented->setSegmentCount(count);
        segmented->setCurrentIndex(qMax(0, count - 2));
        segmented->resize(segmented->sizeHint());
    }
    if (auto* stepper = qobject_cast<commonWidget::Z_Stepper*>(widget)) {
        stepper->setSnapshotState(properties.value("State"));
        stepper->resize(stepper->sizeHint());
    }
    if (auto* timeline = qobject_cast<commonWidget::Z_Timeline*>(widget)) {
        timeline->setDensity(properties.value("Density"));
        timeline->resize(timeline->sizeHint());
    }
    if (auto* anchor = qobject_cast<commonWidget::Z_AnchorNavigation*>(widget)) {
        anchor->setOrientation(properties.value("Orientation"));
        anchor->setSnapshotState(properties.value("State"));
        anchor->resize(anchor->sizeHint());
    }
    if (auto* filter = qobject_cast<commonWidget::Z_Filter*>(widget)) {
        filter->setLayoutMode(properties.value("Layout"));
        filter->setSnapshotState(properties.value("State"));
        const bool applied = properties.value("State") == QStringLiteral("Applied");
        filter->setCriteria(applied
            ? QStringList{QStringLiteral("Material: Si"), QStringLiteral("Status")}
            : QStringList{QStringLiteral("Material"), QStringLiteral("Status")});
        filter->setApplyLabel(applied ? QStringLiteral("Apply 1") : QStringLiteral("Apply"));
        filter->resize(filter->sizeHint());
    }
    if (auto* pagination = qobject_cast<commonWidget::Z_Pagination*>(widget)) {
        pagination->setDensity(properties.value("Density"));
        pagination->setSnapshotState(properties.value("State"));
        pagination->setPageCount(3);
        pagination->setCurrentPage(2);
        pagination->resize(pagination->sizeHint());
    }
    if (auto* pageHeader = qobject_cast<commonWidget::Z_PageHeader*>(widget)) {
        pageHeader->setDensity(properties.value("Density"));
        pageHeader->setActionsVisible(properties.value("Actions") == "On");
        pageHeader->resize(pageHeader->sizeHint());
    }
    if (auto* titleBar = qobject_cast<commonWidget::Z_TitleBar*>(widget)) {
        titleBar->setSnapshotState(properties.value("State"));
        titleBar->resize(titleBar->sizeHint());
    }

    const QString orientation = properties.value("Orientation").toLower();
    if(componentSet.id=="475:551")if(auto* dial=qobject_cast<QDial*>(widget)) {
        const bool wrapping=properties.value("Wrapping")=="On";
        // Avoid Qt5's modulo-zero wrapping boundary without altering SDK behavior.
        if(wrapping&&dial->minimum()==dial->maximum()){if(dial->maximum()==1000000)dial->setMinimum(999999);else dial->setMaximum(dial->minimum()+1);}
        dial->setWrapping(wrapping);
        commonWidget::applyFigmaDialAppearance(dial,dial->property("figmaDialSnapshotState").toString(),false,
            dial->property("figmaDialShowValue").toBool(),dial->property("figmaDialAutomaticLabel").toBool()?QString():dial->property("figmaDialValueLabel").toString());
        dial->setFixedSize(180,180);
    }
    if (auto* slider = qobject_cast<QAbstractSlider*>(widget)) {
        if (orientation == "vertical")
            slider->setOrientation(Qt::Vertical);
        else if (orientation == "horizontal")
            slider->setOrientation(Qt::Horizontal);
        if(componentSet.id=="467:195")
            if(auto* native=qobject_cast<QSlider*>(slider))commonWidget::applyFigmaSliderAppearance(native,
                native->property("figmaSliderSnapshotState").toString(),false,native->property("figmaSliderShowValue").toBool(),
                native->property("figmaSliderAutomaticLabel").toBool()?QString():native->property("figmaSliderValueLabel").toString());
        if(componentSet.id=="471:477")if(auto* bar=qobject_cast<QScrollBar*>(slider)) {
            commonWidget::applyFigmaScrollBarAppearance(bar,bar->property("figmaScrollBarSnapshotState").toString(),false,bar->property("figmaScrollBarShowLineButtons").toBool());
            if(bar->orientation()==Qt::Horizontal){bar->setMinimumSize(280,56);bar->setMaximumSize(QWIDGETSIZE_MAX,56);bar->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);}
            else{bar->setMinimumSize(100,260);bar->setMaximumSize(100,QWIDGETSIZE_MAX);bar->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Expanding);}
        }
    }
    if (auto* splitter = qobject_cast<QSplitter*>(widget)) {
        if (orientation == "vertical")
            splitter->setOrientation(Qt::Vertical);
        else if (orientation == "horizontal")
            splitter->setOrientation(Qt::Horizontal);
    }
    if (auto* toolbar = qobject_cast<QToolBar*>(widget)) {
        if (orientation == "vertical")
            toolbar->setOrientation(Qt::Vertical);
        else if (orientation == "horizontal")
            toolbar->setOrientation(Qt::Horizontal);
    }
    if (auto* progress = qobject_cast<QProgressBar*>(widget)) {
        if (componentSet.id=="410:57") {
            auto progressProperties=properties;
            progressProperties.insert("Show label",
                progress->property("progressShowLabelPreference").toBool()?"True":"False");
            commonWidget::configureFigmaProgressBar(progress,progressProperties);
            progress->setMinimumSize(progress->orientation()==Qt::Vertical ? QSize(32,200) : QSize(270,24));
            progress->setMaximumSize(progress->orientation()==Qt::Vertical ? QSize(32,200) : QSize(QWIDGETSIZE_MAX,24));
            if(!progress->property("progressFormatEdited").toBool()) {
                const auto caption=properties.value("State");
                progress->setFormat(caption=="In-progress" ? "In progress" :
                    QStringList{"Paused","Error","Completed"}.contains(caption) ? caption : "%p%");
            }
        }
    }

    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    if(componentSet.id=="507:443")if(auto* dock=qobject_cast<QDockWidget*>(widget)) {
        commonWidget::applyFigmaDockWidgetAppearance(dock,properties.value("Area"),properties.value("Placement"),
            dock->property("figmaDockShowCloseGlyph").toBool(),dock->property("figmaDockShowFloatGlyph").toBool());
    }
    if(componentSet.id=="505:553")if(auto* box=qobject_cast<QToolBox*>(widget)) {
        const int target=box->property("toolBoxDemoTarget").toInt();box->setItemEnabled(target,state!="disabled");
        commonWidget::applyFigmaToolBoxAppearance(box,properties.value("State"),target,box->property("figmaToolBoxShowMnemonic").toBool());
    }
    if(componentSet.id=="500:847")if(auto* tabs=qobject_cast<QTabWidget*>(widget)) {
        const auto position=properties.value("Position");
        tabs->setTabPosition(position=="Bottom"?QTabWidget::South:position=="Left"?QTabWidget::West:position=="Right"?QTabWidget::East:QTabWidget::North);
        const int target=tabs->property("tabDemoTarget").toInt();
        tabs->setTabEnabled(target,state!="disabled");
        commonWidget::applyFigmaTabWidgetAppearance(tabs,properties.value("State"),target);
    }
    if(componentSet.id=="447:593")if(auto* box=qobject_cast<QDialogButtonBox*>(widget)) {
        configureDialogActions(box,properties.value("Configuration"));box->setOrientation(properties.value("Orientation")=="Vertical"?Qt::Vertical:Qt::Horizontal);box->setCenterButtons(properties.value("Centered")=="True");commonWidget::applyFigmaDialogButtonBoxAppearance(box);
    }
    if(componentSet.id=="443:222")if(auto* tool=qobject_cast<QToolButton*>(widget)) {
        tool->setAutoRaise(properties.value("Surface")=="AutoRaise");tool->setPopupMode(properties.value("Menu")=="Split"?QToolButton::MenuButtonPopup:QToolButton::DelayedPopup);
        if(auto* action=tool->defaultAction()){action->setEnabled(state!="disabled");action->setCheckable(state.startsWith("checked"));action->setChecked(state.startsWith("checked"));}
        else{tool->setCheckable(state.startsWith("checked"));tool->setChecked(state.startsWith("checked"));}
        commonWidget::applyFigmaToolButtonAppearance(tool,state=="disabled"?QString():properties.value("State"));
        tool->setMinimumSize(0,0);
    }
    if(componentSet.id=="450:102")if(auto* command=qobject_cast<QCommandLinkButton*>(widget)) {
        command->setFlat(properties.value("Surface")=="Flat");command->setCheckable(state.startsWith("checked"));command->setChecked(state.startsWith("checked"));
        commonWidget::applyFigmaCommandLinkAppearance(command,state=="disabled"?QString():properties.value("State"),command->property("figmaCommandShowDescription").toBool(),command->property("figmaCommandShowIcon").toBool());
    }
    // Restore captured dimensions after shell QSS polishing (native Windows timing differs).
    if(componentSet.id=="1120:18" || componentSet.id=="1121:18")if(auto* combo=qobject_cast<QComboBox*>(widget))
        commonWidget::applyFigmaComboBoxAppearance(combo,properties.value("Size"),state=="disabled"?QString():properties.value("State"));
    widget->update();
}

QWidget* createVariantCell(const DesignComponentSet& componentSet, const QString& variant,
                           const QList<DesignComponentDescriptor>& descriptors,
                           const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                           QWidget* parent) {
    auto* host = new QWidget(parent);
    host->setObjectName("variantCell");
    host->setToolTip(variant);
    auto* layout = new QVBoxLayout(host);
    layout->setContentsMargins(8, 6, 8, 6);
    bool found = false;
    const auto descriptor = descriptorForSet(componentSet, descriptors, &found);
    if (!found || !descriptor.instantiable) {
        auto* contract = new QLabel(QString::fromUtf8("设计契约"), host);
        contract->setAlignment(Qt::AlignCenter);
        contract->setProperty("contractOnly", true);
        layout->addWidget(contract);
        return host;
    }

    auto* preview = factory->create(descriptor.sdkClass, host);
    configurePreview(preview, descriptor.qtClass, descriptor.sdkClass);
    preview->setObjectName("variantPreview_" + QString(componentSet.id).replace(':', '_'));
    preview->setMinimumWidth(100);
    preview->setMaximumWidth(240);
    preview->setMaximumHeight(96);
    applyVariant(preview, componentSet, variant);
    layout->addWidget(preview, 0, Qt::AlignCenter);
    return host;
}

QString rowKey(const QMap<QString, QString>& properties, const QStringList& rowAxes) {
    QStringList values;
    for (const auto& axis : rowAxes)
        values.append(axis + "=" + properties.value(axis));
    return values.join(QString::fromUtf8(" · "));
}

QTableWidget* createVariantMatrix(
    const DesignComponentSet& componentSet,
    const QList<DesignComponentDescriptor>& descriptors,
    const std::shared_ptr<commonWidget::ComponentFactory>& factory,
    QWidget* parent) {
    auto* table = new QTableWidget(parent);
    table->setObjectName("variantMatrix_" + QString(componentSet.id).replace(':', '_'));
    table->setProperty("figmaComponentSetId", componentSet.id);
    table->setProperty("figmaVariantCount", componentSet.variants.size());
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);

    QString columnAxis = componentSet.variantAxes.isEmpty()
        ? QString()
        : componentSet.variantAxes.last().name;
    for (const auto& axis : componentSet.variantAxes) {
        if (axis.name.compare("State", Qt::CaseInsensitive) == 0) {
            columnAxis = axis.name;
            break;
        }
    }
    QStringList columnValues;
    QStringList rowAxes;
    for (const auto& axis : componentSet.variantAxes) {
        if (axis.name == columnAxis)
            columnValues = axis.values;
        else
            rowAxes.append(axis.name);
    }
    if (columnValues.isEmpty())
        columnValues.append(QString::fromUtf8("Variant"));

    QStringList rows;
    QMap<QString, QMap<QString, QString>> variants;
    for (const auto& variant : componentSet.variants) {
        const auto properties = variantProperties(variant);
        const QString row = rowAxes.isEmpty() ? componentSet.name : rowKey(properties, rowAxes);
        const QString column = columnAxis.isEmpty() ? columnValues.first()
                                                    : properties.value(columnAxis);
        if (!rows.contains(row))
            rows.append(row);
        variants[row].insert(column, variant);
    }

    table->setRowCount(rows.size());
    table->setColumnCount(columnValues.size());
    table->setHorizontalHeaderLabels(columnValues);
    table->setVerticalHeaderLabels(rows);
    for (int row = 0; row < rows.size(); ++row) {
        table->setRowHeight(row, 92);
        for (int column = 0; column < columnValues.size(); ++column) {
            const QString variant = variants.value(rows[row]).value(columnValues[column]);
            if (variant.isEmpty()) {
                auto* missing = new QTableWidgetItem(QString::fromUtf8("—"));
                missing->setTextAlignment(Qt::AlignCenter);
                missing->setFlags(Qt::NoItemFlags);
                table->setItem(row, column, missing);
            } else {
                table->setCellWidget(row, column, createVariantCell(
                    componentSet, variant, descriptors, factory, table));
            }
        }
    }
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    return table;
}

QPixmap tintedIconAsset(const QString& assetId, int extent, const QColor& color) {
    QPixmap result = commonWidget::IconSource::load(assetId).pixmap(extent, extent);
    QPainter painter(&result);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(result.rect(), color);
    return result;
}

QWidget* createIconAssetGallery(bool includeCatalog, QWidget* parent) {
    auto* root = new QWidget(parent);
    root->setObjectName(includeCatalog ? QStringLiteral("iconAssetGallery")
                                       : QStringLiteral("iconAssetVariantGallery"));
    const auto assets = commonWidget::IconAssetCatalog::descriptors();
    root->setProperty("iconAssetDescriptorCount", assets.size());
    root->setProperty("iconAssetVariantCount", 6);
    root->setProperty("iconAssetUsesSdkCatalog", true);
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(10);

    auto* contract = new QLabel(QString::fromUtf8(
        "Figma 合同：Size 16 / 20 / 24 × Tone Primary / Secondary。"
        "以下预览与 GenerateLayout 使用同一 SDK 语义资产目录。"), root);
    contract->setWordWrap(true);
    contract->setObjectName("iconAssetContract");
    layout->addWidget(contract);

    auto* variants = new QWidget(root);
    variants->setObjectName("iconAssetVariants");
    auto* grid = new QGridLayout(variants);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(8);
    const QList<int> sizes{16, 20, 24};
    const QList<QPair<QString, QColor>> tones{
        {QStringLiteral("Primary"), QColor(QStringLiteral("#6235f7"))},
        {QStringLiteral("Secondary"), QColor(QStringLiteral("#444444"))},
    };
    for (int toneIndex = 0; toneIndex < tones.size(); ++toneIndex) {
        for (int sizeIndex = 0; sizeIndex < sizes.size(); ++sizeIndex) {
            const int extent = sizes[sizeIndex];
            auto* tile = new QFrame(variants);
            tile->setObjectName(QStringLiteral("iconAsset_%1_%2")
                                    .arg(extent).arg(tones[toneIndex].first));
            tile->setProperty("assetId", QStringLiteral("icon.action.search"));
            tile->setProperty("figmaSize", extent);
            tile->setProperty("figmaTone", tones[toneIndex].first);
            tile->setFrameShape(QFrame::StyledPanel);
            tile->setMinimumSize(112, 84);
            auto* tileLayout = new QVBoxLayout(tile);
            auto* glyph = new QLabel(tile);
            glyph->setObjectName("iconAssetGlyph");
            glyph->setAlignment(Qt::AlignCenter);
            glyph->setPixmap(tintedIconAsset(QStringLiteral("icon.action.search"), extent,
                                             tones[toneIndex].second));
            tileLayout->addWidget(glyph, 1);
            auto* label = new QLabel(QStringLiteral("%1 px · %2")
                                         .arg(extent).arg(tones[toneIndex].first), tile);
            label->setAlignment(Qt::AlignCenter);
            tileLayout->addWidget(label);
            grid->addWidget(tile, toneIndex, sizeIndex);
        }
    }
    layout->addWidget(variants);

    if (includeCatalog) {
        auto* table = new QTableWidget(assets.size(), 6, root);
        table->setObjectName("iconAssetCatalogTable");
        table->setHorizontalHeaderLabels({QString::fromUtf8("预览"),
                                          QStringLiteral("Stable ID"),
                                          QString::fromUtf8("可访问名称"),
                                          QString::fromUtf8("尺寸 / 色调"),
                                          QString::fromUtf8("格式 / 可着色"),
                                          QString::fromUtf8("来源 / 许可")});
        table->verticalHeader()->hide();
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        for (int row = 0; row < assets.size(); ++row) {
            const auto& asset = assets[row];
            auto* preview = new QLabel(table);
            preview->setAlignment(Qt::AlignCenter);
            preview->setPixmap(tintedIconAsset(asset.id, 20, QColor(QStringLiteral("#6235f7"))));
            table->setCellWidget(row, 0, preview);
            table->setItem(row, 1, new QTableWidgetItem(asset.id));
            table->setItem(row, 2, new QTableWidgetItem(asset.accessibleName));
            QStringList sizesText;
            for (const int size : asset.supportedSizes)
                sizesText.append(QString::number(size));
            table->setItem(row, 3, new QTableWidgetItem(
                sizesText.join(QStringLiteral(" / ")) + QStringLiteral(" · ") +
                asset.supportedTones.join(QStringLiteral(" / "))));
            table->setItem(row, 4, new QTableWidgetItem(
                asset.format + QStringLiteral(" · ") +
                (asset.tintable ? QString::fromUtf8("可着色") : QString::fromUtf8("原色"))));
            table->setItem(row, 5, new QTableWidgetItem(asset.source + QStringLiteral(" · ") + asset.license));
        }
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        table->setMinimumHeight(assets.size() * 34 + 36);
        layout->addWidget(table, 1);
        auto* missing = new QLabel(QString::fromUtf8(
            "缺失策略：未知语义 ID 明确返回 UNMAPPED_RESOURCE；不回退为相似图标。"), root);
        missing->setObjectName("iconAssetMissingPolicy");
        layout->addWidget(missing);
    }
    return root;
}

QTableWidget* createVisualAssetMetadataTable(
    const QList<commonWidget::VisualAssetDescriptor>& assets, QWidget* parent) {
    auto* table = new QTableWidget(assets.size(), 7, parent);
    table->setObjectName("visualAssetCatalogTable");
    table->setHorizontalHeaderLabels({QStringLiteral("Stable ID"),
                                      QString::fromUtf8("类型"),
                                      QString::fromUtf8("固有尺寸"),
                                      QString::fromUtf8("状态"),
                                      QString::fromUtf8("适配"),
                                      QString::fromUtf8("格式"),
                                      QString::fromUtf8("来源 / 许可 / SHA-256")});
    table->verticalHeader()->hide();
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    for (int row = 0; row < assets.size(); ++row) {
        const auto& asset = assets[row];
        const QString defaultState = asset.kind == commonWidget::VisualAssetKind::Image
            ? QStringLiteral("Loaded") : QStringLiteral("Ready");
        table->setItem(row, 0, new QTableWidgetItem(asset.id));
        table->setItem(row, 1, new QTableWidgetItem(
            asset.kind == commonWidget::VisualAssetKind::Image
                ? QStringLiteral("Image") : QStringLiteral("Model2D")));
        table->setItem(row, 2, new QTableWidgetItem(
            QStringLiteral("%1 × %2").arg(asset.intrinsicSize.width())
                                      .arg(asset.intrinsicSize.height())));
        table->setItem(row, 3, new QTableWidgetItem(asset.supportedStates.join(" / ")));
        table->setItem(row, 4, new QTableWidgetItem(asset.supportedFits.join(" / ")));
        table->setItem(row, 5, new QTableWidgetItem(
            asset.format + (asset.scalable ? QString::fromUtf8(" · 可缩放") : QString())));
        table->setItem(row, 6, new QTableWidgetItem(
            asset.source + QStringLiteral(" · ") + asset.license + QStringLiteral(" · ") +
            commonWidget::VisualAssetCatalog::sha256(asset.id, defaultState)));
    }
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    table->setMinimumHeight(assets.size() * 34 + 36);
    return table;
}

QWidget* createImageAssetGallery(const QString& profile, bool includeCatalog, QWidget* parent) {
    auto* root = new QWidget(parent);
    root->setObjectName(includeCatalog ? QStringLiteral("imageAssetGallery")
                                       : QStringLiteral("imageAssetVariantGallery"));
    const auto assets = commonWidget::VisualAssetCatalog::imageDescriptors();
    root->setProperty("imageAssetDescriptorCount", assets.size());
    root->setProperty("imageAssetVariantCount", 6);
    root->setProperty("imageAssetUsesSdkCatalog", true);
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(10);
    auto* contract = new QLabel(QString::fromUtf8(
        "Figma 合同：Fit Contain / Cover × State Loaded / Empty / Error。"
        "Loaded 使用 SDK 稳定资产 ID；Empty 与 Error 是已知无载荷状态。"), root);
    contract->setWordWrap(true);
    layout->addWidget(contract);

    auto* variants = new QWidget(root);
    auto* grid = new QGridLayout(variants);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(8);
    const QStringList fits{QStringLiteral("Contain"), QStringLiteral("Cover")};
    const QStringList states{QStringLiteral("Loaded"), QStringLiteral("Empty"),
                             QStringLiteral("Error")};
    for (int fitIndex = 0; fitIndex < fits.size(); ++fitIndex) {
        for (int stateIndex = 0; stateIndex < states.size(); ++stateIndex) {
            const QString fit = fits[fitIndex];
            const QString state = states[stateIndex];
            auto* tile = new QFrame(variants);
            tile->setObjectName(QStringLiteral("imageAsset_%1_%2").arg(fit, state));
            tile->setProperty("assetId", QStringLiteral("image.foundation.geometric-preview"));
            tile->setProperty("figmaFit", fit);
            tile->setProperty("figmaState", state);
            tile->setFrameShape(QFrame::StyledPanel);
            tile->setFixedSize(320, 210);
            auto* tileLayout = new QVBoxLayout(tile);
            tileLayout->setContentsMargins(14, 12, 14, 12);
            tileLayout->setSpacing(10);
            auto* viewport = new QLabel(tile);
            viewport->setObjectName("imageAssetViewport");
            viewport->setAlignment(Qt::AlignCenter);
            viewport->setFixedSize(fit == QStringLiteral("Contain") ? QSize(230, 145)
                                                                      : QSize(280, 145));
            if (state == QStringLiteral("Loaded")) {
                viewport->setPixmap(commonWidget::VisualAssetCatalog::render(
                    QStringLiteral("image.foundation.geometric-preview"), state,
                    viewport->size(), fit, profile, viewport->devicePixelRatioF()));
            } else {
                viewport->setText(state == QStringLiteral("Empty")
                    ? QStringLiteral("Empty image") : QStringLiteral("Image error"));
                viewport->setStyleSheet(state == QStringLiteral("Error")
                    ? QStringLiteral("border:2px solid #e02128;border-radius:8px;color:#e02128;")
                    : QStringLiteral("border:1px solid #d0d0d0;border-radius:8px;color:#444444;"));
            }
            tileLayout->addWidget(viewport, 0, Qt::AlignHCenter);
            auto* label = new QLabel(fit + QStringLiteral(" · ") + state, tile);
            label->setAlignment(Qt::AlignCenter);
            tileLayout->addWidget(label);
            grid->addWidget(tile, fitIndex, stateIndex);
        }
    }
    layout->addWidget(variants);
    if (includeCatalog) {
        layout->addWidget(createVisualAssetMetadataTable(assets, root));
        auto* policy = new QLabel(QString::fromUtf8(
            "适配策略：Contain 完整显示，Cover 等比放大并居中裁切；SVG 按设备像素比渲染。"
            "未知 ID 明确返回 UNMAPPED_RESOURCE。"), root);
        policy->setWordWrap(true);
        layout->addWidget(policy);
    }
    return root;
}

QWidget* createModelAssetGallery(const QString& profile, bool includeCatalog, QWidget* parent) {
    auto* root = new QWidget(parent);
    root->setObjectName(includeCatalog ? QStringLiteral("modelAssetGallery")
                                       : QStringLiteral("modelAssetVariantGallery"));
    const auto assets = commonWidget::VisualAssetCatalog::modelDescriptors();
    root->setProperty("modelAssetDescriptorCount", assets.size());
    root->setProperty("modelAssetVariantCount", 3);
    root->setProperty("modelAssetUsesSdkCatalog", true);
    root->setProperty("modelAssetIs2D", true);
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(10);
    auto* contract = new QLabel(QString::fromUtf8(
        "Figma 合同：State Ready / Loading / Error。该资产是 2D 原理图基础，"
        "不代表 3D 模型、QAbstractItemModel 或运行时交互画布。"), root);
    contract->setWordWrap(true);
    layout->addWidget(contract);
    auto* variants = new QWidget(root);
    auto* row = new QHBoxLayout(variants);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(8);
    for (const QString& state : {QStringLiteral("Ready"), QStringLiteral("Loading"),
                                 QStringLiteral("Error")}) {
        auto* tile = new QFrame(variants);
        tile->setObjectName(QStringLiteral("modelAsset_%1").arg(state));
        tile->setProperty("assetId", QStringLiteral("model.foundation.schematic"));
        tile->setProperty("figmaState", state);
        tile->setFrameShape(QFrame::StyledPanel);
        tile->setFixedSize(360, 230);
        auto* tileLayout = new QVBoxLayout(tile);
        tileLayout->setContentsMargins(14, 12, 14, 12);
        tileLayout->setSpacing(8);
        auto* viewport = new QLabel(tile);
        viewport->setObjectName("modelAssetViewport");
        viewport->setAlignment(Qt::AlignCenter);
        viewport->setFixedSize(310, 156);
        viewport->setPixmap(commonWidget::VisualAssetCatalog::render(
            QStringLiteral("model.foundation.schematic"), state, viewport->size(),
            QStringLiteral("Contain"), profile, viewport->devicePixelRatioF()));
        tileLayout->addWidget(viewport, 0, Qt::AlignHCenter);
        auto* title = new QLabel(state == QStringLiteral("Ready")
            ? QStringLiteral("2D schematic ready")
            : state == QStringLiteral("Loading")
            ? QStringLiteral("Loading schematic…")
            : QStringLiteral("Schematic unavailable"), tile);
        title->setAlignment(Qt::AlignCenter);
        tileLayout->addWidget(title);
        auto* note = new QLabel(QStringLiteral("Large visualization foundation asset"), tile);
        note->setAlignment(Qt::AlignCenter);
        tileLayout->addWidget(note);
        row->addWidget(tile);
    }
    layout->addWidget(variants);
    if (includeCatalog)
        layout->addWidget(createVisualAssetMetadataTable(assets, root));
    return root;
}

QWidget* createFigmaPage(const DesignComponentPage& figmaPage,
                         const QList<DesignComponentDescriptor>& descriptors,
                         const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                         const QMap<QString, QWidget*>& detailedPages,
                         const QString& profile,
                         QWidget* parent) {
    auto* page = new QWidget(parent);
    page->setObjectName("figmaPage_" + QString(figmaPage.id).replace(':', '_'));
    page->setProperty("figmaPageId", figmaPage.id);
    page->setProperty("componentSetCount", figmaPage.componentSets.size());
    int variantCount = 0;
    for (const auto& componentSet : figmaPage.componentSets)
        variantCount += componentSet.variants.size();
    page->setProperty("variantCount", variantCount);

    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 8, 8, 8);
    auto* title = new QLabel(figmaPage.name, page);
    title->setObjectName("figmaPageTitle");
    title->setProperty("role", "title");
    layout->addWidget(title);

    auto* sections = new QTabWidget(page);
    sections->setObjectName("componentSections");
    layout->addWidget(sections, 1);

    auto* appearance = new QWidget(sections);
    auto* appearanceLayout = new QVBoxLayout(appearance);
    const QMap<QString, QString> editorClasses{
        {"172:2710", "commonWidget::Z_PushButton"},
        {"234:2717", "commonWidget::Z_CheckBox"},
        {"339:2", "commonWidget::Z_InputField"},
        {"242:2720", "commonWidget::Z_Select"}};
    auto* existingPage = detailedPages.value(editorClasses.value(figmaPage.id), nullptr);
    auto* existingSections = existingPage ? existingPage->findChild<QTabWidget*>() : nullptr;
    if (figmaPage.id == QStringLiteral("2127:3")) {
        appearanceLayout->addWidget(createIconAssetGallery(true, appearance));
    } else if (figmaPage.id == QStringLiteral("2127:4")) {
        appearanceLayout->addWidget(createImageAssetGallery(profile, true, appearance));
    } else if (figmaPage.id == QStringLiteral("2127:5")) {
        appearanceLayout->addWidget(createModelAssetGallery(profile, true, appearance));
    } else if (hasNavigationGallery(figmaPage.id)) {
        appearanceLayout->addWidget(createNavigationAppearance(figmaPage.id, appearance));
    } else if (existingSections && existingSections->count()) {
        // Keep the original editor, its signal wiring and QObject inspector alive.
        // The Figma navigation owns only presentation and must not replace this contract.
        auto* editor = existingSections->widget(0);
        existingSections->removeTab(0);
        appearanceLayout->setContentsMargins(0, 0, 0, 0);
        appearanceLayout->addWidget(editor);
        editor->show();
    } else {
    auto* summary = new QLabel(
        QString::fromUtf8("Figma 页面 %1 · %2 个组件集 · %3 个 Variant")
            .arg(figmaPage.id).arg(figmaPage.componentSets.size()).arg(variantCount),
        appearance);
    summary->setObjectName("figmaPageSummary");
    appearanceLayout->addWidget(summary);
    auto* setChoice=new QComboBox(appearance);
    setChoice->setObjectName("appearanceComponentSetControl");
    auto* editors=new QStackedWidget(appearance);
    editors->setObjectName("appearanceComponentSetEditors");
    for(const auto& componentSet:figmaPage.componentSets) {
        setChoice->addItem(componentSet.name,componentSet.id);
        bool found=false;
        const auto descriptor=descriptorForSet(componentSet,descriptors,&found);
        if(componentSet.id=="341:118")
            editors->addWidget(createRadioGroupAppearance(componentSet,factory,editors));
        else if(found && descriptor.instantiable)
            editors->addWidget(createNativeAppearance(descriptor,componentSet,factory,editors));
        else if(found)
            editors->addWidget(createContractPage(descriptor,editors));
        else {
            auto* note=new QLabel(QString::fromUtf8("%1（%2）是内部结构或尚未建立可实例化映射的设计资产；不伪造独立 SDK 类型。")
                .arg(componentSet.name,componentSet.id),editors);
            note->setWordWrap(true);editors->addWidget(note);
        }
    }
    appearanceLayout->addWidget(setChoice);
    appearanceLayout->addWidget(editors,1);
    QObject::connect(setChoice,qOverload<int>(&QComboBox::currentIndexChanged),editors,&QStackedWidget::setCurrentIndex);
    }
    sections->addTab(appearance, QString::fromUtf8("外观"));

    auto* variants = new QWidget(sections);
    auto* variantsLayout = new QVBoxLayout(variants);
    variantsLayout->setContentsMargins(0, 8, 0, 0);
    if (figmaPage.id == QStringLiteral("2127:3")) {
        variantsLayout->addWidget(createIconAssetGallery(false, variants));
        sections->addTab(variants, QString::fromUtf8("分组与状态"));
        return page;
    } else if (figmaPage.id == QStringLiteral("2127:4")) {
        variantsLayout->addWidget(createImageAssetGallery(profile, false, variants));
        sections->addTab(variants, QString::fromUtf8("分组与状态"));
        return page;
    } else if (figmaPage.id == QStringLiteral("2127:5")) {
        variantsLayout->addWidget(createModelAssetGallery(profile, false, variants));
        sections->addTab(variants, QString::fromUtf8("分组与状态"));
        return page;
    }
    if (hasNavigationGallery(figmaPage.id)) {
        variantsLayout->addWidget(createNavigationGallery(figmaPage.id, variants));
        sections->addTab(variants, QString::fromUtf8("分组与状态"));
        return page;
    }
    if (figmaPage.id == "234:2717") {
        variantsLayout->addWidget(createCheckboxGallery(factory, variants));
        sections->addTab(variants, QString::fromUtf8("分组与状态"));
        return page;
    }
    if(figmaPage.id=="172:2710") {
        variantsLayout->addWidget(createButtonGallery(factory,variants));
        sections->addTab(variants,QString::fromUtf8("分组与状态"));
        return page;
    }
    if(figmaPage.id=="341:2") {
        variantsLayout->addWidget(createRadioGallery(factory,variants));
        sections->addTab(variants,QString::fromUtf8("分组与状态"));
        return page;
    }
    if(figmaPage.id=="500:2") {
        variantsLayout->addWidget(createTabsGallery(factory,variants));
        sections->addTab(variants,QString::fromUtf8("分组与状态"));return page;
    }
    if(figmaPage.id=="505:2") {
        variantsLayout->addWidget(createToolBoxGallery(factory,variants));
        sections->addTab(variants,QString::fromUtf8("分组与状态"));return page;
    }
    if(figmaPage.id=="507:2") {
        variantsLayout->addWidget(createDockWidgetGallery(factory,variants));
        sections->addTab(variants,QString::fromUtf8("分组与状态"));return page;
    }
    if(figmaPage.id=="511:11") {
        variantsLayout->addWidget(createMdiGallery(factory,variants));
        sections->addTab(variants,QString::fromUtf8("分组与状态"));return page;
    }
    if(figmaPage.id=="516:11") {
        variantsLayout->addWidget(createPageFlowGallery(factory,variants));
        sections->addTab(variants,QString::fromUtf8("分组与状态"));return page;
    }
    if(hasCompositeGallery(figmaPage.id)) {
        variantsLayout->addWidget(createCompositeGallery(figmaPage.id,*factory,variants));
        sections->addTab(variants,QString::fromUtf8("分组与状态"));return page;
    }
    if(hasDataViewsGallery(figmaPage.id)) {
        variantsLayout->addWidget(createDataViewsGallery(figmaPage.id,variants));
        sections->addTab(variants,QString::fromUtf8("分组与状态"));return page;
    }
    if(hasWindowsGallery(figmaPage.id)) {
        variantsLayout->addWidget(createWindowsGallery(figmaPage.id,factory,variants));
        sections->addTab(variants,QString::fromUtf8("分组与状态"));return page;
    }
    if(hasContainersGallery(figmaPage.id)) {
        variantsLayout->addWidget(createContainersGallery(figmaPage.id,factory,variants));
        sections->addTab(variants,QString::fromUtf8("分组与状态"));return page;
    }
    if(hasNativeDisplayGallery(figmaPage.id)) {
        variantsLayout->addWidget(createNativeDisplayGallery(figmaPage.id,factory,variants));
        sections->addTab(variants,QString::fromUtf8("分组与状态"));return page;
    }
    if(hasEvidenceGallery(figmaPage.id)) {
        variantsLayout->addWidget(createEvidenceGallery(figmaPage,factory,variants));
        sections->addTab(variants,QString::fromUtf8("分组与状态"));return page;
    }
    if(figmaPage.id=="450:2" || figmaPage.id=="443:2" || figmaPage.id=="447:2") {
        variantsLayout->addWidget(createActionGallery(figmaPage.id,factory,variants));
        sections->addTab(variants,QString::fromUtf8("分组与状态"));
        return page;
    }
    if(figmaPage.id=="339:2") {
        variantsLayout->addWidget(createInputGallery(factory,variants));
        sections->addTab(variants,QString::fromUtf8("分组与状态"));
        return page;
    }
    if(figmaPage.id=="414:7" || figmaPage.id=="410:13" || figmaPage.id=="458:2" || figmaPage.id=="460:2" ||
       figmaPage.id=="463:2" || figmaPage.id=="464:2" || figmaPage.id=="465:2" || figmaPage.id=="467:2" || figmaPage.id=="471:334" || figmaPage.id=="475:334" || figmaPage.id=="1116:2" || figmaPage.id=="478:337" || figmaPage.id=="1120:2" || figmaPage.id=="1121:2") {
        variantsLayout->addWidget(createDisplayGallery(figmaPage.id,factory,variants));
        sections->addTab(variants,QString::fromUtf8("分组与状态"));
        return page;
    }
    auto* setTabs = new QTabWidget(variants);
    setTabs->setObjectName("componentSetTabs");
    variantsLayout->addWidget(setTabs);
    for (const auto& componentSet : figmaPage.componentSets) {
        auto* setPage = new QWidget(setTabs);
        auto* setLayout = new QVBoxLayout(setPage);
        QStringList axisDescriptions;
        for (const auto& axis : componentSet.variantAxes)
            axisDescriptions.append(axis.name + "=" + axis.values.join("/"));
        auto* meta = new QLabel(
            QString::fromUtf8("组件集 %1 · %2 个 Variant · %3")
                .arg(componentSet.id).arg(componentSet.variants.size())
                .arg(axisDescriptions.isEmpty()
                    ? QString::fromUtf8("无 Variant 轴")
                    : axisDescriptions.join(QString::fromUtf8(" · "))),
            setPage);
        meta->setObjectName("componentSetSummary");
        meta->setWordWrap(true);
        setLayout->addWidget(meta);
        setLayout->addWidget(createVariantMatrix(componentSet, descriptors, factory, setPage), 1);
        setTabs->addTab(setPage, componentSet.name);
    }
    sections->addTab(variants, QString::fromUtf8("分组与状态"));
    return page;
}

} // namespace

QWidget* createGallery(const QString& profile, const QMap<QString, QWidget*>& detailedPages,
                       QWidget* parent) {
    Q_UNUSED(detailedPages);
    auto* root = new QWidget(parent);
    root->setObjectName("nativeGalleryPage");
    const auto descriptors = commonWidget::designComponentDescriptors();
    const auto libraryEntries = commonWidget::designLibraryEntries();
    const auto groups = commonWidget::figmaComponentCategories();
    const auto figmaPages = commonWidget::figmaComponentPages();
    const int instantiableCount = std::count_if(
        descriptors.begin(), descriptors.end(),
        [](const auto& descriptor) { return descriptor.instantiable; });
    int componentSetCount = 0;
    int variantCount = 0;
    for (const auto& figmaPage : figmaPages) {
        componentSetCount += figmaPage.componentSets.size();
        for (const auto& componentSet : figmaPage.componentSets)
            variantCount += componentSet.variants.size();
    }
    QMap<QString, QString> pageSections;
    QMap<QString, int> sectionEntryCounts;
    for (const auto& entry : libraryEntries) {
        const QString section = entry.kind == QStringLiteral("LayoutHelper")
            ? QStringLiteral("layout")
            : entry.kind == QStringLiteral("Asset")
            ? QStringLiteral("assets")
            : QStringLiteral("components");
        pageSections.insert(entry.pageId, section);
        ++sectionEntryCounts[section];
    }
    QMap<QString, int> sectionPageCounts;
    for (const auto& figmaPage : figmaPages)
        ++sectionPageCounts[pageSections.value(figmaPage.id, QStringLiteral("components"))];
    root->setProperty("descriptorCount", descriptors.size());
    root->setProperty("libraryEntryCount", libraryEntries.size());
    root->setProperty("componentCount", sectionEntryCounts.value("components"));
    root->setProperty("pageCount", figmaPages.size());
    root->setProperty("componentPageCount", sectionPageCounts.value("components"));
    root->setProperty("layoutHelperCount", sectionEntryCounts.value("layout"));
    root->setProperty("layoutPageCount", sectionPageCounts.value("layout"));
    root->setProperty("assetCount", sectionEntryCounts.value("assets"));
    root->setProperty("assetPageCount", sectionPageCounts.value("assets"));
    root->setProperty("sectionCount", 3);
    root->setProperty("componentSetCount", componentSetCount);
    root->setProperty("variantCount", variantCount);
    root->setProperty("instantiableCount", instantiableCount);
    root->setProperty("contractOnlyCount", descriptors.size() - instantiableCount);
    root->setProperty("figmaFileKey", "w92BKW1EcutKmTXkwzNHbf");
    root->setProperty("categoryCount", groups.size());

    auto* layout = new QHBoxLayout(root);
    auto* componentTree = new QTreeWidget(root);
    componentTree->setObjectName("componentTree");
    componentTree->setHeaderHidden(true);
    componentTree->setMinimumWidth(320);
    componentTree->setMaximumWidth(440);
    layout->addWidget(componentTree);

    auto* pages = new QStackedWidget(root);
    pages->setObjectName("componentPages");
    layout->addWidget(pages, 1);

    QMap<QString, DesignComponentDescriptor> byClass;
    for (const auto& descriptor : descriptors)
        byClass.insert(descriptor.sdkClass, descriptor);
    struct GallerySection { QString id; QString label; };
    const QList<GallerySection> sections{
        {QStringLiteral("components"), QString::fromUtf8("组件")},
        {QStringLiteral("layout"), QString::fromUtf8("布局辅助")},
        {QStringLiteral("assets"), QString::fromUtf8("资产")},
    };
    for (const auto& section : sections) {
        auto* sectionItem = new QTreeWidgetItem(componentTree);
        sectionItem->setData(0, Qt::UserRole, QStringLiteral("section:") + section.id);
        sectionItem->setData(0, Qt::UserRole + 3, section.id);
        for (const auto& group : groups) {
            QTreeWidgetItem* branch = nullptr;
            for (const auto& figmaPage : figmaPages) {
                if (figmaPage.categoryId != group.id ||
                    pageSections.value(figmaPage.id, QStringLiteral("components")) != section.id)
                    continue;
                if (!branch) {
                    branch = new QTreeWidgetItem(sectionItem, {group.label});
                    branch->setData(0, Qt::UserRole, group.id);
                    branch->setData(0, Qt::UserRole + 3, section.id);
                }
                auto* leaf = new QTreeWidgetItem(branch, {figmaPage.name});
                leaf->setData(0, Qt::UserRole, figmaPage.id);
                leaf->setData(0, Qt::UserRole + 1, figmaPage.categoryId);
                leaf->setData(0, Qt::UserRole + 2, figmaPage.componentSets.size());
                leaf->setData(0, Qt::UserRole + 3, section.id);
                leaf->setToolTip(0, leaf->text(0));
            }
            if (!branch)
                continue;
            branch->setText(0, QString("%1 (%2)").arg(group.label).arg(branch->childCount()));
            branch->setToolTip(0, branch->text(0));
            branch->setExpanded(section.id == QStringLiteral("components") &&
                                group.id == QStringLiteral("20.1"));
        }
        sectionItem->setText(0, QString::fromUtf8("%1 (%2 项 / %3 页)")
            .arg(section.label)
            .arg(sectionEntryCounts.value(section.id))
            .arg(sectionPageCounts.value(section.id)));
        sectionItem->setToolTip(0, sectionItem->text(0));
        sectionItem->setExpanded(true);
    }

    auto factory = std::make_shared<commonWidget::ComponentFactory>(profile);
    QMap<QString, DesignComponentPage> byId;
    for (const auto& figmaPage : figmaPages)
        byId.insert(figmaPage.id, figmaPage);
    auto pageById = std::make_shared<QMap<QString, QWidget*>>();
    QObject::connect(componentTree, &QTreeWidget::currentItemChanged, root,
                     [root, pages, byId, descriptors, factory, detailedPages, pageById,
                      profile](QTreeWidgetItem* current) {
        if (!current)
            return;
        const QString pageId = current->data(0, Qt::UserRole).toString();
        if (!byId.contains(pageId))
            return;
        QWidget* page = pageById->value(pageId, nullptr);
        if (!page) {
            page = createFigmaPage(byId.value(pageId), descriptors, factory, detailedPages,
                                   profile, pages);
            pages->addWidget(page);
            pageById->insert(pageId, page);
        }
        pages->setCurrentWidget(page);
        root->setProperty("activePageId", pageId);
        root->setProperty("visibleDescriptorCount", 1);
    });
    QTreeWidgetItem* firstLeaf = nullptr;
    for (int sectionIndex = 0; sectionIndex < componentTree->topLevelItemCount() && !firstLeaf;
         ++sectionIndex) {
        auto* sectionItem = componentTree->topLevelItem(sectionIndex);
        for (int categoryIndex = 0; categoryIndex < sectionItem->childCount() && !firstLeaf;
             ++categoryIndex) {
            auto* categoryItem = sectionItem->child(categoryIndex);
            if (categoryItem->childCount())
                firstLeaf = categoryItem->child(0);
        }
    }
    if (firstLeaf)
        componentTree->setCurrentItem(firstLeaf);
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

    const auto descriptors = commonWidget::designComponentDescriptors();
    const auto libraryEntries = commonWidget::designLibraryEntries();
    const auto categories = commonWidget::designComponentCategories();
    const int instantiableCount = std::count_if(
        descriptors.begin(), descriptors.end(),
        [](const auto& descriptor) { return descriptor.instantiable; });
    root->setProperty("componentCount", descriptors.size());
    root->setProperty("libraryEntryCount", libraryEntries.size());
    root->setProperty("instantiableCount", instantiableCount);
    root->setProperty("contractOnlyCount", descriptors.size() - instantiableCount);
    summary->setText(QString::fromUtf8(
        "当前规范库：%1 项，其中组件身份 %2 项；%3 项可实例化，%4 项仅有设计合同。布局辅助和资产不进入组件 Factory。未获审批的设计身份不会被标记为发布完成。")
        .arg(libraryEntries.size()).arg(descriptors.size()).arg(instantiableCount)
        .arg(descriptors.size() - instantiableCount));
    auto* table = new QTableWidget(descriptors.size(), 6, root);
    table->setObjectName("nativeRegistryTable");
    table->setHorizontalHeaderLabels({QString::fromUtf8("Z-DesignSystem 组件"),
                                      QString::fromUtf8("设计组件"), QString::fromUtf8("Qt 基类 / 结构"),
                                      QString::fromUtf8("类型"), QString::fromUtf8("分类"),
                                      QString::fromUtf8("运行状态")});
    for (int row = 0; row < descriptors.size(); ++row) {
        const auto& descriptor = descriptors[row];
        QString categoryLabel;
        for (const auto& category : categories) {
            if (category.id == descriptor.category) {
                categoryLabel = category.label;
                break;
            }
        }
        table->setItem(row, 0, new QTableWidgetItem(descriptor.sdkClass));
        table->setItem(row, 1, new QTableWidgetItem(descriptor.designComponent));
        table->setItem(row, 2, new QTableWidgetItem(descriptor.qtClass));
        table->setItem(row, 3, new QTableWidgetItem(descriptor.mappingKind));
        table->setItem(row, 4, new QTableWidgetItem(categoryLabel));
        table->setItem(row, 5, new QTableWidgetItem(descriptor.runtimeStatus));
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
