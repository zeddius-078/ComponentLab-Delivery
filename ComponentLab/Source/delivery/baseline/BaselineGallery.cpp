#include "BaselineGallery.h"

#include <MyDesignSystem/MyDesignSystem.h>

#include <QApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QSaveFile>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyle>
#include <QStyleOptionButton>
#include <QTableWidget>
#include <QTabWidget>
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

void appendObjectTree(QTreeWidgetItem* parent, QWidget* widget) {
    if (!widget)
        return;
    const QString name = widget->objectName().isEmpty()
                             ? QString::fromLatin1(widget->metaObject()->className())
                             : widget->objectName();
    auto* item = new QTreeWidgetItem(parent,
                                     {name, QString::fromLatin1(widget->metaObject()->className())});
    item->setExpanded(true);
    addProperty(item, QStringLiteral("enabled"), widget->isEnabled());
    addProperty(item, QStringLiteral("visible"), widget->isVisible());
    for (const auto& property : {"nativeQtClass", "designSystemClass"}) {
        const QVariant value = widget->property(property);
        if (value.isValid())
            addProperty(item, QString::fromLatin1(property), value);
    }
    for (auto* child : widget->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
        appendObjectTree(item, child);
}

QColor indeterminateDashPixel(QMainWindow* window) {
    auto* checkbox = window->findChild<commonWidget::Z_CheckBox*>(
        QStringLiteral("baselineIndeterminateCheckbox"));
    if (!checkbox || checkbox->checkState() != Qt::PartiallyChecked)
        return {};
    QStyleOptionButton option;
    option.initFrom(checkbox);
    option.state |= QStyle::State_NoChange;
    const QRect indicator = checkbox->style()->subElementRect(
        QStyle::SE_CheckBoxIndicator, &option, checkbox);
    return checkbox->grab().toImage().pixelColor(indicator.center());
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

SectionPage createActionSection(QWidget* parent) {
    auto* appearanceContent = new QWidget;
    auto* appearance = new QVBoxLayout(appearanceContent);
    appearance->setContentsMargins(20, 20, 20, 28);
    appearance->setSpacing(16);
    auto* buttons = card(QObject::tr("Button / 按钮"),
                         QObject::tr("所有样例均由已发布的 Z_* 公共头文件实例化。"),
                         appearanceContent);
    auto* buttonRow = new QHBoxLayout;
    buttonRow->setSpacing(12);
    auto* primary = new commonWidget::Z_PushButton(QObject::tr("Primary action"), buttons);
    primary->setObjectName(QStringLiteral("baselinePrimaryButton"));
    auto* secondary = new commonWidget::Z_PushButton(QObject::tr("Secondary action"), buttons);
    secondary->setObjectName(QStringLiteral("baselineSecondaryButton"));
    auto* tool = new commonWidget::Z_ToolButton(buttons);
    tool->setText(QObject::tr("More"));
    tool->setObjectName(QStringLiteral("baselineToolButton"));
    auto* command = new commonWidget::Z_CommandLinkButton(QObject::tr("查看包装合同"), buttons);
    command->setObjectName(QStringLiteral("baselineCommandLink"));
    buttonRow->addWidget(primary);
    buttonRow->addWidget(secondary);
    buttonRow->addWidget(tool);
    buttonRow->addWidget(command);
    buttonRow->addStretch();
    static_cast<QVBoxLayout*>(buttons->layout())->addLayout(buttonRow);
    appearance->addWidget(buttons);

    auto* select = card(QObject::tr("Checkbox 与 Radio / 复选与单选"),
                        QObject::tr("半选保持 SDK 的紫底白色横杠，状态不是静态图片。"),
                        appearanceContent);
    auto* matrix = new QGridLayout;
    matrix->setHorizontalSpacing(18);
    matrix->setVerticalSpacing(10);
    for (int index = 0; index < 4; ++index)
        matrix->addWidget(new QLabel(QStringList{QObject::tr("未选"), QObject::tr("选中"),
                                                 QObject::tr("半选"), QObject::tr("禁用")}
                                        .at(index), select),
                          0, index);
    auto* unchecked = new commonWidget::Z_CheckBox(QObject::tr("复选状态"), select);
    auto* checked = new commonWidget::Z_CheckBox(QObject::tr("复选状态"), select);
    checked->setChecked(true);
    auto* indeterminate = new commonWidget::Z_CheckBox(QObject::tr("半选状态"), select);
    indeterminate->setObjectName(QStringLiteral("baselineIndeterminateCheckbox"));
    indeterminate->setTristate(true);
    indeterminate->setCheckState(Qt::PartiallyChecked);
    auto* disabled = new commonWidget::Z_CheckBox(QObject::tr("不可用"), select);
    disabled->setChecked(true);
    disabled->setEnabled(false);
    matrix->addWidget(unchecked, 1, 0);
    matrix->addWidget(checked, 1, 1);
    matrix->addWidget(indeterminate, 1, 2);
    matrix->addWidget(disabled, 1, 3);
    auto* radio = new commonWidget::Z_RadioButton(QObject::tr("默认"), select);
    auto* selectedRadio = new commonWidget::Z_RadioButton(QObject::tr("选中"), select);
    selectedRadio->setChecked(true);
    auto* disabledRadio = new commonWidget::Z_RadioButton(QObject::tr("不可用"), select);
    disabledRadio->setEnabled(false);
    matrix->addWidget(radio, 2, 0);
    matrix->addWidget(selectedRadio, 2, 1);
    matrix->addWidget(disabledRadio, 2, 3);
    static_cast<QVBoxLayout*>(select->layout())->addLayout(matrix);
    appearance->addWidget(select);
    appearance->addStretch();

    auto* stateContent = new QWidget;
    auto* stateLayout = new QVBoxLayout(stateContent);
    stateLayout->setContentsMargins(20, 20, 20, 28);
    auto* note = new QWidget(stateContent);
    note->setObjectName(QStringLiteral("baselineStateNote"));
    auto* noteLayout = new QVBoxLayout(note);
    noteLayout->setContentsMargins(14, 10, 14, 10);
    noteLayout->addWidget(label(QObject::tr("状态由控件自身驱动：可点击、可切换，非截图或仿制外观。"),
                                QStringLiteral("baselineStateNoteText"), note));
    stateLayout->addWidget(note);
    auto* table = new commonWidget::Z_TableWidget(5, 3, stateContent);
    table->setObjectName(QStringLiteral("baselineActionStateTable"));
    table->setHorizontalHeaderLabels({QObject::tr("组件"), QObject::tr("状态"),
                                      QObject::tr("运行时证据")});
    const QList<QStringList> rows{
        {QObject::tr("Z_PushButton"), QObject::tr("default / hover / disabled"), QObject::tr("Qt pressed")},
        {QObject::tr("Z_CheckBox"), QObject::tr("unchecked / checked / indeterminate"), QObject::tr("白色横杠")},
        {QObject::tr("Z_RadioButton"), QObject::tr("default / selected / disabled"), QObject::tr("Qt exclusive")},
        {QObject::tr("Z_ToolButton"), QObject::tr("default / pressed"), QObject::tr("公共包装")},
        {QObject::tr("Z_CommandLinkButton"), QObject::tr("default / disabled"), QObject::tr("公共包装")},
    };
    for (int row = 0; row < rows.size(); ++row)
        for (int column = 0; column < rows.at(row).size(); ++column)
            table->setItem(row, column, new QTableWidgetItem(rows.at(row).at(column)));
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    stateLayout->addWidget(table);
    stateLayout->addStretch();
    return {scroll(appearanceContent, parent), scroll(stateContent, parent)};
}

SectionPage createInputSection(QWidget* parent) {
    auto* appearanceContent = new QWidget;
    auto* appearance = new QVBoxLayout(appearanceContent);
    appearance->setContentsMargins(20, 20, 20, 28);
    appearance->setSpacing(16);
    auto* formCard = card(QObject::tr("Input / 输入"),
                          QObject::tr("字段边框、字体与焦点色均来自 SDK 固定 QSS。"), appearanceContent);
    auto* form = new QFormLayout;
    form->setSpacing(12);
    auto* line = new commonWidget::Z_LineEdit(formCard);
    line->setObjectName(QStringLiteral("baselineLineEdit"));
    line->setPlaceholderText(QObject::tr("输入文本"));
    auto* combo = new commonWidget::Z_ComboBox(formCard);
    combo->setObjectName(QStringLiteral("baselineComboBox"));
    combo->addItems({QObject::tr("选项 A"), QObject::tr("选项 B"), QObject::tr("选项 C")});
    auto* spin = new commonWidget::Z_SpinBox(formCard);
    spin->setRange(0, 100);
    spin->setValue(42);
    auto* date = new commonWidget::Z_DateEdit(QDate::currentDate(), formCard);
    date->setCalendarPopup(true);
    form->addRow(QObject::tr("文本"), line);
    form->addRow(QObject::tr("选择"), combo);
    form->addRow(QObject::tr("数值"), spin);
    form->addRow(QObject::tr("日期"), date);
    static_cast<QVBoxLayout*>(formCard->layout())->addLayout(form);
    appearance->addWidget(formCard);
    auto* progressCard = card(QObject::tr("Progress / 进度"),
                              QObject::tr("进度颜色和圆角不由 Gallery 定义。"), appearanceContent);
    auto* progress = new commonWidget::Z_ProgressBar(progressCard);
    progress->setObjectName(QStringLiteral("baselineProgressBar"));
    progress->setValue(68);
    static_cast<QVBoxLayout*>(progressCard->layout())->addWidget(progress);
    appearance->addWidget(progressCard);
    appearance->addStretch();

    auto* stateContent = new QWidget;
    auto* stateLayout = new QVBoxLayout(stateContent);
    stateLayout->setContentsMargins(20, 20, 20, 28);
    auto* stateCard = card(QObject::tr("输入状态"),
                           QObject::tr("使用真实控件展示 placeholder、value 与 disabled。"), stateContent);
    auto* grid = new QGridLayout;
    grid->addWidget(new QLabel(QObject::tr("Placeholder"), stateCard), 0, 0);
    grid->addWidget(new QLabel(QObject::tr("Value"), stateCard), 0, 1);
    grid->addWidget(new QLabel(QObject::tr("Disabled"), stateCard), 0, 2);
    auto* placeholder = new commonWidget::Z_LineEdit(stateCard);
    placeholder->setPlaceholderText(QObject::tr("输入文本"));
    auto* value = new commonWidget::Z_LineEdit(QStringLiteral("HarmonyOS Sans SC"), stateCard);
    auto* unavailable = new commonWidget::Z_LineEdit(QObject::tr("不可编辑"), stateCard);
    unavailable->setEnabled(false);
    grid->addWidget(placeholder, 1, 0);
    grid->addWidget(value, 1, 1);
    grid->addWidget(unavailable, 1, 2);
    static_cast<QVBoxLayout*>(stateCard->layout())->addLayout(grid);
    stateLayout->addWidget(stateCard);
    stateLayout->addStretch();
    return {scroll(appearanceContent, parent), scroll(stateContent, parent)};
}

SectionPage createContainerSection(QWidget* parent) {
    auto* appearanceContent = new QWidget;
    auto* appearance = new QVBoxLayout(appearanceContent);
    appearance->setContentsMargins(20, 20, 20, 28);
    appearance->setSpacing(16);
    auto* dataCard = card(QObject::tr("Table、Tabs 与 Calendar / 数据与容器"),
                          QObject::tr("容器使用发布范围中的 Qt 原生包装类型。"), appearanceContent);
    auto* grid = new QGridLayout;
    auto* table = new commonWidget::Z_TableWidget(3, 2, dataCard);
    table->setObjectName(QStringLiteral("baselineTableWidget"));
    table->setHorizontalHeaderLabels({QObject::tr("参数"), QObject::tr("值")});
    const QList<QStringList> records{{QObject::tr("SDK"), QObject::tr("已发布")},
                                     {QObject::tr("QSS"), QObject::tr("QRC 内嵌")},
                                     {QObject::tr("Token JSON"), QObject::tr("运行时不读取")}};
    for (int row = 0; row < records.size(); ++row)
        for (int column = 0; column < records.at(row).size(); ++column)
            table->setItem(row, column, new QTableWidgetItem(records.at(row).at(column)));
    table->horizontalHeader()->setStretchLastSection(true);
    auto* tabs = new commonWidget::Z_TabWidget(dataCard);
    tabs->setObjectName(QStringLiteral("baselineDataTabs"));
    tabs->addTab(new commonWidget::Z_CalendarWidget(tabs), QObject::tr("Calendar"));
    auto* tree = new commonWidget::Z_TreeWidget(tabs);
    tree->setHeaderLabel(QObject::tr("TreeWidget"));
    new QTreeWidgetItem(tree, {QObject::tr("组件资源")});
    new QTreeWidgetItem(tree, {QObject::tr("发布合同")});
    tree->expandAll();
    tabs->addTab(tree, QObject::tr("Tree"));
    grid->addWidget(table, 0, 0);
    grid->addWidget(tabs, 0, 1);
    static_cast<QVBoxLayout*>(dataCard->layout())->addLayout(grid);
    appearance->addWidget(dataCard);
    auto* scrollCard = card(QObject::tr("ScrollArea / 滚动区域"),
                            QObject::tr("该样例验证真实滚动容器，而不是静态边框。"), appearanceContent);
    auto* scrollArea = new commonWidget::Z_ScrollArea(scrollCard);
    scrollArea->setObjectName(QStringLiteral("baselineScrollArea"));
    scrollArea->setWidgetResizable(true);
    auto* items = new QWidget(scrollArea);
    auto* itemsLayout = new QVBoxLayout(items);
    for (int index = 1; index <= 10; ++index)
        itemsLayout->addWidget(new QLabel(QObject::tr("滚动内容 %1").arg(index), items));
    itemsLayout->addStretch();
    scrollArea->setWidget(items);
    scrollArea->setMinimumHeight(120);
    static_cast<QVBoxLayout*>(scrollCard->layout())->addWidget(scrollArea);
    appearance->addWidget(scrollCard);
    appearance->addStretch();

    auto* stateContent = new QWidget;
    auto* stateLayout = new QVBoxLayout(stateContent);
    stateLayout->setContentsMargins(20, 20, 20, 28);
    auto* splitCard = card(QObject::tr("Splitter / 分割器"),
                           QObject::tr("拖动分隔条可改变两个真实区域的宽度。"), stateContent);
    auto* splitter = new commonWidget::Z_Splitter(Qt::Horizontal, splitCard);
    splitter->setObjectName(QStringLiteral("baselineDataSplitter"));
    auto* left = new QLabel(QObject::tr("左侧内容"), splitter);
    left->setAlignment(Qt::AlignCenter);
    auto* right = new QLabel(QObject::tr("可拖拽分割器\n右侧内容"), splitter);
    right->setAlignment(Qt::AlignCenter);
    splitter->addWidget(left);
    splitter->addWidget(right);
    splitter->setMinimumHeight(140);
    static_cast<QVBoxLayout*>(splitCard->layout())->addWidget(splitter);
    stateLayout->addWidget(splitCard);
    stateLayout->addStretch();
    return {scroll(appearanceContent, parent), scroll(stateContent, parent)};
}

SectionPage createWindowSection(QWidget* parent) {
    auto* appearanceContent = new QWidget;
    auto* appearance = new QVBoxLayout(appearanceContent);
    appearance->setContentsMargins(20, 20, 20, 28);
    appearance->setSpacing(16);
    auto* toolboxCard = card(QObject::tr("ToolBox / 工具箱"),
                             QObject::tr("呈现已发布的原生工具箱包装。"), appearanceContent);
    auto* toolbox = new commonWidget::Z_ToolBox(toolboxCard);
    toolbox->setObjectName(QStringLiteral("baselineToolBox"));
    toolbox->addItem(new QLabel(QObject::tr("工具箱内容 A"), toolbox), QObject::tr("常用操作"));
    toolbox->addItem(new QLabel(QObject::tr("工具箱内容 B"), toolbox), QObject::tr("更多组件"));
    toolbox->setMinimumHeight(130);
    static_cast<QVBoxLayout*>(toolboxCard->layout())->addWidget(toolbox);
    appearance->addWidget(toolboxCard);
    auto* mdiCard = card(QObject::tr("MDI SubWindow / 多文档窗口"),
                         QObject::tr("展示 Z_MdiArea 与 Z_MdiSubWindow 原生包装。"), appearanceContent);
    auto* mdi = new commonWidget::Z_MdiArea(mdiCard);
    mdi->setObjectName(QStringLiteral("baselineMdiArea"));
    auto* sub = new commonWidget::Z_MdiSubWindow(mdi);
    sub->setObjectName(QStringLiteral("baselineMdiSubWindow"));
    sub->setWidget(new QLabel(QObject::tr("MdiSubWindow 内容"), sub));
    sub->resize(260, 92);
    mdi->addSubWindow(sub);
    sub->show();
    mdi->setMinimumHeight(156);
    static_cast<QVBoxLayout*>(mdiCard->layout())->addWidget(mdi);
    appearance->addWidget(mdiCard);
    appearance->addStretch();

    auto* stateContent = new QWidget;
    auto* stateLayout = new QVBoxLayout(stateContent);
    stateLayout->setContentsMargins(20, 20, 20, 28);
    auto* stateCard = card(QObject::tr("发布边界"),
                           QObject::tr("基础 Gallery 只验证 Qt 原生包装；复合、自绘、页面型和实验性组件不在此包中实例化。"),
                           stateContent);
    auto* table = new commonWidget::Z_TableWidget(4, 2, stateCard);
    table->setHorizontalHeaderLabels({QObject::tr("项目"), QObject::tr("事实")});
    const QList<QStringList> rows{{QObject::tr("SDK 组件"), QObject::tr("59 个 Q* → Z_* 包装")},
                                  {QObject::tr("视觉资源"), QObject::tr("SDK 固定 QSS/QRC")},
                                  {QObject::tr("Token JSON"), QObject::tr("仅构建期")},
                                  {QObject::tr("展示职责"), QObject::tr("只消费 SDK")}};
    for (int row = 0; row < rows.size(); ++row)
        for (int column = 0; column < rows.at(row).size(); ++column)
            table->setItem(row, column, new QTableWidgetItem(rows.at(row).at(column)));
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    static_cast<QVBoxLayout*>(stateCard->layout())->addWidget(table);
    stateLayout->addWidget(stateCard);
    stateLayout->addStretch();
    return {scroll(appearanceContent, parent), scroll(stateContent, parent)};
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
    const auto actions = createActionSection(stack);
    stack->addWidget(wrapPage(tr("20.3 · Actions / 操作"), tr("Button、Checkbox 与 Radio"),
                              tr("以可操作的 Z_* 控件替代静态缩略图；状态由 Qt 事件驱动。"), actions, stack));
    const auto inputs = createInputSection(stack);
    stack->addWidget(wrapPage(tr("20.4 · Input / 输入"), tr("文本、选择与数值输入"),
                              tr("字段视觉来自 SDK 固定 QSS；Gallery 只负责展示结构。"), inputs, stack));
    const auto containers = createContainerSection(stack);
    stack->addWidget(wrapPage(tr("20.6 · Container / 容器"), tr("表格、标签页、滚动与分割"),
                              tr("容器保留真实滚动、选中和拖拽状态。"), containers, stack));
    const auto windows = createWindowSection(stack);
    stack->addWidget(wrapPage(tr("20.1 / 20.9 · Window & System"), tr("窗口与系统控件"),
                              tr("基础包不将旧版页面型或实验性组件伪装为 SDK。"), windows, stack));
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
    const auto addLeaf = [this](QTreeWidgetItem* parent, const QString& text, int page) {
        auto* item = new QTreeWidgetItem(parent, {text});
        item->setData(0, Qt::UserRole, page);
        return item;
    };
    auto* actionsNode = new QTreeWidgetItem(navigation_, {tr("20.3 · Actions / 操作")});
    auto* actionsLeaf = addLeaf(actionsNode, tr("Button & Selection / 按钮与选择"), 0);
    auto* inputNode = new QTreeWidgetItem(navigation_, {tr("20.4 · Input / 输入")});
    addLeaf(inputNode, tr("Input controls / 输入控件"), 1);
    auto* containerNode = new QTreeWidgetItem(navigation_, {tr("20.6 · Container / 容器")});
    addLeaf(containerNode, tr("Data & Layout / 数据与布局"), 2);
    auto* windowsNode = new QTreeWidgetItem(navigation_, {tr("20.1 / 20.9 · Window & System")});
    addLeaf(windowsNode, tr("Native window controls / 窗口控件"), 3);
    navigation_->expandAll();
    connect(navigation_, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item) {
        if (item && item->data(0, Qt::UserRole).isValid())
            selectPage(item->data(0, Qt::UserRole).toInt());
    });
    navigation_->setCurrentItem(actionsLeaf);
    galleryTabs->addTab(browse, tr("组件图库"));
    galleryTabs->addTab(createScopePage(galleryTabs), tr("组件映射"));
    canvasLayout->addWidget(galleryTabs);
    sdkTabs->addTab(canvas, tr("Z-Native SDK"));
    rootLayout->addWidget(sdkTabs, 1);
    setCentralWidget(root);
    setStatusBar(new commonWidget::Z_StatusBar(this));
    statusBar()->showMessage(tr("仅消费 GeneratedComponentSDK：SDK QSS/QRC 已加载；不读取 Token JSON。"));
}

bool BaselineWindow::capture(const QString& directory) {
    if (!QDir().mkpath(directory) || !grab().save(directory + QStringLiteral("/baseline-gallery.png")))
        return false;
    const QColor dash = indeterminateDashPixel(this);
    const auto* checkbox = findChild<commonWidget::Z_CheckBox*>(
        QStringLiteral("baselineIndeterminateCheckbox"));
    QJsonObject result{{QStringLiteral("packageKind"), QStringLiteral("qt-native-baseline")},
                       {QStringLiteral("nativeComponentCount"), publishedScope().size()},
                       {QStringLiteral("runtimeTokenJson"), false},
                       {QStringLiteral("legacyRuntime"), false},
                       {QStringLiteral("qssApplied"), !qApp->styleSheet().isEmpty()},
                       {QStringLiteral("presentation"), QStringLiteral("performance-lab-information-architecture")},
                       {QStringLiteral("gallerySections"), componentSectionCount()},
                       {QStringLiteral("objectInspector"), hasObjectInspector()},
                       {QStringLiteral("indeterminateClass"), checkbox ? checkbox->property("designSystemClass").toString() : QString{}},
                       {QStringLiteral("indeterminateDashPixel"), dash.name()},
                       {QStringLiteral("indeterminateDashVisible"), dash.red() > 220 && dash.green() > 220 && dash.blue() > 220}};
    QSaveFile file(directory + QStringLiteral("/runtime.json"));
    return file.open(QIODevice::WriteOnly) && file.write(QJsonDocument(result).toJson()) >= 0 && file.commit();
}

int BaselineWindow::componentSectionCount() const { return 4; }

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
    auto* root = new QTreeWidgetItem(objectInspector_,
                                     {tr("展示页面"), QString::fromLatin1(target->metaObject()->className())});
    root->setExpanded(true);
    appendObjectTree(root, target);
}
