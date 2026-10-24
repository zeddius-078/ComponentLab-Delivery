// V2 端到端 QtTest：静态 XML、组件调用、交互、重复生成及设计树对应关系。
// D2C_V2_RESULT 指向 HTML + DSL 结果页，D2C_V2_REPORT 保存本路线报告；临时改动使用 QTemporaryDir。
#include "WorkflowWindow.h"
#include "ComponentTheme.h"
#include "Z_InputField.h"
#include "Z_PushButton.h"
#include "Z_CheckBox.h"
#include <QTest>
#include <QSignalSpy>
#include <QFontDatabase>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QListView>
#include <QFile>
#include <QJsonDocument>
#include <QDir>
#include <QLabel>
#include <functional>
#include <QTemporaryDir>
#include <QProgressBar>
#include <QDomDocument>
#include <QProcess>
#include <QCryptographicHash>
using namespace v2;
using namespace commonWidget;

class V2Tests : public QObject {
    Q_OBJECT
    const QString data = qEnvironmentVariableIsSet("D2C_V2_DATA")
                             ? qEnvironmentVariable("D2C_V2_DATA")
                             : QString(D2C_ROOT) + "/../output/work/demo/page_examples/V2/data/html-dsl";
    const QString report = qEnvironmentVariableIsSet("D2C_V2_REPORT")
                               ? qEnvironmentVariable("D2C_V2_REPORT")
                               : QString(D2C_ROOT) + "/../output/logs/v2";
    const QString resultOverride = qEnvironmentVariable("D2C_V2_RESULT");
    QJsonObject baseline, resized;

    QByteArray bytes(const QString& name) {
        QFile file(name);
        // 测试输入必须可读取，否则立即报错，避免空字节哈希或空内容让后续比较失去意义。
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("TEST_INPUT_MISSING");
        return file.readAll();
    }

    QByteArray digest(const QString& name) {
        return QCryptographicHash::hash(bytes(name), QCryptographicHash::Sha256).toHex();
    }

    QStringList compilerArgs(const QString& raw, const QString& output,
                             const QString& libraryProfile = {}) {
        const auto root = QString(FIXTURE_ROOT);
        auto args = QStringList{"--raw",        raw,
                                "--mapping",    root + "/mapping",
                                "--extensions", root + "/mapping/extensions.json",
                                "--font",       data + "/HarmonyOS_Sans_SC_Regular.ttf",
                                "--output",     output};
        if (!libraryProfile.isEmpty())
            args << "--library-profile" << libraryProfile;
        return args;
    }

    void save(const QString& name, const QJsonObject& value) {
        QFile f(report + "/" + name);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(value).toJson());
    }

    // 遍历原始设计节点，对照父局部坐标；根窗口尺寸在独立断言中检查。
    void compareGeometry(UiPage& scene, const QJsonObject& node,
                         const QPoint& parentLayoutOffset = {}) {
        const auto id = node["guid"].toString();
        auto* widget = scene.widget(id);
        auto* layout = scene.layout(id);
        QVERIFY(widget || layout);
        QVERIFY(!(widget && layout));
        // 子节点使用父局部坐标比较；根窗口的尺寸由调用方单独验证，避免混用坐标系。
        if (node["guid"] != scene.rootId()) {
            const auto expected =
                QRect(parentLayoutOffset.x() + qRound(node["left"].toDouble()),
                      parentLayoutOffset.y() + qRound(node["top"].toDouble()),
                      qRound(node["width"].toDouble()), qRound(node["height"].toDouble()));
            QCOMPARE(widget ? widget->geometry() : layout->geometry(), expected);
        }
        // 递归覆盖整棵设计树，确保嵌套布局内部节点也参与几何比较。
        for (const auto& child : node["childNode"].toArray())
            compareGeometry(scene, child.toObject(), layout ? layout->geometry().topLeft() : QPoint{});
    }

