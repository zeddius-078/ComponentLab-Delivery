#include <MyDesignSystem/MyDesignSystem.h>

#include <QApplication>
#include <QCommandLineParser>
#include <QDate>
#include <QDir>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QSaveFile>
#include <QStatusBar>
#include <QStyle>
#include <QStyleOptionButton>
#include <QTimer>
#include <QTreeWidget>

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

class BaselineWindow final : public QMainWindow {
  public:
    BaselineWindow() {
        setWindowTitle(QStringLiteral("Component Lab | Qt Native Baseline"));
        resize(1280, 820);
        setMinimumSize(920, 620);

        auto* nativeMenuBar = new commonWidget::Z_MenuBar(this);
        auto* componentMenu = new commonWidget::Z_Menu(tr("组件"), nativeMenuBar);
        componentMenu->addAction(tr("基础原生包装（59）"));
        nativeMenuBar->addMenu(componentMenu);
        setMenuBar(nativeMenuBar);
        auto* toolbar = new commonWidget::Z_ToolBar(tr("基础组件"), this);
        toolbar->addAction(tr("SDK：已发布"));
        addToolBar(toolbar);
        setStatusBar(new commonWidget::Z_StatusBar(this));
        statusBar()->showMessage(tr("仅消费 GeneratedComponentSDK：QSS/QRC 已由 SDK 提供；不读取 Token JSON。"));

        auto* splitter = new commonWidget::Z_Splitter(this);
        auto* navigation = new commonWidget::Z_TreeWidget(splitter);
        navigation->setHeaderLabel(tr("基础 SDK"));
        navigation->setMinimumWidth(240);
        auto* galleryItem = new QTreeWidgetItem(navigation, {tr("组件样例")});
        auto* scopeItem = new QTreeWidgetItem(navigation, {tr("已发布范围（59）")});
        navigation->expandAll();

        auto* pages = new commonWidget::Z_StackedWidget(splitter);
        pages->addWidget(createGalleryPage());
        pages->addWidget(createScopePage());
        splitter->addWidget(navigation);
        splitter->addWidget(pages);
        splitter->setStretchFactor(1, 1);
        setCentralWidget(splitter);
        connect(navigation, &QTreeWidget::currentItemChanged, this,
                [pages, galleryItem, scopeItem](QTreeWidgetItem* item) {
                    pages->setCurrentIndex(item == scopeItem ? 1 : 0);
                    if (item != galleryItem && item != scopeItem)
                        pages->setCurrentIndex(0);
                });
        navigation->setCurrentItem(galleryItem);
    }

    bool capture(const QString& directory) {
        if (!QDir().mkpath(directory))
            return false;
        if (!grab().save(directory + QStringLiteral("/baseline-gallery.png")))
            return false;
        QJsonObject result{{QStringLiteral("packageKind"), QStringLiteral("qt-native-baseline")},
                           {QStringLiteral("nativeComponentCount"), publishedScope().size()},
                           {QStringLiteral("runtimeTokenJson"), false},
                           {QStringLiteral("legacyRuntime"), false},
                           {QStringLiteral("qssApplied"), !qApp->styleSheet().isEmpty()},
                           {QStringLiteral("indeterminateClass"), indeterminateClass()},
                           {QStringLiteral("indeterminateDashPixel"), indeterminateDashPixel().name()},
                           {QStringLiteral("indeterminateDashVisible"), indeterminateDashVisible()}};
        QSaveFile file(directory + QStringLiteral("/runtime.json"));
        return file.open(QIODevice::WriteOnly) &&
               file.write(QJsonDocument(result).toJson()) >= 0 && file.commit();
    }

