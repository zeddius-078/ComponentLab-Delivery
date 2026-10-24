#include "BaselineGallery.h"

#include <MyDesignSystem/MyDesignSystem.h>

#include <QApplication>
#include <QAbstractItemView>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QHeaderView>
#include <QGraphicsScene>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
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
    if (auto* combo = qobject_cast<QComboBox*>(specimen)) {
        if (!qobject_cast<QFontComboBox*>(combo))
            combo->addItems({QObject::tr("自动"), QObject::tr("手动"), QObject::tr("维护")});
    }
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
            !qobject_cast<QTableWidget*>(view) && !qobject_cast<QUndoView*>(view)) {
            auto* model = new QStandardItemModel(3, 2, view);
            model->setHorizontalHeaderLabels({QObject::tr("设备"), QObject::tr("状态")});
            for (int row = 0; row < 3; ++row) {
                model->setItem(row, 0, new QStandardItem(QObject::tr("设备 %1").arg(row + 1)));
                model->setItem(row, 1, new QStandardItem(QObject::tr("运行")));
            }
            view->setModel(model);
        }
    }
    if (auto* undo = qobject_cast<QUndoView*>(specimen)) {
        auto* stack = new QUndoStack(undo);
        stack->push(new QUndoCommand(QObject::tr("修改参数")));
        stack->push(new QUndoCommand(QObject::tr("保存配置")));
        undo->setStack(stack);
    }
    if (auto* tabs = qobject_cast<QTabWidget*>(specimen)) {
        tabs->addTab(new QLabel(QObject::tr("页签内容"), tabs), QObject::tr("概览"));
        tabs->addTab(new QLabel(QObject::tr("第二页"), tabs), QObject::tr("详情"));
    }
    if (auto* group = qobject_cast<QGroupBox*>(specimen))
        group->setTitle(QObject::tr("组件分组"));
    if (auto* area = qobject_cast<QScrollArea*>(specimen)) {
        auto* content = new QWidget(area);
        auto* contentLayout = new QVBoxLayout(content);
        for (int item = 1; item <= 12; ++item)
            contentLayout->addWidget(new QLabel(QObject::tr("项目 %1").arg(item), content));
        content->setMinimumSize(240, 360);
        area->setWidgetResizable(false);
        area->setWidget(content);
        area->setFixedHeight(200);
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
    if (auto* menu = qobject_cast<QMenu*>(specimen)) {
        menu->addAction(QObject::tr("新建"));
        menu->addAction(QObject::tr("打开"));
        menu->addSeparator();
        menu->addAction(QObject::tr("关闭"));
    }
    if (auto* dock = qobject_cast<QDockWidget*>(specimen)) {
        dock->setWindowTitle(QObject::tr("设备属性"));
        dock->setWidget(new QLabel(QObject::tr("属性内容"), dock));
    }
    if (auto* mdiChild = qobject_cast<QMdiSubWindow*>(specimen)) {
        mdiChild->setWindowTitle(QObject::tr("文档 1"));
        mdiChild->setWidget(new QLabel(QObject::tr("文档内容"), mdiChild));
    }
    if (auto* wizard = qobject_cast<QWizard*>(specimen)) {
        for (const auto& title : {QObject::tr("选择设备"), QObject::tr("确认配置")}) {
            auto* page = new commonWidget::Z_WizardPage(wizard);
            page->setTitle(title);
            auto* pageLayout = new QVBoxLayout(page);
            pageLayout->addWidget(new QLabel(title, page));
            wizard->addPage(page);
        }
    }
    if (auto* page = qobject_cast<QWizardPage*>(specimen)) {
        page->setTitle(QObject::tr("选择设备"));
        auto* pageLayout = new QVBoxLayout(page);
        pageLayout->addWidget(new QLabel(QObject::tr("页面内容"), page));
    }
    if (auto* lcd = qobject_cast<QLCDNumber*>(specimen))
        lcd->display(42);
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
    if (auto* text = qobject_cast<QPlainTextEdit*>(specimen))
        text->setPlainText(QObject::tr("工艺记录：等待设备确认。"));
    if (auto* mdi = qobject_cast<QMdiArea*>(specimen)) {
        auto* child = new QLabel(QObject::tr("文档内容"), mdi);
        mdi->addSubWindow(child)->show();
    }
}

