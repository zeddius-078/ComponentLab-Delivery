#include "BaselineGallery.h"

#include <MyDesignSystem/MyDesignSystem.h>

#include <QApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QHeaderView>
#include <QGraphicsScene>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QSaveFile>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QStackedWidget>
#include <QStyle>
#include <QStyleOptionButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

struct ScopeEntry {
    const char* qtClass;
    const char* sdkClass;
};

const QVector<ScopeEntry>& publishedScope() {
    static const QVector<ScopeEntry> entries{
#define ADD_SCOPE_ENTRY(QT_CLASS, SDK_CLASS) {#QT_CLASS, "commonWidget::" #SDK_CLASS},
        Z_NATIVE_COMPONENT_TYPES(ADD_SCOPE_ENTRY)
#undef ADD_SCOPE_ENTRY
    };
    return entries;
}

struct SectionPage {
    QWidget* appearance;
    QWidget* state;
};

QLabel* label(const QString& text, const QString& name, QWidget* parent) {
    auto* result = new QLabel(text, parent);
    result->setObjectName(name);
    result->setWordWrap(true);
    return result;
}

commonWidget::Z_GroupBox* card(const QString& title, const QString& description, QWidget* parent) {
    auto* result = new commonWidget::Z_GroupBox(title, parent);
    result->setObjectName(QStringLiteral("baselineComponentCard"));
    auto* layout = new QVBoxLayout(result);
    layout->setContentsMargins(16, 20, 16, 16);
    layout->setSpacing(12);
    layout->addWidget(label(description, QStringLiteral("baselinePageDescription"), result));
    return result;
}

QWidget* scroll(QWidget* content, QWidget* parent) {
    auto* result = new commonWidget::Z_ScrollArea(parent);
    result->setWidgetResizable(true);
    result->setFrameShape(QFrame::NoFrame);
    result->setWidget(content);
    return result;
}

void addProperty(QTreeWidgetItem* parent, const QString& name, const QVariant& value) {
    new QTreeWidgetItem(parent, {name, value.toString()});
}