  private:
    QColor indeterminateDashPixel() {
        auto* checkbox = findChild<commonWidget::Z_CheckBox*>(
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

    QString indeterminateClass() const {
        const auto* checkbox = findChild<commonWidget::Z_CheckBox*>(
            QStringLiteral("baselineIndeterminateCheckbox"));
        return checkbox ? checkbox->property("designSystemClass").toString() : QString{};
    }

    bool indeterminateDashVisible() {
        const QColor center = indeterminateDashPixel();
        return center.red() > 220 && center.green() > 220 && center.blue() > 220;
    }

    static QWidget* createGalleryPage() {
        auto* page = new commonWidget::Z_ScrollArea;
        page->setWidgetResizable(true);
        auto* content = new commonWidget::Z_Widget(page);
        auto* layout = new QVBoxLayout(content);
        layout->setContentsMargins(28, 24, 28, 28);
        layout->setSpacing(18);

        auto* title = new commonWidget::Z_Label(
            QObject::tr("Qt 原生包装组件：运行时仅使用 SDK 的固定 QSS"), content);
        title->setObjectName(QStringLiteral("baselineTitle"));
        layout->addWidget(title);

        auto* actions = new commonWidget::Z_GroupBox(QObject::tr("操作与选择"), content);
        auto* actionLayout = new QHBoxLayout(actions);
        auto* primary = new commonWidget::Z_PushButton(QObject::tr("确认"), actions);
        primary->setObjectName(QStringLiteral("baselinePrimaryButton"));
        auto* tool = new commonWidget::Z_ToolButton(actions);
        tool->setText(QObject::tr("更多"));
        auto* check = new commonWidget::Z_CheckBox(QObject::tr("复选状态"), actions);
        check->setObjectName(QStringLiteral("baselineIndeterminateCheckbox"));
        check->setTristate(true);
        check->setCheckState(Qt::PartiallyChecked);
        check->setObjectName(QStringLiteral("baselineIndeterminateCheckbox"));
        auto* radio = new commonWidget::Z_RadioButton(QObject::tr("单选状态"), actions);
        radio->setChecked(true);
        actionLayout->addWidget(primary);
        actionLayout->addWidget(tool);
        actionLayout->addWidget(check);
        actionLayout->addWidget(radio);
        actionLayout->addStretch();
        layout->addWidget(actions);

        auto* input = new commonWidget::Z_GroupBox(QObject::tr("输入与展示"), content);
        auto* inputLayout = new QFormLayout(input);
        auto* lineEdit = new commonWidget::Z_LineEdit(input);
        lineEdit->setPlaceholderText(QObject::tr("输入文本"));
        auto* combo = new commonWidget::Z_ComboBox(input);
        combo->addItems({QObject::tr("选项 A"), QObject::tr("选项 B"), QObject::tr("选项 C")});
        auto* spin = new commonWidget::Z_SpinBox(input);
        spin->setRange(0, 100);
        spin->setValue(42);
        auto* progress = new commonWidget::Z_ProgressBar(input);
        progress->setRange(0, 100);
        progress->setValue(68);
        inputLayout->addRow(QObject::tr("文本"), lineEdit);
        inputLayout->addRow(QObject::tr("选择"), combo);
        inputLayout->addRow(QObject::tr("数值"), spin);
        inputLayout->addRow(QObject::tr("进度"), progress);
        layout->addWidget(input);

        auto* data = new commonWidget::Z_GroupBox(QObject::tr("数据与容器"), content);
        auto* dataLayout = new QGridLayout(data);
        auto* table = new commonWidget::Z_TableWidget(3, 2, data);
        table->setHorizontalHeaderLabels({QObject::tr("参数"), QObject::tr("值")});
        table->setItem(0, 0, new QTableWidgetItem(QObject::tr("状态")));
        table->setItem(0, 1, new QTableWidgetItem(QObject::tr("已发布")));
        table->setItem(1, 0, new QTableWidgetItem(QObject::tr("Token JSON")));
        table->setItem(1, 1, new QTableWidgetItem(QObject::tr("运行时不读取")));
        table->setItem(2, 0, new QTableWidgetItem(QObject::tr("字体")));
        table->setItem(2, 1, new QTableWidgetItem(QStringLiteral("HarmonyOS Sans / SC")));
        table->horizontalHeader()->setStretchLastSection(true);
        auto* tabs = new commonWidget::Z_TabWidget(data);
        tabs->addTab(new commonWidget::Z_Label(QObject::tr("由 SDK QRC 提供资源"), tabs),
                     QObject::tr("资源"));
        tabs->addTab(new commonWidget::Z_CalendarWidget(tabs), QObject::tr("日历"));
        dataLayout->addWidget(table, 0, 0);
        dataLayout->addWidget(tabs, 0, 1);
        layout->addWidget(data);
        layout->addStretch();
        page->setWidget(content);
        return page;
    }

    static QWidget* createScopePage() {
        auto* table = new commonWidget::Z_TableWidget;
        table->setObjectName(QStringLiteral("publishedNativeScope"));
        table->setColumnCount(3);
        table->setHorizontalHeaderLabels(
            {QObject::tr("Qt 原生类"), QObject::tr("公开包装"), QObject::tr("发布状态")});
        table->setRowCount(publishedScope().size());
        for (int row = 0; row < publishedScope().size(); ++row) {
            const auto& entry = publishedScope().at(row);
            table->setItem(row, 0, new QTableWidgetItem(QString::fromLatin1(entry.qtClass)));
            table->setItem(row, 1, new QTableWidgetItem(QString::fromLatin1(entry.sdkClass)));
            table->setItem(row, 2, new QTableWidgetItem(QStringLiteral("baseline-native")));
        }
        table->horizontalHeader()->setStretchLastSection(true);
        return table;
    }
};

}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("Component Lab"));
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({QStringLiteral("capture-dir"), QStringLiteral("保存基线图库截图"),
                      QStringLiteral("path")});
    parser.addOption({QStringLiteral("quit-after"), QStringLiteral("验证时自动退出"),
                      QStringLiteral("ms")});
    parser.process(app);

    commonWidget::initializeNativeComponentResources();
    app.setStyleSheet(commonWidget::nativeComponentStyleSheet());
    BaselineWindow window;
    window.show();
    if (parser.isSet(QStringLiteral("capture-dir"))) {
        QTimer::singleShot(350, &window, [&] {
            const bool captured = window.capture(parser.value(QStringLiteral("capture-dir")));
            if (!captured)
                app.exit(2);
            // A verification runner also needs a live process long enough to
            // inspect its loaded Qt plugins.  Interactive one-shot capture
            // remains backwards-compatible when no quit timer is requested.
            else if (!parser.isSet(QStringLiteral("quit-after")))
                app.exit(0);
        });
    }
    if (parser.isSet(QStringLiteral("quit-after"))) {
        bool valid = false;
        const int milliseconds = parser.value(QStringLiteral("quit-after")).toInt(&valid);
        if (!valid || milliseconds < 100)
            return 2;
        QTimer::singleShot(milliseconds, &app, &QCoreApplication::quit);
    }
    return app.exec();
}