commonWidget::Z_ComboBox* selectSpecimen(QWidget* parent,
                                         commonWidget::Z_ComboBox::SelectionMode mode,
                                         const QString& size = QStringLiteral("Middle"),
                                         const QString& appearance = QStringLiteral("Outlined"),
                                         const QString& status = QStringLiteral("Normal")) {
    auto* select = new commonWidget::Z_ComboBox(parent);
    select->addItem(QObject::tr("自动"), QStringLiteral("auto"));
    select->addItem(QObject::tr("手动"), QStringLiteral("manual"));
    select->addItem(QObject::tr("维护"), QStringLiteral("maintenance"));
    select->setCurrentIndex(-1);
    select->setPlaceholderText(QObject::tr("请选择"));
    select->setSelectSize(size);
    select->setSelectAppearance(appearance);
    select->setSelectStatus(status);
    select->setSelectionMode(mode);
    if (mode != commonWidget::Z_ComboBox::SelectionMode::Single)
        select->setSelectedValues({QStringLiteral("auto"), QStringLiteral("manual")});
    select->setMinimumWidth(220);
    select->setMaximumWidth(300);
    return select;
}

SectionPage createSelectSection(QWidget* parent) {
    auto* appearanceContent = new QWidget;
    auto* appearanceLayout = new QVBoxLayout(appearanceContent);
    appearanceLayout->setContentsMargins(20, 20, 20, 28);
    appearanceLayout->setSpacing(16);
    auto* appearanceCard = card(QObject::tr("Select / 外观与内容"),
        QObject::tr("Figma 20.5.02 · QComboBox 原生底座 + Z_ComboBox 选择适配；以下均为 SDK 真实实例。"),
        appearanceContent);
    auto* cardLayout = static_cast<QVBoxLayout*>(appearanceCard->layout());
    const struct {
        const char* name;
        commonWidget::Z_ComboBox::SelectionMode mode;
    } modes[] = {
        {"Single / 单值", commonWidget::Z_ComboBox::SelectionMode::Single},
        {"Multiple / 多值", commonWidget::Z_ComboBox::SelectionMode::Multiple},
        {"Tags / 多标签", commonWidget::Z_ComboBox::SelectionMode::Tags},
    };
    for (const auto& entry : modes) {
        cardLayout->addWidget(label(QString::fromUtf8(entry.name),
                                    QStringLiteral("baselinePageDescription"), appearanceCard));
        auto* specimen = selectSpecimen(appearanceCard, entry.mode);
        specimen->setObjectName(entry.mode == commonWidget::Z_ComboBox::SelectionMode::Single
                                    ? QStringLiteral("componentRoot_QComboBox")
                                    : QStringLiteral("selectSample_QComboBox_%1")
                                          .arg(static_cast<int>(entry.mode)));
        cardLayout->addWidget(specimen, 0, Qt::AlignLeft);
    }
    cardLayout->addWidget(label(
        QObject::tr("点击展开 Option；Multiple / Tags 可逐项切换，标签右侧可移除。"),
        QStringLiteral("baselinePageDescription"), appearanceCard));
    appearanceLayout->addWidget(appearanceCard);
    appearanceLayout->addStretch();

    auto* stateContent = new QWidget;
    auto* stateLayout = new QVBoxLayout(stateContent);
    stateLayout->setContentsMargins(20, 20, 20, 28);
    stateLayout->setSpacing(16);
    auto* stateCard = card(QObject::tr("Select / 分组与状态"),
        QObject::tr("4 外观 × 3 校验状态 × 3 尺寸；Hover、Focus、Open 可在实例上直接交互。"),
        stateContent);
    auto* matrix = new QGridLayout;
    matrix->setHorizontalSpacing(12);
    matrix->setVerticalSpacing(12);
    const QStringList sizes{QStringLiteral("Small"), QStringLiteral("Middle"),
                            QStringLiteral("Large")};
    const QStringList appearances{QStringLiteral("Outlined"), QStringLiteral("Filled"),
                                  QStringLiteral("Borderless"), QStringLiteral("Underlined")};
    const QStringList statuses{QStringLiteral("Normal"), QStringLiteral("Error"),
                               QStringLiteral("Warning")};
    matrix->addWidget(label(QObject::tr("外观 / 状态"), QStringLiteral("baselinePageDescription"),
                            stateCard), 0, 0);
    for (int column = 0; column < sizes.size(); ++column)
        matrix->addWidget(label(sizes.at(column), QStringLiteral("baselinePageDescription"),
                                stateCard), 0, column + 1);
    int row = 1;
    for (const QString& appearance : appearances) {
        for (const QString& status : statuses) {
            auto* rowLabel = label(appearance + QStringLiteral(" / ") + status,
                                   QStringLiteral("baselinePageDescription"), stateCard);
            rowLabel->setMaximumWidth(125);
            matrix->addWidget(rowLabel, row, 0);
            for (int column = 0; column < sizes.size(); ++column) {
                auto* specimen = selectSpecimen(stateCard,
                    commonWidget::Z_ComboBox::SelectionMode::Single,
                    sizes.at(column), appearance, status);
                specimen->setMinimumWidth(130);
                specimen->setMaximumWidth(150);
                specimen->setObjectName(QStringLiteral("selectSample_QComboBox_%1_%2_%3")
                                            .arg(appearance, status, sizes.at(column)));
                matrix->addWidget(specimen, row, column + 1);
            }
            ++row;
        }
    }
    static_cast<QVBoxLayout*>(stateCard->layout())->addLayout(matrix);
    auto* disabled = selectSpecimen(stateCard,
        commonWidget::Z_ComboBox::SelectionMode::Single);
    disabled->setObjectName(QStringLiteral("selectSample_QComboBox_Disabled"));
    disabled->setEnabled(false);
    static_cast<QVBoxLayout*>(stateCard->layout())->addWidget(label(
        QObject::tr("Disabled / 禁用"), QStringLiteral("baselinePageDescription"), stateCard));
    static_cast<QVBoxLayout*>(stateCard->layout())->addWidget(disabled, 0, Qt::AlignLeft);
    stateLayout->addWidget(stateCard);
    stateLayout->addStretch();
    return {scroll(appearanceContent, parent), scroll(stateContent, parent)};
}