private slots:

    void initTestCase() {
        // 显式装载相同字体及 Token profile，控制平台字体替代带来的误差。
        QDir().mkpath(report);
        QVERIFY(QFontDatabase::addApplicationFont(data + "/HarmonyOS_Sans_SC_Regular.ttf") >= 0);
        component_theme::install(*qApp, "standard-light");
    }

    // 检查三层布局和节点几何，保存运行追踪供 HTML 路径逐项对照。
    void recursiveLayoutGeometry() {
        WorkflowWindow w(data, resultOverride);
        w.show();
        w.setCurrentIndex(1);
        QTest::qWait(150);
        // 截图前清除当前焦点，减少自动获焦造成的外观差异，几何数据仍取实际窗口。
        if (qApp->focusWidget())
            qApp->focusWidget()->clearFocus();
        baseline = w.scene(true).trace();
        save("native-result.json", baseline);
        QVERIFY(w.grab().save(report + "/native-result.png"));
        QCOMPARE(w.size(), QSize(600, 560));
        QCOMPARE(baseline["layouts"].toArray().size(), 3);
        QCOMPARE(baseline["components"].toArray().size(), 6);
        QCOMPARE(baseline["adapters"].toArray().size(), 4);
        compareGeometry(w.scene(true),
                        readJson(QString(FIXTURE_ROOT) +
                                 "/design-inputs/result-page/canonical.raw.json")["pixTreeNodes"]
                            .toArray()
                            .first()
                            .toObject());
        QCOMPARE(w.widget(1)->property("structureSource").toString(), QString("ui-xml"));
        auto* title = qobject_cast<QLabel*>(w.scene(true).widget("21:437"));
        QVERIFY(title);
        QCOMPARE(title->text(), QString("工单列表"));
        QCOMPARE(title->font().pixelSize(), 24);
    }

    // 放大窗口后核对交叉轴 Fill 与嵌套 Hug 的尺寸约束。
    void nestedFillHugResize() {
        WorkflowWindow w(data, resultOverride);
        w.show();
        w.setCurrentIndex(1);
        w.resize(760, 640);
        QTest::qWait(80);
        resized = w.scene(true).trace();
        save("native-resized.json", resized);
        QCOMPARE(w.scene(true).widget("21:457")->geometry(), QRect(36, 398, 696, 158));
        QCOMPARE(w.scene(true).layout("21:473")->geometry(), QRect(36, 576, 490, 32));
        QVERIFY(!w.scene(true).widget("21:473"));
        QCOMPARE(w.scene(true).widget("21:471")->size(), QSize(310, 32));
        QVERIFY(w.grab().save(report + "/native-resized.png"));
    }

    // 禁用项既有正确文字，也必须拒绝选择；禁用按钮不得发出 clicked。
    void textAndDisabledOptions() {
        WorkflowWindow w(data, resultOverride);
        w.show();
        w.setCurrentIndex(1);
        QTest::qWait(50);
        // 逐个检查四个禁用选项，确认文字、模型标志与鼠标行为一致，避免只检查外观。
        for (const auto& id : {"21:471", "21:469", "21:467", "21:465"}) {
            auto* view = w.scene(true).widget(id)->findChild<QListView*>();
            QVERIFY(view);
            QCOMPARE(view->model()->rowCount(), 1);
            auto index = view->model()->index(0, 0);
            QCOMPARE(index.data().toString(), QString("tags ETCH-01"));
            QVERIFY(!(index.flags() & Qt::ItemIsEnabled));
            QVERIFY(!(index.flags() & Qt::ItemIsSelectable));
            QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(30, 16));
            QVERIFY(view->selectionModel()->selectedIndexes().isEmpty());
        }
        auto* disabled = qobject_cast<Z_PushButton*>(w.scene(true).widget("21:450"));
        QVERIFY(disabled);
        QCOMPARE(disabled->text(), QString("提交"));
        QVERIFY(!disabled->isEnabled());
        QSignalSpy clicks(disabled, &QAbstractButton::clicked);
        disabled->click();
        QCOMPARE(clicks.count(), 0);
    }

    // 覆盖空值校验、鼠标确认、返回保值、回车提交和结果页演示信号。
    void completeReviewFlow() {
        WorkflowWindow w(data, resultOverride);
        w.show();
        QTest::qWait(80);
        auto* input = qobject_cast<Z_InputField*>(w.scene(false).widget("19:47"));
        QVERIFY(input);
        auto* confirm = qobject_cast<Z_PushButton*>(w.scene(false).widget("19:57"));
        QVERIFY(confirm);
        input->setValue({});
        confirm->click();
        QCOMPARE(w.currentIndex(), 0);
        input->lineEdit()->setFocus();
        QTest::keyClicks(input->lineEdit(), "WO-2026-001");
        QCOMPARE(input->value(), QString("WO-2026-001"));
        // 输入完成后清除焦点再截图，使演示截图不依赖键盘焦点的临时绘制状态。
        if (qApp->focusWidget())
            qApp->focusWidget()->clearFocus();
        QVERIFY(w.grab().save(report + "/native-review.png"));
        QTest::mouseClick(confirm, Qt::LeftButton);
        QCOMPARE(w.currentIndex(), 1);
        QCOMPARE(w.workOrder(), QString("WO-2026-001"));
        QSignalSpy actions(&w, &WorkflowWindow::demoAction);
        // 逐个触发四个已绑定演示动作，确认每个入口均能发出事件。
        for (const auto& id : {"21:448", "21:474", "21:476", "21:478"}) {
            auto* button = qobject_cast<Z_PushButton*>(w.scene(true).widget(id));
            QVERIFY(button);
            button->click();
        }
        QCOMPARE(actions.count(), 4);
        auto* back = qobject_cast<Z_PushButton*>(w.scene(true).widget("21:481"));
        QVERIFY(back);
        back->click();
        QCOMPARE(w.currentIndex(), 0);
        QCOMPARE(input->value(), QString("WO-2026-001"));
        QTest::keyClick(input->lineEdit(), Qt::Key_Return);
        QCOMPARE(w.currentIndex(), 1);
        save("flow-result.json", {{"events", w.events()},
                                  {"workOrder", w.workOrder()},
                                  {"finalPage", w.currentIndex()}});
    }

    // 仅在临时 XML 中改成未知类，确认加载器拒绝并保持正式页面原样。
    void rejectsUnregisteredUiClass() {
        QTemporaryDir temporary(QString(D2C_ROOT) + "/../output/work/v2-test-XXXXXX");
        QVERIFY(temporary.isValid());
        // 复制页面及配套参数到临时目录，只对副本注入未知类，保护正式交付文件。
        for (const auto& ext : {".ui", ".qss", ".components.json"})
            QVERIFY(QFile::copy(data + "/result" + ext, temporary.path() + "/result" + ext));
        QFile ui(temporary.path() + "/result.ui");
        QVERIFY(ui.open(QIODevice::ReadOnly));
        auto bytes = ui.readAll();
        ui.close();
        bytes.replace("commonWidget::Z_PushButton", "unknown::Button");
        QVERIFY(ui.open(QIODevice::WriteOnly | QIODevice::Truncate));
        ui.write(bytes);
        ui.close();
        UiPage page(temporary.path() + "/result",
                    std::make_shared<WidgetRegistry>("standard-light"));
        QVERIFY_EXCEPTION_THROWN(page.load(), std::runtime_error);
    }

    // 在临时副本把间距 10 改为 14，证明运行布局由 XML 驱动；末尾核对原文件哈希。
    void xmlEditControlsLayoutWithoutDsl() {
        QTemporaryDir temporary(QString(D2C_ROOT) + "/../output/work/v2-test-XXXXXX");
        QVERIFY(temporary.isValid());
        // 成套复制页面、样式和组件参数，隔离间距修改实验，避免正式页面被测试改写。
        for (const auto& ext : {".ui", ".qss", ".components.json"})
            QVERIFY(QFile::copy(data + "/result" + ext, temporary.path() + "/result" + ext));
        const auto original = digest(data + "/result.ui");
        QFile ui(temporary.path() + "/result.ui");
        QVERIFY(ui.open(QIODevice::ReadOnly));
        QDomDocument xml;
        QVERIFY(xml.setContent(&ui));
        ui.close();
        auto layouts = xml.elementsByTagName("layout");
        bool changed = false;
        // 遍历 XML 布局元素，定位受控容器，保证只改变本次实验的一个布局参数。
        for (int i = 0; i < layouts.size(); ++i) {
            auto layout = layouts.at(i).toElement();
            // 跳过其他布局，防止多个容器同时变化后无法说明几何变化的来源。
            if (layout.attribute("name") != "layout_workOrderForm")
                continue;
            // 逐个查找目标布局的属性，保留其他参数，以单变量修改验证 XML 对运行时布局的控制。
            for (auto p = layout.firstChildElement("property"); !p.isNull();
                 p = p.nextSiblingElement("property"))
                // 只将已确认的 spacing 从 10 改为 14，并标记确实命中，避免空操作也被当作有效测试。
                if (p.attribute("name") == "spacing") {
                    QCOMPARE(p.firstChildElement("number").text(), QString("10"));
                    p.firstChildElement("number").firstChild().setNodeValue("14");
                    changed = true;
                }
        }
        QVERIFY(changed);
        QVERIFY(ui.open(QIODevice::WriteOnly | QIODevice::Truncate));
        ui.write(xml.toByteArray());
        ui.close();
        UiPage page(temporary.path() + "/result",
                    std::make_shared<WidgetRegistry>("standard-light"));
        std::unique_ptr<QWidget> root(page.load());
        root->show();
        QTest::qWait(50);
        QCOMPARE(page.widget("21:457")->height(), 170);
        QCOMPARE(page.widget("21:469")->y(), 46);
        QVERIFY(!QFile::exists(temporary.path() + "/result.json"));
        QCOMPARE(digest(data + "/result.ui"), original);
    }

    // 用 QProgressBar 测试“空 XML 宿主 + 运行时更新”接口，当前尚未实现真实仪表。
    void staticInstrumentHostRuntimeMount() {
        auto registry = std::make_shared<WidgetRegistry>("standard-light");
        registry->registerType("TestGauge", [](const QJsonObject&, QWidget* p) {
            return new QProgressBar(p);
        });
        UiPage page(data + "/instrument_host", registry);
        std::unique_ptr<QWidget> root(page.load());
        root->show();
        auto* gauge = qobject_cast<QProgressBar*>(registry->create("TestGauge", {}, nullptr));
        QVERIFY(gauge);
        gauge->setRange(0, 100);
        page.mountExtension("instrumentSlot", gauge);
        gauge->setValue(73);
        QTest::qWait(30);
        QCOMPARE(gauge->value(), 73);
        QCOMPARE(gauge->parentWidget()->objectName(), QString("instrumentSlot"));
        auto extra = std::make_unique<QProgressBar>();
        QVERIFY_EXCEPTION_THROWN(page.mountExtension("instrumentSlot", extra.get()),
                                 std::runtime_error);
    }

    // 业务状态更新只作用于当前控件，磁盘上的 .ui 字节应保持一致。
    void runtimeStateUpdatesDoNotRegenerateUi() {
        const auto original = digest(data + "/result.ui");
        WorkflowWindow w(data, resultOverride);
        w.show();
        w.setCurrentIndex(1);
        auto* button = qobject_cast<Z_PushButton*>(w.scene(true).widget("21:450"));
        QVERIFY(button);
        button->setEnabled(true);
        button->setText("完成复核");
        button->setLoading(true);
        button->setLoading(false);
        QCOMPARE(button->text(), QString("完成复核"));
        QVERIFY(button->isEnabled());
        QSignalSpy clicks(button, &QAbstractButton::clicked);
        button->click();
        QCOMPARE(clicks.count(), 1);
        QCOMPARE(digest(data + "/result.ui"), original);
    }

    // 两次独立生成四类文件并逐字节比较，再与正式部署页面对照。
    void compilerIsDeterministic() {
        QTemporaryDir temporary(QString(D2C_ROOT) + "/../output/work/v2-test-XXXXXX");
        QVERIFY(temporary.isValid());
        const auto raw = QString(FIXTURE_ROOT) + "/design-inputs/result-page/canonical.raw.json";
        // 在两个独立输出前缀下运行生成器，排除复用上次文件造成的假确定性。
        for (const auto& prefix : {"first", "second"}) {
            QProcess compiler;
            compiler.start(QCoreApplication::applicationDirPath() + "/v2_ui_compiler.exe",
                           compilerArgs(raw, temporary.path() + "/" + prefix));
            QVERIFY(compiler.waitForFinished(30000));
            QVERIFY2(compiler.exitCode() == 0, compiler.readAllStandardError().constData());
        }
        QJsonObject hashes;
        // 逐类比较两次输出及正式部署文件，覆盖 UI、样式、参数和中立设计树的字节一致性。
        for (const auto& ext : {".ui", ".qss", ".components.json", ".design-tree.json"}) {
            QCOMPARE(bytes(temporary.path() + "/first" + ext),
                     bytes(temporary.path() + "/second" + ext));
            // DSL 路径核对正式页面；HTML 路径另由来源哈希与几何对照检查其适配结果。
            if (resultOverride.isEmpty())
                QCOMPARE(bytes(temporary.path() + "/first" + ext), bytes(data + "/result" + ext));
            hashes[ext] = QString::fromLatin1(digest(temporary.path() + "/first" + ext));
        }
        save("static-generation.json", {{"status", "PASS"},
                                        {"repeatRuns", 2},
                                        {"files", hashes},
                                        {"sourceSha256", QString::fromLatin1(digest(raw))}});
    }

    // 两个点位视图对照：token-sdk 页面用 Z Design System 类，qt-native 页面用公开 Qt 内置类，
    // 且 qt-native 的初始化参数已序列化进 XML、无需运行时工厂即可还原。
    void pointUiLibrarySwitchLoadsBothMappings() {
        QTemporaryDir temporary(QString(D2C_ROOT) + "/../output/work/v2-test-XXXXXX");
        QVERIFY(temporary.isValid());
        const auto raw = QString(FIXTURE_ROOT) + "/design-inputs/review-page/canonical.raw.json";
        QProcess tokenCompiler;
        tokenCompiler.start(QCoreApplication::applicationDirPath() + "/v2_ui_compiler.exe",
                            compilerArgs(raw, temporary.path() + "/review-token"));
        QVERIFY(tokenCompiler.waitForFinished(30000));
        QVERIFY2(tokenCompiler.exitCode() == 0, tokenCompiler.readAllStandardError().constData());
        QProcess qtCompiler;
        qtCompiler.start(QCoreApplication::applicationDirPath() + "/v2_ui_compiler.exe",
                         compilerArgs(raw, temporary.path() + "/review-qt", "qt-native"));
        QVERIFY(qtCompiler.waitForFinished(30000));
        QVERIFY2(qtCompiler.exitCode() == 0, qtCompiler.readAllStandardError().constData());
        const auto tokenProfile =
            readJson(temporary.path() + "/review-token.components.json")["profile"].toString();
        auto tokenRegistry = std::make_shared<WidgetRegistry>(tokenProfile);
        v2::UiPage tokenPage(temporary.path() + "/review-token", tokenRegistry);
        std::unique_ptr<QWidget> tokenWindow(tokenPage.load());
        QVERIFY(tokenWindow->findChildren<Z_PushButton*>().size() >= 1);
        QVERIFY(tokenWindow->findChild<Z_InputField*>());
        QVERIFY(tokenWindow->findChild<Z_CheckBox*>());
        const auto qtProfile =
            readJson(temporary.path() + "/review-qt.components.json")["profile"].toString();
        auto qtRegistry = std::make_shared<WidgetRegistry>(qtProfile);
        v2::UiPage qtPage(temporary.path() + "/review-qt", qtRegistry);
        std::unique_ptr<QWidget> qtWindow(qtPage.load());
        QVERIFY(qtWindow->findChildren<QPushButton*>().size() >= 1);
        QVERIFY(qtWindow->findChild<QLineEdit*>());
        QVERIFY(qtWindow->findChild<QCheckBox*>());
        // qt-native 页面不允许出现任何 SDK 命名空间组件。
        QVERIFY(qtWindow->findChildren<Z_PushButton*>().isEmpty());
        // 公开 Qt 组件的初始文本必须已由 XML 属性还原（无运行时工厂）。
        bool sawText = false;
        for (auto* button : qtWindow->findChildren<QPushButton*>())
            if (!button->text().isEmpty())
                sawText = true;
        QVERIFY(sawText);
    }

    // 真实 27:450 页面使用设计侧 light-compact；生成必须显式解析为已发布的 wide-light，
    // 并把设计名与运行时名同时写入清单，证明没有把别名伪装成 SDK Token profile。
    void designProfileAliasResolvesToPublishedRuntimeContext() {
        QTemporaryDir temporary(QString(D2C_ROOT) + "/../output/work/v2-test-XXXXXX");
        QVERIFY(temporary.isValid());
        const auto raw = QString(FIXTURE_ROOT) +
                         "/design-inputs/horizontal-27-450-full/canonical.raw.json";
        QProcess compiler;
        compiler.start(QCoreApplication::applicationDirPath() + "/v2_ui_compiler.exe",
                       compilerArgs(raw, temporary.path() + "/horizontal-27-450"));
        QVERIFY(compiler.waitForFinished(30000));
        QVERIFY2(compiler.exitCode() == 0, compiler.readAllStandardError().constData());
        const auto manifest = readJson(temporary.path() + "/horizontal-27-450.components.json");
        QCOMPARE(manifest["profile"].toString(), QStringLiteral("wide-light"));
        QCOMPARE(manifest["designProfile"].toString(), QStringLiteral("light-compact"));
        QCOMPARE(manifest["profileAlias"].toString(), QStringLiteral("light-compact"));
        QVERIFY(QFileInfo::exists(temporary.path() + "/horizontal-27-450.ui"));
        QVERIFY(QFileInfo::exists(temporary.path() + "/horizontal-27-450.qss"));
    }

    void parentPrimarySizingHasExplicitOutcomes() {
        QTemporaryDir temporary(QString(D2C_ROOT) + "/../output/work/v2-test-XXXXXX");
        QVERIFY(temporary.isValid());
        const auto source = readJson(QString(FIXTURE_ROOT) +
                                     "/design-inputs/result-page/canonical.raw.json");
        for (const auto& mode : {"RESIZE_TO_FIT", "FILL_PARENT", "UNKNOWN"}) {
            auto changed = source;
            auto roots = changed["pixTreeNodes"].toArray();
            auto root = roots[0].toObject();
            auto rootLayout = root["autoLayout"].toObject();
            rootLayout["autoLayoutPrimaryAlign"] = "flex-start";
            root["autoLayout"] = rootLayout;
            auto children = root["childNode"].toArray();
            for (int i = 0; i < children.size(); ++i) {
                auto child = children[i].toObject();
                if (child["guid"] != "21:473")
                    continue;
                auto sizing = child["autoLayout"].toObject();
                sizing["stackChildPrimarySizing"] = mode;
                child["autoLayout"] = sizing;
                children[i] = child;
            }
            root["childNode"] = children;
            roots[0] = root;
            changed["pixTreeNodes"] = roots;
            const QString prefix = temporary.path() + "/" + mode;
            QFile input(prefix + ".json");
            QVERIFY(input.open(QIODevice::WriteOnly));
            input.write(QJsonDocument(changed).toJson());
            input.close();
            QProcess compiler;
            compiler.start(QCoreApplication::applicationDirPath() + "/v2_ui_compiler.exe",
                           compilerArgs(input.fileName(), prefix));
            QVERIFY(compiler.waitForFinished(30000));
            if (QString(mode) == "UNKNOWN") {
                QVERIFY(compiler.exitCode() != 0);
                QVERIFY(compiler.readAllStandardError().contains("UNMAPPED_PARENT_PRIMARY_SIZE"));
                continue;
            }
            QVERIFY2(compiler.exitCode() == 0, compiler.readAllStandardError().constData());
            QFile ui(prefix + ".ui");
            QVERIFY(ui.open(QIODevice::ReadOnly));
            QDomDocument xml;
            QVERIFY(xml.setContent(&ui));
            const auto widgets = xml.elementsByTagName("widget");
            bool matched = false;
            for (int i = 0; i < widgets.size(); ++i) {
                const auto widget = widgets.at(i).toElement();
                if (widget.attribute("name") != "resultActions")
                    continue;
                matched = true;
                for (auto property = widget.firstChildElement("property"); !property.isNull();
                     property = property.nextSiblingElement("property"))
                    if (property.attribute("name") == "sizePolicy")
                        QCOMPARE(property.firstChildElement("sizepolicy").attribute("vsizetype"),
                                 QString(mode) == "FILL_PARENT" ? QString("Expanding")
                                                                  : QString("Maximum"));
            }
            QVERIFY(matched);
            QVERIFY(readJson(prefix + ".components.json")["layoutNodeIds"].toArray().isEmpty());
        }
    }

    // 设计树须有原始父子关系和 Token 引用，通过节点 ID、来源与设计树哈希关联清单。
    void neutralTreeMatchesSourceAndXml() {
        const auto prefix = resultOverride.isEmpty() ? data + "/result" : resultOverride;
        const auto tree = readJson(prefix + ".design-tree.json");
        const auto manifest = readJson(prefix + ".components.json");
        QCOMPARE(tree["sourceSha256"], manifest["sourceSha256"]);
        QCOMPARE(QString::fromLatin1(digest(prefix + ".design-tree.json")),
                 manifest["designTreeSha256"].toString());
        QCOMPARE(tree["nodes"].toArray().size(), 14);
        QMap<QString, QJsonObject> nodes;
        // 逐节点检查唯一 ID、必要字段和 Qt 对象绑定，确认中立树完整且可追溯。
        for (const auto& value : tree["nodes"].toArray()) {
            const auto node = value.toObject();
            const auto id = node["nodeId"].toString();
            QVERIFY(!nodes.contains(id));
            nodes[id] = node;
            // 逐项检查平台中立字段是否保留，防止转换时遗漏布局、身份或 Token 引用。
            for (const auto& key : {"name", "type", "parentId", "children", "componentRef",
                                    "bounds", "layout", "tokenBindings"})
                QVERIFY(node.contains(key));
            QVERIFY(!node.contains("qtClass"));
            QVERIFY(!node.contains("header"));
            QVERIFY(!manifest["nodes"].toObject()[id].toString().isEmpty());
            QVERIFY(!manifest["nodes"].toObject()[id].toString().startsWith("node_"));
        }
        QCOMPARE(manifest["nodes"].toObject()["21:457"].toString(),
                 QString("workOrderForm"));
        QCOMPARE(manifest["nodes"].toObject()["21:473"].toString(),
                 QString("layout_resultActions"));
        QCOMPARE(manifest["layoutNodeIds"].toArray(), QJsonArray{QString("21:473")});
        QVERIFY(nodes["21:436"]["parentId"].isNull());
        QCOMPARE(nodes["21:450"]["parentId"].toString(), QString("21:473"));
        QVERIFY(!nodes["21:450"]["tokenBindings"].toObject().isEmpty());
        // 遍历全部父节点，检查每条父子关系的双向引用是否一致。
        for (auto it = nodes.begin(); it != nodes.end(); ++it)
            // 逐个核对孩子声明的 parentId，发现孤立引用或错误挂接。
            for (const auto& child : it.value()["children"].toArray())
                QCOMPARE(nodes[child.toString()]["parentId"].toString(), it.key());
        const auto original =
            readJson(QString(FIXTURE_ROOT) + "/design-inputs/result-page/canonical.raw.json");
        std::function<void(QJsonObject)> compare = [&](QJsonObject rawNode) {
            const auto n = nodes[rawNode["guid"].toString()];
            QCOMPARE(n["layout"], rawNode["autoLayout"]);
            QCOMPARE(n["bounds"].toObject()["width"], rawNode["width"]);
            // 递归把原始树与中立节点逐项对应，确保嵌套节点的布局和宽度同样得到核对。
            for (const auto& child : rawNode["childNode"].toArray())
                compare(child.toObject());
        };
        // 原生 DSL 路径直接比较原布局字段；HTML 路径的布局由浏览器计算，另由双路几何检查验证。
        if (resultOverride.isEmpty())
            compare(original["pixTreeNodes"].toArray().first().toObject());
    }

    // 临时将选项状态改为未知值，生成器必须失败且不能留下可加载的 invalid.ui。
    void compilerRejectsUnsupportedVariant() {
        QTemporaryDir temporary(QString(D2C_ROOT) + "/../output/work/v2-test-XXXXXX");
        QVERIFY(temporary.isValid());
        auto raw =
            readJson(QString(FIXTURE_ROOT) + "/design-inputs/result-page/canonical.raw.json");
        bool changed = false;
        std::function<QJsonObject(QJsonObject)> change = [&](QJsonObject node) {
            // 只向指定选项注入非法状态，让失败能够明确归因于不支持的变体值。
            if (node["guid"] == "21:471") {
                auto map = node["propRefMap"].toObject();
                auto variants = map["variant"].toArray();
                // 遍历变体属性寻找 state，保留其他属性，使负向测试只有一个故障来源。
                for (int i = 0; i < variants.size(); ++i) {
                    auto v = variants[i].toObject();
                    // 将状态值替换为未知枚举并记录命中，验证编译器会显式拒绝而非忽略该值。
                    if (v["name"] == "state") {
                        v["value"] = "unsupported";
                        variants[i] = v;
                        changed = true;
                    }
                }
                map["variant"] = variants;
                node["propRefMap"] = map;
            }
            auto children = node["childNode"].toArray();
            // 递归复制并修改子树，确保深层目标也可注入错误，且修改只留在内存与临时文件。
            for (int i = 0; i < children.size(); ++i)
                children[i] = change(children[i].toObject());
            node["childNode"] = children;
            return node;
        };
        auto roots = raw["pixTreeNodes"].toArray();
        roots[0] = change(roots[0].toObject());
        raw["pixTreeNodes"] = roots;
        QVERIFY(changed);
        QFile input(temporary.path() + "/invalid.json");
        QVERIFY(input.open(QIODevice::WriteOnly));
        input.write(QJsonDocument(raw).toJson());
        input.close();
        QProcess compiler;
        compiler.start(QCoreApplication::applicationDirPath() + "/v2_ui_compiler.exe",
                       compilerArgs(input.fileName(), temporary.path() + "/invalid"));
        QVERIFY(compiler.waitForFinished(30000));
        QVERIFY(compiler.exitCode() != 0);
        QVERIFY(compiler.readAllStandardError().contains("UNSUPPORTED_OPTION_VARIANT"));
        QVERIFY(!QFile::exists(temporary.path() + "/invalid.ui"));
    }

    // 多次创建/销毁用于发现基本生命周期问题，不等同于完整的泄漏检测。
    void repeatConstructionAndDestruction() {
        // 重复创建、显示与析构五次，检查基本所有权和生命周期行为是否稳定。
        for (int i = 0; i < 5; ++i) {
            WorkflowWindow w(data, resultOverride);
            w.show();
            w.setCurrentIndex(1);
            QTest::qWait(10);
        }
    }
};
QTEST_MAIN(V2Tests)
#include "V2Tests.moc"