void appendObjectTree(QTreeWidgetItem* parent, QWidget* widget, int depth = 0) {
    if (!widget)
        return;
    const QString name = widget->objectName().isEmpty()
                             ? QString::fromLatin1(widget->metaObject()->className())
                             : widget->objectName();
    auto* item = new QTreeWidgetItem(parent,
                                     {name, QString::fromLatin1(widget->metaObject()->className())});
    item->setExpanded(depth == 0);
    if (depth == 0) {
        addProperty(item, QStringLiteral("geometry"),
                    QStringLiteral("%1 × %2").arg(widget->width()).arg(widget->height()));
        addProperty(item, QStringLiteral("enabled"), widget->isEnabled());
        addProperty(item, QStringLiteral("visible"), widget->isVisible());
        for (const auto& property : {"nativeQtClass", "designSystemClass"}) {
            const QVariant value = widget->property(property);
            if (value.isValid())
                addProperty(item, QString::fromLatin1(property), value);
        }
        if (auto* button = qobject_cast<QAbstractButton*>(widget))
            addProperty(item, QStringLiteral("text"), button->text());
        if (auto* checkbox = qobject_cast<QCheckBox*>(widget))
            addProperty(item, QStringLiteral("checkState"), checkbox->checkState());
    }
    if (depth >= 2)
        return;
    for (auto* child : widget->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
        appendObjectTree(item, child, depth + 1);
}

QString categoryFor(const QString& qtClass) {
    static const QHash<QString, QString> categories{
        {QStringLiteral("QMenu"), QStringLiteral("Navigation / 导航")},
        {QStringLiteral("QMenuBar"), QStringLiteral("Navigation / 导航")},
        {QStringLiteral("QToolBar"), QStringLiteral("Navigation / 导航")},
        {QStringLiteral("QToolBox"), QStringLiteral("Navigation / 导航")},
        {QStringLiteral("QTabWidget"), QStringLiteral("Navigation / 导航")},
        {QStringLiteral("QDockWidget"), QStringLiteral("Navigation / 导航")},
        {QStringLiteral("QMdiArea"), QStringLiteral("Navigation / 导航")},
        {QStringLiteral("QMdiSubWindow"), QStringLiteral("Navigation / 导航")},
        {QStringLiteral("QWizard"), QStringLiteral("Navigation / 导航")},
        {QStringLiteral("QWizardPage"), QStringLiteral("Navigation / 导航")},
        {QStringLiteral("QLabel"), QStringLiteral("Display / 展示")},
        {QStringLiteral("QLCDNumber"), QStringLiteral("Display / 展示")},
        {QStringLiteral("QProgressBar"), QStringLiteral("Display / 展示")},
        {QStringLiteral("QTextBrowser"), QStringLiteral("Display / 展示")},
        {QStringLiteral("QGraphicsView"), QStringLiteral("Display / 展示")},
        {QStringLiteral("QCalendarWidget"), QStringLiteral("Display / 展示")},
        {QStringLiteral("QOpenGLWidget"), QStringLiteral("Display / 展示")},
        {QStringLiteral("QPushButton"), QStringLiteral("Actions / 操作")},
        {QStringLiteral("QToolButton"), QStringLiteral("Actions / 操作")},
        {QStringLiteral("QCommandLinkButton"), QStringLiteral("Actions / 操作")},
        {QStringLiteral("QDialogButtonBox"), QStringLiteral("Actions / 操作")},
        {QStringLiteral("QLineEdit"), QStringLiteral("Input / 输入")},
        {QStringLiteral("QSpinBox"), QStringLiteral("Input / 输入")},
        {QStringLiteral("QDoubleSpinBox"), QStringLiteral("Input / 输入")},
        {QStringLiteral("QSlider"), QStringLiteral("Input / 输入")},
        {QStringLiteral("QScrollBar"), QStringLiteral("Input / 输入")},
        {QStringLiteral("QDial"), QStringLiteral("Input / 输入")},
        {QStringLiteral("QDateTimeEdit"), QStringLiteral("Input / 输入")},
        {QStringLiteral("QDateEdit"), QStringLiteral("Input / 输入")},
        {QStringLiteral("QTimeEdit"), QStringLiteral("Input / 输入")},
        {QStringLiteral("QKeySequenceEdit"), QStringLiteral("Input / 输入")},
        {QStringLiteral("QTextEdit"), QStringLiteral("Input / 输入")},
        {QStringLiteral("QPlainTextEdit"), QStringLiteral("Input / 输入")},
        {QStringLiteral("QComboBox"), QStringLiteral("Selection / 选择")},
        {QStringLiteral("QFontComboBox"), QStringLiteral("Selection / 选择")},
        {QStringLiteral("QRadioButton"), QStringLiteral("Selection / 选择")},
        {QStringLiteral("QCheckBox"), QStringLiteral("Selection / 选择")},
        {QStringLiteral("QWidget"), QStringLiteral("Container / 容器")},
        {QStringLiteral("QFrame"), QStringLiteral("Container / 容器")},
        {QStringLiteral("QGroupBox"), QStringLiteral("Container / 容器")},
        {QStringLiteral("QScrollArea"), QStringLiteral("Container / 容器")},
        {QStringLiteral("QStackedWidget"), QStringLiteral("Container / 容器")},
        {QStringLiteral("QSplitter"), QStringLiteral("Container / 容器")},
        {QStringLiteral("QListView"), QStringLiteral("Data Views / 数据视图")},
        {QStringLiteral("QTreeView"), QStringLiteral("Data Views / 数据视图")},
        {QStringLiteral("QTableView"), QStringLiteral("Data Views / 数据视图")},
        {QStringLiteral("QColumnView"), QStringLiteral("Data Views / 数据视图")},
        {QStringLiteral("QUndoView"), QStringLiteral("Data Views / 数据视图")},
        {QStringLiteral("QListWidget"), QStringLiteral("Data Views / 数据视图")},
        {QStringLiteral("QTreeWidget"), QStringLiteral("Data Views / 数据视图")},
        {QStringLiteral("QTableWidget"), QStringLiteral("Data Views / 数据视图")},
        {QStringLiteral("QStatusBar"), QStringLiteral("Feedback / 反馈")},
    };
    if (categories.contains(qtClass))
        return categories.value(qtClass);
    return QStringLiteral("Window & System / 窗口与系统");
}

bool isFloatingType(const QString& qtClass) {
    return qtClass == QStringLiteral("QMenu") || qtClass == QStringLiteral("QWizard") ||
           qtClass.endsWith(QStringLiteral("Dialog")) ||
           qtClass == QStringLiteral("QMessageBox") ||
           qtClass == QStringLiteral("QProgressDialog");
}

QWidget* instantiate(const ScopeEntry& entry, QWidget* parent) {
    const QString sdkClass = QString::fromLatin1(entry.sdkClass);
#define INSTANTIATE(QT_CLASS, SDK_CLASS) \
    if (sdkClass == QStringLiteral("commonWidget::" #SDK_CLASS)) \
        return new commonWidget::SDK_CLASS(parent);
    Z_NATIVE_COMPONENT_TYPES(INSTANTIATE)
#undef INSTANTIATE
    return nullptr;
}

void configureSpecimen(QWidget* specimen, const QString& qtClass) {
    specimen->setObjectName(QStringLiteral("componentRoot_%1").arg(qtClass));
    if (auto* button = qobject_cast<QAbstractButton*>(specimen)) {
        button->setText(qtClass);
        if (auto* checkbox = qobject_cast<QCheckBox*>(button)) {
            checkbox->setText(QObject::tr("检查项目"));
            checkbox->setTristate(true);
        }
    }
    if (auto* edit = qobject_cast<QLineEdit*>(specimen))
        edit->setPlaceholderText(QObject::tr("输入批次或工单编号"));
    if (auto* combo = qobject_cast<QComboBox*>(specimen))
        combo->addItems({QObject::tr("自动"), QObject::tr("手动"), QObject::tr("维护")});
    if (auto* spin = qobject_cast<QSpinBox*>(specimen)) {
        spin->setRange(0, 100);
        spin->setValue(42);
    }
    if (auto* slider = qobject_cast<QAbstractSlider*>(specimen)) {
        slider->setOrientation(Qt::Horizontal);
        slider->setRange(0, 100);
        slider->setValue(64);
    }
    if (auto* progress = qobject_cast<QProgressBar*>(specimen)) {
        progress->setValue(68);
        progress->setFormat(QStringLiteral("%p%"));
    }
    if (auto* text = qobject_cast<QLabel*>(specimen))
        text->setText(QObject::tr("设备状态：运行中"));
    if (auto* table = qobject_cast<QTableWidget*>(specimen)) {
        table->setColumnCount(2);
        table->setRowCount(2);
        table->setHorizontalHeaderLabels({QObject::tr("参数"), QObject::tr("值")});
        table->setItem(0, 0, new QTableWidgetItem(QObject::tr("状态")));
        table->setItem(0, 1, new QTableWidgetItem(QObject::tr("运行")));
    }
    if (auto* list = qobject_cast<QListWidget*>(specimen))
        list->addItems({QObject::tr("设备 01"), QObject::tr("设备 02"), QObject::tr("设备 03")});
    if (auto* tree = qobject_cast<QTreeWidget*>(specimen)) {
        tree->setHeaderLabel(QObject::tr("设备组"));
        new QTreeWidgetItem(tree, {QStringLiteral("ETCH-01")});
    }
    if (auto* view = qobject_cast<QAbstractItemView*>(specimen)) {
        if (!qobject_cast<QListWidget*>(view) && !qobject_cast<QTreeWidget*>(view) &&
            !qobject_cast<QTableWidget*>(view)) {
            auto* model = new QStandardItemModel(3, 2, view);
            model->setHorizontalHeaderLabels({QObject::tr("设备"), QObject::tr("状态")});
            for (int row = 0; row < 3; ++row) {
                model->setItem(row, 0, new QStandardItem(QObject::tr("设备 %1").arg(row + 1)));
                model->setItem(row, 1, new QStandardItem(QObject::tr("运行")));
            }
            view->setModel(model);
        }
    }
    if (auto* tabs = qobject_cast<QTabWidget*>(specimen)) {
        tabs->addTab(new QLabel(QObject::tr("页签内容"), tabs), QObject::tr("概览"));
        tabs->addTab(new QLabel(QObject::tr("第二页"), tabs), QObject::tr("详情"));
    }
    if (auto* group = qobject_cast<QGroupBox*>(specimen))
        group->setTitle(QObject::tr("组件分组"));
    if (auto* area = qobject_cast<QScrollArea*>(specimen)) {
        auto* content = new QLabel(QObject::tr("滚动内容\n项目 1\n项目 2\n项目 3\n项目 4\n项目 5"), area);
        area->setWidget(content);
    }
    if (auto* toolbox = qobject_cast<QToolBox*>(specimen)) {
        toolbox->addItem(new QLabel(QObject::tr("概览内容"), toolbox), QObject::tr("概览"));
        toolbox->addItem(new QLabel(QObject::tr("参数内容"), toolbox), QObject::tr("参数"));
    }
    if (auto* stack = qobject_cast<QStackedWidget*>(specimen))
        stack->addWidget(new QLabel(QObject::tr("内容区域"), stack));
    if (auto* splitter = qobject_cast<QSplitter*>(specimen)) {
        splitter->addWidget(new QLabel(QObject::tr("导航"), splitter));
        splitter->addWidget(new QLabel(QObject::tr("内容"), splitter));
    }
    if (auto* bar = qobject_cast<QMenuBar*>(specimen))
        bar->addMenu(QObject::tr("文件"))->addAction(QObject::tr("新建"));
    if (auto* bar = qobject_cast<QToolBar*>(specimen))
        bar->addAction(QObject::tr("刷新"));
    if (auto* bar = qobject_cast<QStatusBar*>(specimen))
        bar->showMessage(QObject::tr("运行中"));
    if (auto* view = qobject_cast<QGraphicsView*>(specimen)) {
        auto* scene = new QGraphicsScene(view);
        scene->addText(QObject::tr("图形视图"));
        view->setScene(scene);
    }
    if (auto* calendar = qobject_cast<QCalendarWidget*>(specimen))
        calendar->setSelectedDate(QDate(2026, 9, 23));
    if (auto* box = qobject_cast<QDialogButtonBox*>(specimen))
        box->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    if (auto* date = qobject_cast<QDateTimeEdit*>(specimen))
        date->setDateTime(QDateTime(QDate(2026, 9, 23), QTime(9, 30)));
    if (auto* text = qobject_cast<QTextEdit*>(specimen))
        text->setPlainText(QObject::tr("工艺记录：等待设备确认。"));
    if (auto* mdi = qobject_cast<QMdiArea*>(specimen)) {
        auto* child = new QLabel(QObject::tr("文档内容"), mdi);
        mdi->addSubWindow(child)->show();
    }
}

SectionPage createComponentSection(const ScopeEntry& entry, QWidget* parent) {
    const QString qtClass = QString::fromLatin1(entry.qtClass);
    const QString sdkClass = QString::fromLatin1(entry.sdkClass);
    const bool floating = isFloatingType(qtClass);
    const bool requiresGraphics = qtClass == QStringLiteral("QOpenGLWidget");
    auto* appearanceContent = new QWidget;
    auto* appearanceLayout = new QVBoxLayout(appearanceContent);
    appearanceLayout->setContentsMargins(20, 20, 20, 28);
    appearanceLayout->setSpacing(16);
    auto* appearanceCard = card(qtClass + QObject::tr(" / 外观"),
                                sdkClass + QObject::tr(" · SDK 真实实例"), appearanceContent);
    if (floating) {
        auto* openButton = new commonWidget::Z_PushButton(QObject::tr("打开 %1").arg(qtClass), appearanceCard);
        auto* popup = instantiate(entry, appearanceCard);
        configureSpecimen(popup, qtClass);
        popup->setWindowFlags(Qt::Dialog);
        QObject::connect(openButton, &QPushButton::clicked, popup, [popup] { popup->show(); });
        static_cast<QVBoxLayout*>(appearanceCard->layout())->addWidget(openButton, 0, Qt::AlignLeft);
    } else if (requiresGraphics) {
        static_cast<QVBoxLayout*>(appearanceCard->layout())->addWidget(
            label(QObject::tr("OpenGL 视口需要图形上下文；离线截图环境不创建上下文。"),
                  QStringLiteral("baselinePageDescription"), appearanceCard));
    } else {
        auto* specimen = instantiate(entry, appearanceCard);
        configureSpecimen(specimen, qtClass);
        specimen->setWindowFlags(Qt::Widget);
        const bool spacious = qobject_cast<QAbstractItemView*>(specimen) ||
                              qobject_cast<QCalendarWidget*>(specimen) ||
            qobject_cast<QTextEdit*>(specimen) || qobject_cast<QMdiArea*>(specimen) ||
            qobject_cast<QTabWidget*>(specimen) || qobject_cast<QGraphicsView*>(specimen) ||
            qobject_cast<QSplitter*>(specimen) || qobject_cast<QScrollArea*>(specimen);
        if (spacious)
            specimen->setMinimumHeight(190);
        else
            specimen->setMaximumWidth(qtClass == QStringLiteral("QSlider") ? 280 : 260);
        static_cast<QVBoxLayout*>(appearanceCard->layout())->addWidget(
            specimen, 0, spacious ? Qt::Alignment{} : Qt::AlignLeft);
    }
    appearanceLayout->addWidget(appearanceCard);
    appearanceLayout->addStretch();

    auto* stateContent = new QWidget;
    auto* stateLayout = new QVBoxLayout(stateContent);
    stateLayout->setContentsMargins(20, 20, 20, 28);
    auto* stateCard = card(qtClass + QObject::tr(" / 分组与状态"),
                           QObject::tr("此页只展示当前包装类的 Qt 原生状态。"), stateContent);
    if (floating || requiresGraphics) {
        static_cast<QVBoxLayout*>(stateCard->layout())->addWidget(
            label(floating ? QObject::tr("窗口状态请使用外观页的打开操作查看。")
                           : QObject::tr("此状态需要可用的 OpenGL 图形上下文。"),
                  QStringLiteral("baselinePageDescription"), stateCard));
    } else {
        auto* row = new QHBoxLayout;
        auto* normal = instantiate(entry, stateCard);
        auto* disabled = instantiate(entry, stateCard);
        configureSpecimen(normal, qtClass);
        configureSpecimen(disabled, qtClass);
        disabled->setEnabled(false);
        normal->setMaximumWidth(260);
        disabled->setMaximumWidth(260);
        row->addWidget(normal, 0, Qt::AlignLeft);
        row->addSpacing(32);
        row->addWidget(disabled, 0, Qt::AlignLeft);
        row->addStretch();
        static_cast<QVBoxLayout*>(stateCard->layout())->addWidget(
            label(QObject::tr("默认 / 可交互"), QStringLiteral("baselinePageDescription"), stateCard));
        static_cast<QVBoxLayout*>(stateCard->layout())->addLayout(row);
        if (auto* checkbox = qobject_cast<QCheckBox*>(normal)) {
            auto* mixed = instantiate(entry, stateCard);
            auto* mixedCheckbox = qobject_cast<QCheckBox*>(mixed);
            mixedCheckbox->setTristate(true);
            mixedCheckbox->setCheckState(Qt::PartiallyChecked);
            mixedCheckbox->setText(QObject::tr("半选"));
            mixedCheckbox->setObjectName(QStringLiteral("baselineIndeterminateCheckbox"));
            static_cast<QVBoxLayout*>(stateCard->layout())->addWidget(mixed);
            checkbox->setChecked(true);
        }
    }
    stateLayout->addWidget(stateCard);
    stateLayout->addStretch();
    return {scroll(appearanceContent, parent), scroll(stateContent, parent)};
}

QWidget* wrapPage(const QString& kicker, const QString& title, const QString& description,
                  const SectionPage& section, QWidget* parent) {
    auto* host = new QWidget(parent);
    host->setObjectName(QStringLiteral("baselineContentHost"));
    auto* layout = new QVBoxLayout(host);
    layout->setContentsMargins(20, 18, 20, 20);
    layout->setSpacing(8);
    layout->addWidget(label(kicker, QStringLiteral("baselinePageKicker"), host));
    layout->addWidget(label(title, QStringLiteral("baselinePageTitle"), host));
    layout->addWidget(label(description, QStringLiteral("baselinePageDescription"), host));
    layout->addSpacing(6);
    auto* tabs = new commonWidget::Z_TabWidget(host);
    tabs->setObjectName(QStringLiteral("baselineComponentTabs"));
    tabs->addTab(section.appearance, QObject::tr("外观"));
    tabs->addTab(section.state, QObject::tr("分组与状态"));
    layout->addWidget(tabs, 1);
    return host;
}

QWidget* createScopePage(QWidget* parent) {
    auto* host = new QWidget(parent);
    host->setObjectName(QStringLiteral("baselineMappingHost"));
    auto* layout = new QVBoxLayout(host);
    layout->setContentsMargins(24, 20, 24, 24);
    layout->setSpacing(10);
    layout->addWidget(label(QObject::tr("基础 SDK 公开映射"), QStringLiteral("baselinePageTitle"), host));
    layout->addWidget(label(QObject::tr("仅列出可迁移的 Qt 原生类 → commonWidget::Z_* 包装；非基础组件在后续范围中处理。"),
                            QStringLiteral("baselineMappingDescription"), host));
    auto* table = new commonWidget::Z_TableWidget(host);
    table->setObjectName(QStringLiteral("publishedNativeScope"));
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({QObject::tr("Qt 原生类"), QObject::tr("公开包装"),
                                      QObject::tr("发布状态")});
    table->setRowCount(publishedScope().size());
    for (int row = 0; row < publishedScope().size(); ++row) {
        const auto& entry = publishedScope().at(row);
        table->setItem(row, 0, new QTableWidgetItem(QString::fromLatin1(entry.qtClass)));
        table->setItem(row, 1, new QTableWidgetItem(QString::fromLatin1(entry.sdkClass)));
        table->setItem(row, 2, new QTableWidgetItem(QStringLiteral("baseline-native")));
    }
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    layout->addWidget(table, 1);
    return host;
}

}  // namespace

QString baselineGalleryCanvasStyleSheet() {
    static const QString styleSheet = [] {
        Q_INIT_RESOURCE(BaselineGallery);
        QFile file(QStringLiteral(":/componentlab/baseline/BaselineGallery.qss"));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return QString{};
        return QString::fromUtf8(file.readAll());
    }();
    return styleSheet;
}

BaselineWindow::BaselineWindow() {
    setWindowTitle(QStringLiteral("Component Lab | Qt Native Baseline"));
    resize(1360, 880);
    setMinimumSize(1040, 700);
    auto* root = new QWidget(this);
    root->setObjectName(QStringLiteral("baselineRoot"));
    auto* rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(24, 20, 24, 18);
    rootLayout->setSpacing(16);
    auto* header = new QWidget(root);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    auto* titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(3);
    titleLayout->addWidget(label(tr("Component Lab"), QStringLiteral("baselineGalleryTitle"), header));
    titleLayout->addWidget(label(tr("Published Qt-native SDK consumer"),
                                 QStringLiteral("baselineGallerySubtitle"), header));
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();
    headerLayout->addWidget(label(tr("59 个已发布包装"), QStringLiteral("baselineReleaseBadge"), header));
    rootLayout->addWidget(header);

    auto* sdkTabs = new commonWidget::Z_TabWidget(root);
    sdkTabs->setObjectName(QStringLiteral("baselineSdkTabs"));
    auto* canvas = new QWidget(sdkTabs);
    canvas->setObjectName(QStringLiteral("baselineCanvas"));
    auto* canvasLayout = new QVBoxLayout(canvas);
    canvasLayout->setContentsMargins(12, 12, 12, 12);
    auto* galleryTabs = new commonWidget::Z_TabWidget(canvas);
    galleryTabs->setObjectName(QStringLiteral("baselineGalleryTabs"));
    auto* browse = new QWidget(galleryTabs);
    auto* browseLayout = new QVBoxLayout(browse);
    browseLayout->setContentsMargins(0, 0, 0, 0);
    auto* splitter = new commonWidget::Z_Splitter(Qt::Horizontal, browse);
    splitter->setObjectName(QStringLiteral("baselineGallerySplitter"));
    navigation_ = new commonWidget::Z_TreeWidget(splitter);
    navigation_->setObjectName(QStringLiteral("baselineNavigation"));
    navigation_->setHeaderLabel(tr("基础组件"));
    navigation_->setMinimumWidth(254);
    navigation_->setMaximumWidth(330);
    navigation_->setAlternatingRowColors(true);
    auto* stack = new QStackedWidget(splitter);
    stack->setObjectName(QStringLiteral("baselineGalleryPages"));
    pages_ = stack;
    objectInspector_ = new commonWidget::Z_TreeWidget(splitter);
    objectInspector_->setObjectName(QStringLiteral("baselineObjectInspector"));
    objectInspector_->setHeaderLabels({tr("节点 / 属性"), tr("类型 / 值")});
    objectInspector_->setMinimumWidth(270);
    objectInspector_->setMaximumWidth(360);
    objectInspector_->setAlternatingRowColors(true);
    objectInspector_->setColumnWidth(0, 150);
    splitter->addWidget(navigation_);
    splitter->addWidget(stack);
    splitter->addWidget(objectInspector_);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({285, 770, 300});
    browseLayout->addWidget(splitter);
    auto* publishedNode = new QTreeWidgetItem(navigation_,
                                               {tr("组件（%1）").arg(publishedScope().size())});
    QHash<QString, QTreeWidgetItem*> categoryItems;
    QTreeWidgetItem* defaultLeaf = nullptr;
    const QStringList order{
        QStringLiteral("Navigation / 导航"), QStringLiteral("Display / 展示"),
        QStringLiteral("Actions / 操作"), QStringLiteral("Input / 输入"),
        QStringLiteral("Selection / 选择"), QStringLiteral("Container / 容器"),
        QStringLiteral("Data Views / 数据视图"), QStringLiteral("Feedback / 反馈"),
        QStringLiteral("Window & System / 窗口与系统")};
    for (const auto& category : order)
        categoryItems.insert(category, new QTreeWidgetItem(publishedNode, {category}));
    for (const auto& entry : publishedScope()) {
        const QString qtClass = QString::fromLatin1(entry.qtClass);
        const QString category = categoryFor(qtClass);
        auto* branch = categoryItems.value(category);
        auto* leaf = new QTreeWidgetItem(branch, {QString::fromLatin1(entry.sdkClass).section("::", -1)});
        leaf->setData(0, Qt::UserRole, stack->count());
        leaf->setData(0, Qt::UserRole + 1, qtClass);
        leaf->setToolTip(0, QString::fromLatin1(entry.sdkClass));
        const auto section = createComponentSection(entry, stack);
        auto* page = wrapPage(category, qtClass + QStringLiteral(" / ") +
                             QString::fromLatin1(entry.sdkClass).section("::", -1),
                             tr("单组件展示 · 外观与分组状态分别查看"), section, stack);
        connect(page->findChild<QTabWidget*>(QStringLiteral("baselineComponentTabs")),
                &QTabWidget::currentChanged, this,
                [this, page](int) { if (pages_->isVisible() &&
                                        qobject_cast<QStackedWidget*>(pages_)->currentWidget() == page)
                                    populateObjectInspector(page); });
        stack->addWidget(page);
        if (qtClass == QStringLiteral("QPushButton"))
            defaultLeaf = leaf;
    }
    for (const auto& category : order) {
        auto* branch = categoryItems.value(category);
        branch->setText(0, tr("%1 (%2)").arg(category).arg(branch->childCount()));
        branch->setExpanded(category == QStringLiteral("Actions / 操作"));
    }
    publishedNode->setExpanded(true);
    connect(navigation_, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item) {
        if (item && item->data(0, Qt::UserRole).isValid())
            selectPage(item->data(0, Qt::UserRole).toInt());
    });
    navigation_->setCurrentItem(defaultLeaf);
    galleryTabs->addTab(browse, tr("组件图库"));
    galleryTabs->addTab(createScopePage(galleryTabs), tr("组件映射"));
    canvasLayout->addWidget(galleryTabs);
    sdkTabs->addTab(canvas, tr("Z-Native SDK"));
    rootLayout->addWidget(sdkTabs, 1);
    setCentralWidget(root);
    setStatusBar(new commonWidget::Z_StatusBar(this));
    statusBar()->showMessage(tr("仅消费 GeneratedComponentSDK：SDK QSS/QRC 已加载；不读取 Token JSON。"));
    QTimer::singleShot(0, this, [this] {
        populateObjectInspector(qobject_cast<QStackedWidget*>(pages_)->currentWidget());
    });
}

bool BaselineWindow::capture(const QString& directory) {
    if (!QDir().mkpath(directory) || !grab().save(directory + QStringLiteral("/baseline-gallery.png")))
        return false;
    auto* checkboxPage = findChild<commonWidget::Z_CheckBox*>(
        QStringLiteral("baselineIndeterminateCheckbox"));
    commonWidget::Z_CheckBox checkboxProbe;
    checkboxProbe.setTristate(true);
    checkboxProbe.setCheckState(Qt::PartiallyChecked);
    checkboxProbe.resize(180, 32);
    checkboxProbe.show();
    QStyleOptionButton option;
    option.initFrom(&checkboxProbe);
    option.state |= QStyle::State_NoChange;
    const QRect indicator = checkboxProbe.style()->subElementRect(
        QStyle::SE_CheckBoxIndicator, &option, &checkboxProbe);
    const QColor dash = checkboxProbe.grab().toImage().pixelColor(indicator.center());
    QJsonObject result{{QStringLiteral("packageKind"), QStringLiteral("qt-native-baseline")},
                       {QStringLiteral("nativeComponentCount"), publishedScope().size()},
                       {QStringLiteral("runtimeTokenJson"), false},
                       {QStringLiteral("legacyRuntime"), false},
                       {QStringLiteral("qssApplied"), !qApp->styleSheet().isEmpty()},
                       {QStringLiteral("presentation"), QStringLiteral("component-per-page-gallery")},
                       {QStringLiteral("gallerySections"), componentSectionCount()},
                       {QStringLiteral("galleryPages"), componentPageCount()},
                       {QStringLiteral("objectInspector"), hasObjectInspector()},
                       {QStringLiteral("indeterminateClass"), checkboxPage ? checkboxPage->property("designSystemClass").toString() : QString{}},
                       {QStringLiteral("indeterminateDashPixel"), dash.name()},
                       {QStringLiteral("indeterminateDashVisible"), dash.red() > 220 && dash.green() > 220 && dash.blue() > 220}};
    QSaveFile file(directory + QStringLiteral("/runtime.json"));
    return file.open(QIODevice::WriteOnly) && file.write(QJsonDocument(result).toJson()) >= 0 && file.commit();
}

int BaselineWindow::componentSectionCount() const { return navigation_ ? navigation_->topLevelItem(0)->childCount() : 0; }

int BaselineWindow::componentPageCount() const {
    const auto* stack = qobject_cast<const QStackedWidget*>(pages_);
    return stack ? stack->count() : 0;
}

bool BaselineWindow::hasObjectInspector() const {
    return objectInspector_ && objectInspector_->columnCount() == 2;
}

void BaselineWindow::selectPage(int pageIndex) {
    auto* stack = qobject_cast<QStackedWidget*>(pages_);
    if (!stack || pageIndex < 0 || pageIndex >= stack->count())
        return;
    stack->setCurrentIndex(pageIndex);
    populateObjectInspector(stack->currentWidget());
}

void BaselineWindow::populateObjectInspector(QWidget* target) {
    objectInspector_->clear();
    if (!target)
        return;
    auto* tabs = target->findChild<QTabWidget*>(QStringLiteral("baselineComponentTabs"));
    QWidget* active = tabs ? tabs->currentWidget() : target;
    QWidget* specimen = nullptr;
    for (auto* child : active->findChildren<QWidget*>()) {
        if (child->objectName().startsWith(QStringLiteral("componentRoot_"))) {
            specimen = child;
            break;
        }
    }
    auto* root = new QTreeWidgetItem(objectInspector_,
                                     {tr("当前组件"), specimen ? specimen->property("designSystemClass").toString()
                                                              : tr("无可嵌入实例")});
    root->setExpanded(true);
    if (specimen)
        appendObjectTree(root, specimen);
}