enum class TableSpecimenPart { Combined, Header, Body };

commonWidget::Z_TableView* tableViewSpecimen(QWidget* parent, const QString& size,
                                               const QString& state,
                                               TableSpecimenPart part = TableSpecimenPart::Combined) {
    auto* table = new commonWidget::Z_TableView(parent);
    table->setObjectName(QStringLiteral("baselineTableView_%1_%2").arg(size, state));
    table->setProperty("figmaState", state);
    auto* model = new QStandardItemModel(part == TableSpecimenPart::Header ? 0 : 3, 4, table);
    model->setHorizontalHeaderLabels({QStringLiteral("Name"), QStringLiteral("Status"),
                                       QStringLiteral("Value"), QStringLiteral("操作")});
    const QStringList names{QStringLiteral("Alpha"), QStringLiteral("Beta"),
                            QStringLiteral("Gamma")};
    const QStringList statuses{QStringLiteral("Ready"), QStringLiteral("Pending"),
                               QStringLiteral("Idle")};
    const QStringList values{QStringLiteral("128"), QStringLiteral("64"),
                             QStringLiteral("32")};
    for (int row = 0; row < model->rowCount(); ++row) {
        model->setData(model->index(row, 0), names[row]);
        model->setData(model->index(row, 1), statuses[row]);
        model->setData(model->index(row, 2), values[row]);
    }
    table->setModel(model);
    table->setTableSize(size);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    if (part != TableSpecimenPart::Body) {
        table->setSortingEnabled(true);
        table->sortByColumn(0, Qt::AscendingOrder);
        auto* headerAction = new commonWidget::Z_ToolButton;
        headerAction->setObjectName(QStringLiteral("tableHeaderAction"));
        headerAction->setIcon(QIcon(QStringLiteral(":/z-native/icons/table-more.svg")));
        headerAction->setToolTip(QObject::tr("表头操作"));
        QObject::connect(headerAction, &QToolButton::clicked, table, [table] {
            table->setProperty("lastTableAction", QStringLiteral("header"));
        });
        table->setHeaderActionWidget(3, headerAction);
    }
    if (part != TableSpecimenPart::Header) {
        auto* rowAction = new commonWidget::Z_PushButton(QObject::tr("查看"));
        rowAction->setObjectName(QStringLiteral("tableCellAction"));
        QObject::connect(rowAction, &QPushButton::clicked, table, [table] {
            table->setProperty("lastTableAction", QStringLiteral("view"));
        });
        table->setCellActionWidget(model->index(0, 3), rowAction);
        auto* iconAction = new commonWidget::Z_ToolButton;
        iconAction->setObjectName(QStringLiteral("tableCellIconAction"));
        iconAction->setIcon(QIcon(QStringLiteral(":/z-native/icons/table-open.svg")));
        iconAction->setToolTip(QObject::tr("进入详情"));
        QObject::connect(iconAction, &QToolButton::clicked, table, [table] {
            table->setProperty("lastTableAction", QStringLiteral("details"));
        });
        table->setCellActionWidget(model->index(1, 3), iconAction);
    }
    if (part == TableSpecimenPart::Body)
        table->horizontalHeader()->hide();
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    if (state == QStringLiteral("Selected") || state == QStringLiteral("Selected-hover"))
        table->selectRow(0);
    if (state == QStringLiteral("Disabled"))
        table->setEnabled(false);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setMinimumWidth(320);
    table->setMaximumWidth(760);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    const int partHeight = size == QStringLiteral("Small") ? 39
                           : size == QStringLiteral("Medium") ? 47 : 55;
    table->setFixedHeight(part == TableSpecimenPart::Header ? partHeight + 2
                          : part == TableSpecimenPart::Body ? partHeight * 3 + 2
                          : partHeight * 4 + 2);
    return table;
}

SectionPage createTableViewSection(QWidget* parent) {
    auto* appearanceContent = new QWidget;
    auto* appearanceLayout = new QVBoxLayout(appearanceContent);
    appearanceLayout->setContentsMargins(20, 20, 20, 28);
    appearanceLayout->setSpacing(16);
    auto* headerCard = card(QObject::tr("Table/Header · 表头"),
        QObject::tr("QHeaderView：浅灰背景、强调字重、独立高度与表头操作 Slot。样例没有数据行。"),
        appearanceContent);
    auto* headerTable = tableViewSpecimen(headerCard, QStringLiteral("Medium"),
                                          QStringLiteral("Default"), TableSpecimenPart::Header);
    headerTable->setObjectName(QStringLiteral("tableHeaderSpecimen"));
    static_cast<QVBoxLayout*>(headerCard->layout())->addWidget(headerTable);
    appearanceLayout->addWidget(headerCard);

    auto* bodyCard = card(QObject::tr("Table/Body · 表格项"),
        QObject::tr("QTableView viewport：白色内容底、常规字重、独立行高与选中态。样例隐藏表头。"),
        appearanceContent);
    auto* bodyTable = tableViewSpecimen(bodyCard, QStringLiteral("Medium"),
                                        QStringLiteral("Default"), TableSpecimenPart::Body);
    bodyTable->setObjectName(QStringLiteral("tableBodySpecimen"));
    static_cast<QVBoxLayout*>(bodyCard->layout())->addWidget(bodyTable);
    appearanceLayout->addWidget(bodyCard);

    auto* appearanceCard = card(QObject::tr("TableView · 组合与交互"),
        QObject::tr("表头与数据项由同一个 SDK Z_TableView 组合；数据来自外部 QAbstractItemModel。"),
        appearanceContent);
    auto* appearanceTable = tableViewSpecimen(appearanceCard, QStringLiteral("Medium"),
                                               QStringLiteral("Default"));
    appearanceTable->setObjectName(QStringLiteral("componentRoot_QTableView"));
    static_cast<QVBoxLayout*>(appearanceCard->layout())->addWidget(appearanceTable);
    auto* tableControls = new QHBoxLayout;
    auto* striped = new commonWidget::Z_CheckBox(QObject::tr("斑马纹"), appearanceCard);
    striped->setObjectName(QStringLiteral("baselineTableViewStriped"));
    QObject::connect(striped, &QCheckBox::toggled, appearanceTable,
                     &commonWidget::Z_TableView::setTableStriped);
    tableControls->addWidget(striped);
    auto* lifecycle = new commonWidget::Z_ComboBox(appearanceCard);
    lifecycle->setObjectName(QStringLiteral("baselineTableViewLifecycle"));
    lifecycle->addItem(QObject::tr("数据就绪"), QStringLiteral("ready"));
    lifecycle->addItem(QObject::tr("加载中"), QStringLiteral("loading"));
    lifecycle->addItem(QObject::tr("空数据"), QStringLiteral("empty"));
    lifecycle->addItem(QObject::tr("无匹配"), QStringLiteral("no-match"));
    lifecycle->addItem(QObject::tr("错误"), QStringLiteral("error"));
    QObject::connect(lifecycle, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     appearanceTable, [appearanceTable, lifecycle](int) {
                         appearanceTable->setLifecycleState(lifecycle->currentData().toString());
                     });
    tableControls->addWidget(lifecycle);
    tableControls->addStretch();
    static_cast<QVBoxLayout*>(appearanceCard->layout())->addLayout(tableControls);
    static_cast<QVBoxLayout*>(appearanceCard->layout())->addWidget(label(
        QObject::tr("点击数据列头排序、点击行选择；表头/单元格按钮均由 SDK 承载。容器、表头、内容视觉分别来自 ZTableView.qss、ZTableHeader.qss、ZTableBody.qss。"),
        QStringLiteral("baselinePageDescription"), appearanceCard));
    appearanceLayout->addWidget(appearanceCard);
    appearanceLayout->addStretch();

    auto* stateContent = new QWidget;
    auto* stateLayout = new QVBoxLayout(stateContent);
    stateLayout->setContentsMargins(20, 20, 20, 28);
    auto* stateCard = card(QObject::tr("TableView / 分组与状态"),
        QObject::tr("Figma Size × State：Small / Medium / Large × Default / Hover / Selected / Selected-hover / Disabled。Hover 为指针瞬时态，请移动鼠标验证。"),
        stateContent);
    auto* tabs = new commonWidget::Z_TabWidget(stateCard);
    tabs->setObjectName(QStringLiteral("baselineTableViewSizeTabs"));
    const QStringList sizes{QStringLiteral("Small"), QStringLiteral("Medium"),
                            QStringLiteral("Large")};
    const QStringList states{QStringLiteral("Default"), QStringLiteral("Hover"),
                             QStringLiteral("Selected"), QStringLiteral("Selected-hover"),
                             QStringLiteral("Disabled")};
    for (const QString& size : sizes) {
        auto* group = new QWidget(tabs);
        auto* groupLayout = new QVBoxLayout(group);
        groupLayout->setContentsMargins(12, 12, 12, 20);
        groupLayout->setSpacing(12);
        for (const QString& state : states) {
            groupLayout->addWidget(label(size + QStringLiteral(" / ") + state,
                                         QStringLiteral("baselinePageDescription"), group));
            groupLayout->addWidget(tableViewSpecimen(group, size, state));
        }
        groupLayout->addStretch();
        tabs->addTab(scroll(group, tabs), size);
    }
    static_cast<QVBoxLayout*>(stateCard->layout())->addWidget(tabs);
    stateLayout->addWidget(stateCard);
    stateLayout->addStretch();
    return {scroll(appearanceContent, parent), scroll(stateContent, parent)};
}

SectionPage createComponentSection(const ScopeEntry& entry, QWidget* parent) {
    const QString qtClass = QString::fromLatin1(entry.qtClass);
    if (qtClass == QStringLiteral("QComboBox"))
        return createSelectSection(parent);
    if (qtClass == QStringLiteral("QTableView"))
        return createTableViewSection(parent);
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
        if (auto* menu = qobject_cast<QMenu*>(popup)) {
            QObject::connect(openButton, &QPushButton::clicked, menu,
                             [menu, openButton] {
                                 menu->popup(openButton->mapToGlobal(
                                     QPoint(0, openButton->height())));
                             });
        } else {
            popup->setWindowFlags(Qt::Dialog);
            QObject::connect(openButton, &QPushButton::clicked, popup, [popup] { popup->show(); });
        }
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

SectionPage createInputSection(QWidget* parent) {
    auto* appearanceContent = new QWidget;
    auto* appearanceLayout = new QVBoxLayout(appearanceContent);
    appearanceLayout->setContentsMargins(20, 20, 20, 28);
    appearanceLayout->setSpacing(16);
    auto* appearanceCard = card(QObject::tr("Input / 复合内容"),
                                QObject::tr("全部为 SDK 中的 Z_InputField 实例；图标、清除、密码显隐、搜索与加载可交互。"),
                                appearanceContent);
    struct InputSample {
        const char* name;
        commonWidget::Z_InputField::Kind kind;
    };
    const InputSample samples[]{
        {"Text / 无左图标 + 清除", commonWidget::Z_InputField::Kind::Text},
        {"Password / 密码显隐", commonWidget::Z_InputField::Kind::Password},
        {"Search / 搜索与清除", commonWidget::Z_InputField::Kind::Search},
        {"SearchLoading / 加载中", commonWidget::Z_InputField::Kind::Search},
        {"TextArea / 多行输入", commonWidget::Z_InputField::Kind::TextArea},
        {"Prefix icon / 左图标 + 右侧操作", commonWidget::Z_InputField::Kind::Text},
    };
    auto* sampleGrid = new QGridLayout;
    sampleGrid->setHorizontalSpacing(24);
    sampleGrid->setVerticalSpacing(16);
    for (int index = 0; index < 6; ++index) {
        auto* cell = new QWidget(appearanceCard);
        auto* cellLayout = new QVBoxLayout(cell);
        cellLayout->setContentsMargins(0, 0, 0, 0);
        cellLayout->setSpacing(6);
        cellLayout->addWidget(label(QString::fromUtf8(samples[index].name),
                                    QStringLiteral("baselinePageDescription"), cell));
        auto* input = new commonWidget::Z_InputField(cell);
        input->setObjectName(QStringLiteral("componentRoot_Input_%1").arg(index));
        input->setKind(samples[index].kind);
        input->setPlaceholder(index == 4 ? QObject::tr("输入多行说明…") : QObject::tr("输入内容…"));
        input->setAllowClear(index == 0 || index == 2);
        if (index == 0 || index == 2)
            input->setValue(QObject::tr("ComponentLab"));
        if (index == 5)
            input->setPrefixIcon(QIcon(QStringLiteral(":/z-native/icons/input-search.svg")));
        if (index == 1)
            input->setValue(QStringLiteral("secret42"));
        if (index == 3)
            input->setLoading(true);
        if (index == 4)
            input->setValue(QObject::tr("第一行\n第二行"));
        if (index == 5)
            input->setActionIcon(QIcon(QStringLiteral(":/z-native/icons/input-search.svg")));
        input->setMinimumWidth(260);
        cellLayout->addWidget(input);
        sampleGrid->addWidget(cell, index / 2, index % 2);
    }
    static_cast<QVBoxLayout*>(appearanceCard->layout())->addLayout(sampleGrid);
    appearanceLayout->addWidget(appearanceCard);
    appearanceLayout->addStretch();

    auto* stateContent = new QWidget;
    auto* stateLayout = new QVBoxLayout(stateContent);
    stateLayout->setContentsMargins(20, 20, 20, 28);
    auto* stateCard = card(QObject::tr("Input / 分组与状态"),
                           QObject::tr("对应 Figma DSM/Input 的 Size × State；单行文本居中，多行文本靠左上。"),
                           stateContent);
    auto* matrix = new QGridLayout;
    matrix->setHorizontalSpacing(16);
    matrix->setVerticalSpacing(12);
    matrix->setColumnStretch(4, 1);
    const QStringList sizes{QStringLiteral("Small"), QStringLiteral("Medium"), QStringLiteral("Large")};
    const QStringList states{QStringLiteral("Default"), QStringLiteral("Hover"),
                             QStringLiteral("Focus"), QStringLiteral("Filled"),
                             QStringLiteral("Error"), QStringLiteral("Disabled")};
    for (int column = 0; column < sizes.size(); ++column)
        matrix->addWidget(label(sizes.at(column), QStringLiteral("baselinePageDescription"), stateCard),
                          0, column + 1);
    for (int row = 0; row < states.size(); ++row) {
        matrix->addWidget(label(states.at(row), QStringLiteral("baselinePageDescription"), stateCard),
                          row + 1, 0);
        for (int column = 0; column < sizes.size(); ++column) {
            auto* input = new commonWidget::Z_InputField(stateCard);
            input->setObjectName(QStringLiteral("inputMatrix_%1_%2")
                                     .arg(sizes.at(column), states.at(row)));
            input->setControlSize(sizes.at(column));
            input->setStatus(states.at(row));
            input->setPlaceholder(QObject::tr("Input text"));
            input->setFixedWidth(140);
            if (states.at(row) == QStringLiteral("Filled"))
                input->setValue(QObject::tr("Entered value"));
            if (states.at(row) == QStringLiteral("Disabled"))
                input->setEnabled(false);
            matrix->addWidget(input, row + 1, column + 1);
        }
    }
    static_cast<QVBoxLayout*>(stateCard->layout())->addLayout(matrix);
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
    layout->addWidget(label(QObject::tr("SDK 公开映射"), QStringLiteral("baselinePageTitle"), host));
    layout->addWidget(label(QObject::tr("58 个 Qt 底层包装与 1 个 Input 组件；QLineEdit 仅在 Input 内部使用，不单独发布。"),
                            QStringLiteral("baselineMappingDescription"), host));
    auto* table = new commonWidget::Z_TableWidget(host);
    table->setObjectName(QStringLiteral("publishedNativeScope"));
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({QObject::tr("Qt 原生类"), QObject::tr("公开包装"),
                                      QObject::tr("发布状态")});
    table->setRowCount(publishedScope().size() + 1);
    for (int row = 0; row < publishedScope().size(); ++row) {
        const auto& entry = publishedScope().at(row);
        table->setItem(row, 0, new QTableWidgetItem(QString::fromLatin1(entry.qtClass)));
        table->setItem(row, 1, new QTableWidgetItem(QString::fromLatin1(entry.sdkClass)));
        table->setItem(row, 2, new QTableWidgetItem(
            QString::fromLatin1(entry.qtClass) == QStringLiteral("QComboBox")
                ? QStringLiteral("Select adapter")
                : QString::fromLatin1(entry.qtClass) == QStringLiteral("QTableView")
                    ? QStringLiteral("QSS-driven model/view")
                    : QStringLiteral("baseline-native")));
    }
    const int compositeRow = publishedScope().size();
    table->setItem(compositeRow, 0, new QTableWidgetItem(QStringLiteral("Qt 文本编辑器（内部）")));
    table->setItem(compositeRow, 1, new QTableWidgetItem(QStringLiteral("commonWidget::Z_InputField")));
    table->setItem(compositeRow, 2, new QTableWidgetItem(QStringLiteral("QSS-driven composite")));
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
    setWindowTitle(QStringLiteral("Component Lab | GeneratedComponentSDK"));
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
    titleLayout->addWidget(label(tr("Published SDK consumer"),
                                 QStringLiteral("baselineGallerySubtitle"), header));
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();
    headerLayout->addWidget(label(tr("59 个已发布组件"), QStringLiteral("baselineReleaseBadge"), header));
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
    navigation_->setHeaderLabel(tr("SDK 组件"));
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
                                               {tr("组件（%1）").arg(publishedScope().size() + 1)});
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
        if (qtClass == QStringLiteral("QComboBox"))
            leaf->setText(0, tr("Z_ComboBox · Select"));
        if (qtClass == QStringLiteral("QTableView"))
            leaf->setText(0, tr("Z_TableView · TableView"));
        leaf->setData(0, Qt::UserRole, stack->count());
        leaf->setData(0, Qt::UserRole + 1, qtClass);
        leaf->setToolTip(0, QString::fromLatin1(entry.sdkClass));
        const auto section = createComponentSection(entry, stack);
        const bool selectPage = qtClass == QStringLiteral("QComboBox");
        const bool tableViewPage = qtClass == QStringLiteral("QTableView");
        auto* page = wrapPage(category,
                             selectPage ? tr("20.5.02 · Select / Z_ComboBox")
                                        : tableViewPage ? tr("20.7.06 · TableView / Z_TableView")
                                        : qtClass + QStringLiteral(" / ") +
                                              QString::fromLatin1(entry.sdkClass).section("::", -1),
                             selectPage ? tr("Figma Select → QComboBox → Z_ComboBox；Option 与多标签由 SDK 实现")
                                        : tableViewPage ? tr("Figma TableView → QTableView → Z_TableView；外部 Model、原生选择与排序")
                                        : tr("单组件展示 · 外观与分组状态分别查看"),
                             section, stack);
        page->setProperty("baselineWideMatrixPage", selectPage);
        connect(page->findChild<QTabWidget*>(QStringLiteral("baselineComponentTabs")),
                &QTabWidget::currentChanged, this,
                [this, page](int) {
                    if (pages_->isVisible() &&
                        qobject_cast<QStackedWidget*>(pages_)->currentWidget() == page) {
                        auto* tabs = page->findChild<QTabWidget*>(
                            QStringLiteral("baselineComponentTabs"));
                        objectInspector_->setVisible(
                            !page->property("baselineWideMatrixPage").toBool() ||
                            tabs->currentIndex() == 0);
                        populateObjectInspector(page);
                    }
                });
        stack->addWidget(page);
        if (qtClass == QStringLiteral("QPushButton"))
            defaultLeaf = leaf;
    }
    {
        const QString category = QStringLiteral("Input / 输入");
        auto* leaf = new QTreeWidgetItem(categoryItems.value(category), {tr("Input / 输入框")});
        leaf->setData(0, Qt::UserRole, stack->count());
        leaf->setData(0, Qt::UserRole + 1, QStringLiteral("ComponentLab/Input"));
        leaf->setToolTip(0, QStringLiteral("commonWidget::Z_InputField"));
        auto* page = wrapPage(category, tr("20.4.01 · Input / 输入框"),
                              tr("公开类 commonWidget::Z_InputField；QLineEdit 仅负责内部文本编辑。"),
                              createInputSection(stack), stack);
        page->setProperty("baselineWideMatrixPage", true);
        connect(page->findChild<QTabWidget*>(QStringLiteral("baselineComponentTabs")),
                &QTabWidget::currentChanged, this, [this, page](int) {
                    if (pages_->isVisible() &&
                        qobject_cast<QStackedWidget*>(pages_)->currentWidget() == page) {
                        auto* tabs = page->findChild<QTabWidget*>(QStringLiteral("baselineComponentTabs"));
                        objectInspector_->setVisible(tabs->currentIndex() == 0);
                        populateObjectInspector(page);
                    }
                });
        stack->addWidget(page);
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
    sdkTabs->addTab(canvas, tr("GeneratedComponentSDK"));
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
    QTreeWidgetItem* selectLeaf = nullptr;
    auto* published = navigation_->topLevelItem(0);
    for (int section = 0; section < published->childCount() && !selectLeaf; ++section) {
        auto* category = published->child(section);
        for (int item = 0; item < category->childCount(); ++item) {
            auto* leaf = category->child(item);
            if (leaf->data(0, Qt::UserRole + 1).toString() == QStringLiteral("QComboBox")) {
                selectLeaf = leaf;
                break;
            }
        }
    }
    if (!selectLeaf)
        return false;
    selectLeaf->parent()->setExpanded(true);
    navigation_->setCurrentItem(selectLeaf);
    auto* selectPageWidget = qobject_cast<QStackedWidget*>(pages_)->currentWidget();
    auto* selectTabs = selectPageWidget->findChild<QTabWidget*>(
        QStringLiteral("baselineComponentTabs"));
    if (!selectTabs)
        return false;
    selectTabs->setCurrentIndex(0);
    qApp->processEvents();
    if (!grab().save(directory + QStringLiteral("/select-appearance.png")))
        return false;
    auto* tags = selectPageWidget->findChild<commonWidget::Z_ComboBox*>(
        QStringLiteral("selectSample_QComboBox_2"));
    if (!tags)
        return false;
    tags->showPopup();
    qApp->processEvents();
    const bool popupCaptured = tags->view()->grab().save(
        directory + QStringLiteral("/select-option-popup.png"));
    tags->hidePopup();
    if (!popupCaptured)
        return false;
    selectTabs->setCurrentIndex(1);
    qApp->processEvents();
    if (!grab().save(directory + QStringLiteral("/select-state-matrix.png")))
        return false;
    const auto captureNativeInput = [&](const QString& qtClass, const QString& fileName) {
        for (int section = 0; section < published->childCount(); ++section) {
            auto* category = published->child(section);
            for (int item = 0; item < category->childCount(); ++item) {
                auto* leaf = category->child(item);
                if (leaf->data(0, Qt::UserRole + 1).toString() != qtClass)
                    continue;
                category->setExpanded(true);
                navigation_->setCurrentItem(leaf);
                qApp->processEvents();
                return grab().save(directory + fileName);
            }
        }
        return false;
    };
    if (!captureNativeInput(QStringLiteral("QTextEdit"), QStringLiteral("/text-edit.png")) ||
        !captureNativeInput(QStringLiteral("QPlainTextEdit"), QStringLiteral("/plain-text-edit.png")))
        return false;
    if (!captureNativeInput(QStringLiteral("ComponentLab/Input"), QStringLiteral("/input-appearance.png")))
        return false;
    auto* inputPageWidget = qobject_cast<QStackedWidget*>(pages_)->currentWidget();
    auto* inputTabs = inputPageWidget->findChild<QTabWidget*>(QStringLiteral("baselineComponentTabs"));
    if (!inputTabs)
        return false;
    inputTabs->setCurrentIndex(1);
    qApp->processEvents();
    if (!grab().save(directory + QStringLiteral("/input-state-matrix.png")))
        return false;
    if (!captureNativeInput(QStringLiteral("QTableView"),
                            QStringLiteral("/table-view-appearance.png")))
        return false;
    auto* tablePageWidget = qobject_cast<QStackedWidget*>(pages_)->currentWidget();
    auto* tableTabs = tablePageWidget->findChild<QTabWidget*>(
        QStringLiteral("baselineComponentTabs"));
    if (!tableTabs)
        return false;
    tableTabs->setCurrentIndex(1);
    qApp->processEvents();
    if (!grab().save(directory + QStringLiteral("/table-view-state-matrix.png")))
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
                       {QStringLiteral("compositeComponentCount"), 1},
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
    auto* tabs = stack->currentWidget()->findChild<QTabWidget*>(
        QStringLiteral("baselineComponentTabs"));
    objectInspector_->setVisible(!stack->currentWidget()->property("baselineWideMatrixPage").toBool() ||
                                 !tabs || tabs->currentIndex() == 0);
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
