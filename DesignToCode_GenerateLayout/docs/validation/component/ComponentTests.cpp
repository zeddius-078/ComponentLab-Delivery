// 组件契约的原生测试：保存的 Pixso 实例与受控夹具分别验证，结果写到 component 目录。
// 所有修改仅发生在内存副本，原始设计输入保持原样。
#include "ComponentConverter.h"
#include "LayoutConverter.h"
#include "ComponentTheme.h"
#include "NavigationAppearance.h"
#include "DockWidgetAppearance.h"
#include "MdiAppearance.h"
#include "PageFlowAppearance.h"
#include "TabAppearance.h"
#include "ToolBoxAppearance.h"
#include "VisualAssetCatalog.h"
#include "Z_CheckBox.h"
#include "Z_DesignComponents.h"
#include "Z_FoundationComponents.h"
#include "Z_InputField.h"
#include "native/Z_Menu.h"
#include "native/Z_MenuBar.h"
#include "native/Z_ToolBar.h"
#include "native/Z_TabWidget.h"
#include "native/Z_ToolBox.h"
#include "native/Z_DockWidget.h"
#include "native/Z_MdiArea.h"
#include "native/Z_MdiSubWindow.h"
#include "native/Z_StackedWidget.h"
#include "native/Z_Splitter.h"
#include "native/Z_ScrollArea.h"
#include "native/Z_CalendarWidget.h"
#include "native/Z_Wizard.h"
#include "native/Z_WizardPage.h"
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QMenuBar>
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QCommandLinkButton>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QScrollBar>
#include <QSplitter>
#include <QCalendarWidget>
#include <QStackedWidget>
#include <QTimeEdit>
#include <QWizard>
#include <QWizardPage>
#include <QSignalSpy>
#include <QTest>
#include <QToolButton>
#include <QTabBar>

// 测试根路径由 CMake 注入；读取失败应立即终止，避免空数据让断言失去意义。
QJsonObject read(const QString& relative) {
    QFile file(QString(D2C_ROOT) + "/" + relative);
    // 读取失败立即终止，避免空夹具导致后续测试走到与真实输入无关的分支。
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Missing test input");
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    // 输入必须先通过 JSON 语法检查，保证测试失败能归因于映射行为而非损坏文件。
    if (error.error != QJsonParseError::NoError)
        throw std::runtime_error("Invalid JSON");
    return doc.object();
}

class ComponentTests : public QObject {
    Q_OBJECT
private slots:

    // 工单样本验证输入可编辑、横向填满、勾选切换及按钮启用/禁用的真实点击行为。
    void freshPageContract() {
        const auto raw = read("../docs/fixtures/layout/design-inputs/holdout-19-45-full/canonical.raw.json");
        const auto root = raw["pixTreeNodes"].toArray()[0].toObject();
        d2c::ComponentConverter components(raw, read("../docs/fixtures/layout/mapping/component-mapping.json"));
        d2c::LayoutConverter layouts(read("../docs/fixtures/layout/mapping/layout-mapping.json"), raw);
        QWidget owner;
        owner.resize(root["width"].toInt(), root["height"].toInt());
        auto* layout = layouts.convert(root, &owner,
                                       [&](const QJsonObject& node, QWidget* parent) -> QWidget* {
                                           // 布局回调按节点类型创建组件或文本标签，用实际工单页面验证二者混排。
                                           if (node["type"] == "INSTANCE")
                                               return components.convert(node, parent);
                                           return new QLabel(node["nodeText"].toString(), parent);
                                       });
        owner.show();
        QApplication::processEvents();
        auto* input = owner.findChild<commonWidget::Z_InputField*>();
        QVERIFY(input);
        QVERIFY(!input->lineEdit()->isReadOnly());
        QVERIFY(input->value().isEmpty());
        QTest::keyClicks(input->lineEdit(), "WO-6207");
        QCOMPARE(input->value(), QString("WO-6207"));
        const auto margins = layout->contentsMargins();
        QCOMPARE(input->width(), owner.width() - margins.left() - margins.right());
        owner.resize(owner.width() + 160, owner.height());
        QApplication::processEvents();
        QCOMPARE(input->width(), owner.width() - margins.left() - margins.right());
        auto* check = owner.findChild<commonWidget::Z_CheckBox*>();
        QVERIFY(check);
        QVERIFY(check->isChecked());
        QTest::mouseClick(check, Qt::LeftButton);
        QVERIFY(!check->isChecked());
        int enabled = 0, disabled = 0;
        // 遍历页面直接按钮并记录点击信号，分别验证可用与禁用实例的真实行为。
        for (auto* button : owner.findChildren<commonWidget::Z_PushButton*>(
                 QString(), Qt::FindDirectChildrenOnly)) {
            QSignalSpy clicks(button, &QPushButton::clicked);
            QTest::mouseClick(button, Qt::LeftButton);
            // 可用按钮应发出一次点击，禁用按钮应保持零次；同时计数，确保两类都实际测到。
            if (button->isEnabled()) {
                ++enabled;
                QCOMPARE(clicks.count(), 1);
            } else {
                ++disabled;
                QCOMPARE(clicks.count(), 0);
            }
        }
        QCOMPARE(enabled, 1);
        QCOMPARE(disabled, 1);
    }

    // 横纵样本分别核对只读输入、勾选值及 loading 往返后保持的按钮禁用状态。
    void createdPageStates() {
        // 用横向和纵向两个真实样本重复检查状态映射，排除仅在一种页面方向下正确的情况。
        for (const auto& axis : {QString("vertical"), QString("horizontal")}) {
            const auto raw = read("../docs/fixtures/layout/design-inputs/acceptance-" + axis + "-full/canonical.raw.json");
            d2c::ComponentConverter engine(raw, read("../docs/fixtures/layout/mapping/component-mapping.json"));
            int inputs = 0, checks = 0, enabledButtons = 0, disabledButtons = 0;
            // 逐个实例独立创建控件，核对状态值以及操作后是否保持相应约束。
            for (const auto& value :
                 raw["pixTreeNodes"].toArray()[0].toObject()["childNode"].toArray()) {
                const auto node = value.toObject();
                // 跳过布局或文本节点，本测试只覆盖具有组件映射契约的实例。
                if (node["type"] != "INSTANCE")
                    continue;
                std::unique_ptr<QWidget> widget;
                try {
                    widget.reset(engine.convert(node, nullptr));
                } catch (const std::exception& error) {
                    QFAIL(error.what());
                }
                widget->show();
                QApplication::processEvents();
                // 按原生组件类型执行对应检查：输入只读、复选框切换、按钮 loading 后恢复原禁用值。
                if (auto* input = qobject_cast<commonWidget::Z_InputField*>(widget.get())) {
                    ++inputs;
                    QVERIFY(input->lineEdit()->isReadOnly());
                    const auto original = input->value();
                    QVERIFY(original.startsWith("BATCH-"));
                    QTest::keyClicks(input->lineEdit(), "modified");
                    QCOMPARE(input->value(), original);
                } else if (auto* check = qobject_cast<commonWidget::Z_CheckBox*>(widget.get())) {
                    ++checks;
                    QVERIFY(check->isChecked());
                    check->click();
                    QVERIFY(!check->isChecked());
                } else if (auto* button = qobject_cast<commonWidget::Z_PushButton*>(widget.get())) {
                    const bool enabled = button->isEnabled();
                    button->setLoading(true);
                    button->setLoading(false);
                    QApplication::processEvents();
                    QCOMPARE(button->isEnabled(), enabled);
                    QSignalSpy clicks(button, &QPushButton::clicked);
                    button->click();
                    // 依据恢复后的可用状态检查点击信号，发现 loading 往返误启用禁用按钮的问题。
                    if (button->isEnabled()) {
                        ++enabledButtons;
                        QCOMPARE(clicks.count(), 1);
                    } else {
                        ++disabledButtons;
                        QCOMPARE(clicks.count(), 0);
                    }
                } else
                    QFAIL("Unexpected native class");
            }
            QCOMPARE(inputs, 1);
            QCOMPARE(checks, 1);
            QCOMPARE(enabledButtons, 1);
            QCOMPARE(disabledButtons, 1);
        }
    }

    // 构造非法引用/属性/规则，检查异常包含能追溯到设计节点的上下文。
    void failureContext() {
        const auto raw = read("../docs/fixtures/layout/design-inputs/node-12-382-full/canonical.raw.json");
        const auto rules = read("../docs/fixtures/layout/mapping/component-mapping.json");
        auto node =
            raw["pixTreeNodes"].toArray()[0].toObject()["childNode"].toArray()[0].toObject();
        const auto verifyFailure = [&](QJsonObject sample, QJsonObject mapping, const QString& code,
                                       const QString& component) {
            d2c::ComponentConverter engine(raw, mapping);
            try {
                std::unique_ptr<QWidget> unexpected(engine.convert(sample, nullptr));
            } catch (const std::runtime_error& error) {
                const auto detail = QJsonDocument::fromJson(error.what()).object();
                QCOMPARE(detail["code"].toString(), code);
                QCOMPARE(detail["componentName"].toString(), component);
                QCOMPARE(detail["nodeId"], sample["guid"]);
                QCOMPARE(detail["nodeName"], sample["name"]);
                QCOMPARE(detail["availableProperties"], sample["propRefMap"]);
                QVERIFY(detail.contains("detail"));
                return;
            }
            QFAIL("Expected structured conversion failure");
        };
        auto unknown = node;
        unknown["mainStateGroup"] = "missing";
        verifyFailure(unknown, rules, "UNMAPPED_COMPONENT", "");
        auto props = node["propRefMap"].toObject();
        auto variants = props["variant"].toArray();
        auto bad = variants[0].toObject();
        bad["value"] = "not-in-domain";
        variants[0] = bad;
        props["variant"] = variants;
        auto invalid = node;
        invalid["propRefMap"] = props;
        verifyFailure(invalid, rules, "UNMAPPED_PROPERTY", "ComponentLab/Button");
        props = node["propRefMap"].toObject();
        variants = props["variant"].toArray();
        variants.append(variants[0]);
        props["variant"] = variants;
        invalid["propRefMap"] = props;
        verifyFailure(invalid, rules, "DUPLICATE_PROPERTY", "ComponentLab/Button");
        auto duplicated = rules;
        auto mappings = rules["rules"].toArray();
        mappings.append(mappings[0]);
        duplicated["rules"] = mappings;
        verifyFailure(node, duplicated, "AMBIGUOUS_COMPONENT_MAPPING", "ComponentLab/Button");
        invalid = node;
        invalid["type"] = "FRAME";
        verifyFailure(invalid, rules, "INVALID_NODE_TYPE", "ComponentLab/Button");
        // profileAliases 合同：设计侧 light-compact 必须明确解析到 wide-light，
        // 不能在 standard-light 上仅靠字符串白名单放行。
        auto profileNode = node;
        auto profileProps = profileNode["propRefMap"].toObject();
        auto profileVariants = profileProps["variant"].toArray();
        bool profileEntryFound = false;
        for (int i = 0; i < profileVariants.size(); ++i) {
            auto entry = profileVariants[i].toObject();
            if (entry["name"].toString() == "Profile") {
                entry["value"] = "light-compact";
                profileVariants[i] = entry;
                profileEntryFound = true;
                break;
            }
        }
        QVERIFY(profileEntryFound);
        profileProps["variant"] = profileVariants;
        profileNode["propRefMap"] = profileProps;
        {
            auto aliasRules = rules;
            aliasRules["profile"] = "wide-light";
            d2c::ComponentConverter engine(raw, aliasRules);
            std::unique_ptr<QWidget> accepted(engine.convert(profileNode, nullptr));
            QVERIFY(accepted);
        }
        for (int i = 0; i < profileVariants.size(); ++i) {
            auto entry = profileVariants[i].toObject();
            if (entry["name"].toString() == "Profile") {
                entry["value"] = "dark-compact";
                profileVariants[i] = entry;
                break;
            }
        }
        profileProps["variant"] = profileVariants;
        profileNode["propRefMap"] = profileProps;
        verifyFailure(profileNode, rules, "PROFILE_MISMATCH", "ComponentLab/Button");
    }

    // 从真实母版定义组成测试实例，分别覆盖前图标、后图标、双图标与纯图标模式。
    void resourceMappingFromRealDefinitions() {
        const auto raw = read("../docs/fixtures/layout/design-inputs/node-12-382-full/canonical.raw.json");
        const auto mapping = read("../docs/fixtures/layout/mapping/component-mapping.json");
        d2c::ComponentConverter engine(raw, mapping);
        QJsonObject family;
        // 从保存的组件集中查找 Button，保证图标测试使用真实母版定义。
        for (const auto& s : raw["pixComponentTreeDslNodes"].toArray())
            // 仅选中目标 Button 组件集，避免拿到其他组件的属性结构。
            if (s.toObject()["name"] == "ComponentLab/Button")
                family = s.toObject();
        const QStringList modes{"Leading", "Trailing", "Dual", "IconOnly"};
        // 分别验证前、后、双侧和独立图标，覆盖资源路径与内容模式的组合。
        for (const auto& mode : modes) {
            QJsonObject definition;
            // 逐个查找满足当前内容模式的母版，保证夹具继承的变体具有真实设计依据。
            for (const auto& v : family["childNode"].toArray()) {
                const auto node = v.toObject();
                QMap<QString, QString> properties;
                // 把母版变体整理为按名查值的表，便于同时限定外观、状态和 Profile。
                for (const auto& p : node["propRefMap"].toObject()["variant"].toArray())
                    properties[p.toObject()["name"].toString()] = p.toObject()["value"].toString();
                // 采集母版保留设计侧命名；仅按已登记别名判断其 SDK 上下文。
                const auto designProfile = properties.value("Profile");
                const auto runtimeProfile = designProfile == mapping["profile"].toString()
                    ? designProfile
                    : mapping["profileAliases"].toObject()[designProfile].toObject()["runtimeProfile"].toString();
                // 只选取默认、纯色、Bare、亮色舒适档的对应母版，让本轮只比较图标模式差异。
                if (properties.value("content") == mode && properties.value("state") == "default" &&
                    properties.value("appearance") == "solid" &&
                    properties.value("iconStyle") == "Bare" &&
                    !runtimeProfile.isEmpty() && runtimeProfile == mapping["profile"].toString()) {
                    definition = node;
                    break;
                }
            }
            QVERIFY2(!definition.isEmpty(), qPrintable(mode));
            // 以下为测试夹具，借用已保存母版；fixture ID 与正式 Pixso 实例来源分开。
            auto instance = definition;
            instance["mainStateGroup"] = family["guid"];
            instance["mainComponent"] = definition["guid"];
            instance["type"] = "INSTANCE";
            instance["guid"] = "fixture:" + mode;
            instance["name"] = "Resource contract fixture";
            std::unique_ptr<QWidget> widget(engine.convert(instance, nullptr));
            auto* button = qobject_cast<commonWidget::Z_PushButton*>(widget.get());
            QVERIFY(button);
            QCOMPARE(button->isIconOnly(), mode == "IconOnly");
            // 前侧或双侧模式必须解析到左箭头资源，验证前插槽映射。
            if (mode == "Leading" || mode == "Dual")
                QCOMPARE(button->leadingIconSource(), QString("icon.action.previous"));
            // 后侧或双侧模式必须解析到右箭头资源，验证后插槽映射。
            if (mode == "Trailing" || mode == "Dual")
                QCOMPARE(button->trailingIconSource(), QString("icon.action.next"));
            // 独立图标模式同时核对资源与可访问名称，避免去掉文本后丢失操作含义。
            if (mode == "IconOnly") {
                QCOMPARE(button->leadingIconSource(), QString("icon.action.search"));
                QCOMPARE(button->accessibleName(), QString::fromUtf8("搜索"));
            }
            button->show();
            QApplication::processEvents();
            QSignalSpy spy(button, &QPushButton::clicked);
            button->click();
            QCOMPARE(spy.count(), 1);
        }
        QFile output(QString(D2C_ROOT) + "/../output/logs/component/resource-calls.json");
        QVERIFY(output.open(QIODevice::WriteOnly));
        output.write(QJsonDocument(engine.trace()).toJson());
    }

    // ImageAsset 与 ModelAsset 仅声明稳定语义 ID；具体文件、Profile Token
    // 替换和高 DPI 渲染由 GenerateComponent SDK 统一负责。
    void publicVisualAssetMappingsUseSdkCatalog() {
        const auto mapping = read("../docs/fixtures/layout/mapping/component-mapping.json");
        const auto entries = mapping["publicAssetMappings"].toObject()["entries"].toObject();
        QCOMPARE(entries.size(), 2);
        const QMap<QString, commonWidget::VisualAssetKind> expected{
            {QStringLiteral("image.foundation.geometric-preview"),
             commonWidget::VisualAssetKind::Image},
            {QStringLiteral("model.foundation.schematic"),
             commonWidget::VisualAssetKind::Model2D},
        };
        for (auto it = expected.cbegin(); it != expected.cend(); ++it) {
            const auto declaration = entries[it.key()].toObject();
            QCOMPARE(declaration["ownership"].toString(), QStringLiteral("shared"));
            QCOMPARE(declaration["assetId"].toString(), it.key());
            QVERIFY(!declaration.contains("source"));
            QVERIFY(commonWidget::VisualAssetCatalog::contains(it.key()));
            QVERIFY(commonWidget::VisualAssetCatalog::descriptor(it.key()).kind == it.value());
            QVERIFY(QFile::exists(commonWidget::VisualAssetCatalog::resolve(it.key())));
            QCOMPARE(commonWidget::VisualAssetCatalog::sha256(it.key()).size(), 64);
        }
        const auto image = commonWidget::VisualAssetCatalog::render(
            QStringLiteral("image.foundation.geometric-preview"), QStringLiteral("Loaded"),
            QSize(280, 145), QStringLiteral("Cover"), QStringLiteral("standard-light"));
        const auto model = commonWidget::VisualAssetCatalog::render(
            QStringLiteral("model.foundation.schematic"), QStringLiteral("Error"),
            QSize(310, 156), QStringLiteral("Contain"), QStringLiteral("standard-dark"));
        QVERIFY(!image.isNull());
        QVERIFY(!model.isNull());
        QCOMPARE(image.size(), QSize(280, 145));
        QCOMPARE(model.size(), QSize(310, 156));
        QVERIFY_EXCEPTION_THROWN(
            commonWidget::VisualAssetCatalog::resolve(QStringLiteral("model.unknown")),
            std::invalid_argument);
    }

    // Login 页的四个真实实例：核对文字覆盖、placeholder 和可调用的组件行为。
    void realInstances() {
        const auto raw = read("../docs/fixtures/layout/design-inputs/node-12-382-full/canonical.raw.json");
        d2c::ComponentConverter engine(raw, read("../docs/fixtures/layout/mapping/component-mapping.json"));
        QWidget owner;
        int count = 0;
        // 逐个转换真实页面实例，用组件 API 和实际信号核对映射后的可用性。
        for (const auto& v : raw["pixTreeNodes"].toArray()[0].toObject()["childNode"].toArray()) {
            const auto n = v.toObject();
            // 普通节点没有组件变体，在此跳过，使实例数与组件行为检查范围一致。
            if (n["type"] != "INSTANCE")
                continue;
            auto* w = engine.convert(n, &owner);
            ++count;
            QVERIFY(w->property("designComponent").toString().startsWith("ComponentLab/"));
            // 按钮分支检查实例文字覆盖与点击信号，确认设计内容和交互同时落地。
            if (auto* button = qobject_cast<commonWidget::Z_PushButton*>(w)) {
                QCOMPARE(button->text(), QString::fromUtf8("登录"));
                QSignalSpy spy(button, &QPushButton::clicked);
                button->click();
                QCOMPARE(spy.count(), 1);
            }
            // 复选框分支检查文字、初始值及切换后的值，覆盖静态属性与运行状态。
            if (auto* box = qobject_cast<commonWidget::Z_CheckBox*>(w)) {
                QCOMPARE(box->text(), QString::fromUtf8("保留本次账号"));
                QVERIFY(!box->isChecked());
                box->click();
                QVERIFY(box->isChecked());
            }
            // 输入分支检查内部编辑器、占位文字和赋值接口，确认映射输出可接入业务逻辑。
            if (auto* input = qobject_cast<commonWidget::Z_InputField*>(w)) {
                QVERIFY(input->lineEdit());
                QVERIFY(
                    input->lineEdit()->placeholderText().startsWith(QString::fromUtf8("请输入")));
                input->setValue("test");
                QCOMPARE(input->value(), QString("test"));
            }
        }
        QCOMPARE(count, 4);
        QFile output(QString(D2C_ROOT) + "/../output/logs/component/runtime-calls.json");
        QVERIFY(output.open(QIODevice::WriteOnly));
        output.write(QJsonDocument(engine.trace()).toJson());
    }

    // 当前 Figma 新组件集以 component-set ID 为身份，且不再依赖旧 Profile 变体轴。
    // 本测试证明当前增量设计合同均经 GenerateComponent SDK Factory 创建并应用公开属性。
    void currentFigmaSemanticComponentsUseSdkFactory() {
        try {
        const auto mapping = read("../docs/fixtures/layout/mapping/component-mapping.json");
        const auto fixture = [](const QString& setId, const QString& name,
                                const QList<QPair<QString, QString>>& properties) {
            QJsonArray variants;
            for (const auto& property : properties)
                variants.append(QJsonObject{{"name", property.first},
                                            {"value", property.second},
                                            {"type", "variant"}});
            const QString masterId = setId + ":master";
            const QString componentKey = setId + ":component-key";
            const QJsonArray textNodes{
                QJsonObject{{"guid", masterId + ":label"},
                            {"name", "Label"},
                            {"nodeText", "Mapped label"},
                            {"childNode", QJsonArray{}}},
                QJsonObject{{"guid", masterId + ":content"},
                            {"name", "Content"},
                            {"nodeText", "Mapped content"},
                            {"childNode", QJsonArray{}}},
                QJsonObject{{"guid", masterId + ":profile-name"},
                            {"name", "Profile/Name"},
                            {"nodeText", "Mapped profile"},
                            {"childNode", QJsonArray{}}},
                QJsonObject{{"guid", masterId + ":label-content"},
                            {"name", "Content / text"},
                            {"nodeText", "Mapped label text"},
                            {"childNode", QJsonArray{}}},
                QJsonObject{{"guid", masterId + ":progress-label"},
                            {"name", "Indicator / label"},
                            {"nodeText", "68%"},
                            {"childNode", QJsonArray{}}},
                QJsonObject{{"guid", masterId + ":title"},
                            {"name", "Title"},
                            {"nodeText", "Mapped metric title"},
                            {"childNode", QJsonArray{}}},
                QJsonObject{{"guid", masterId + ":description"},
                            {"name", "Description"},
                            {"nodeText", "Mapped consequence"},
                            {"childNode", QJsonArray{}}},
                QJsonObject{{"guid", masterId + ":metric-value"},
                            {"name", "Content/Value"},
                            {"nodeText", "98.6%"},
                            {"childNode", QJsonArray{}}},
                QJsonObject{{"guid", masterId + ":metric-detail"},
                            {"name", "Content/Detail"},
                            {"nodeText", "+2.4% mapped"},
                            {"childNode", QJsonArray{}}},
                QJsonObject{{"guid", masterId + ":prefix"},
                            {"name", "Prefix"},
                            {"nodeText", "$"},
                            {"childNode", QJsonArray{}}},
                QJsonObject{{"guid", masterId + ":value"},
                            {"name", "Value"},
                            {"nodeText", "93,241.5"},
                            {"childNode", QJsonArray{}}},
                QJsonObject{{"guid", masterId + ":suffix"},
                            {"name", "Suffix"},
                            {"nodeText", "USD"},
                            {"childNode", QJsonArray{}}},
            };
            QJsonObject master{{"guid", masterId},
                               {"name", name + " master"},
                               {"componentKey", componentKey},
                               {"propRefMap", QJsonObject{{"variant", variants}}},
                               {"childNode", textNodes}};
            QJsonObject family{{"guid", setId},
                               {"name", name},
                               {"componentKey", setId + ":family-key"},
                               {"childNode", QJsonArray{master}}};
            QJsonObject instance = master;
            instance["type"] = "INSTANCE";
            instance["guid"] = setId + ":fixture";
            instance["name"] = name + " fixture";
            instance["mainStateGroup"] = setId;
            instance["mainComponent"] = masterId;
            instance["props"] = QJsonArray{};
            return qMakePair(QJsonObject{{"pixComponentTreeDslNodes", QJsonArray{family}},
                                         {"pixTreeNodes", QJsonArray{}}},
                             instance);
        };
        const auto standaloneFixture = [](const QString& componentId, const QString& name,
                                          const QList<QPair<QString, QString>>& properties) {
            QJsonArray variants;
            for (const auto& property : properties)
                variants.append(QJsonObject{{"name", property.first},
                                            {"value", property.second},
                                            {"type", "variant"}});
            const QString componentKey = componentId + ":component-key";
            QJsonObject definition{{"guid", componentId},
                                   {"name", name},
                                   {"componentKey", componentKey},
                                   {"propRefMap", QJsonObject{{"variant", variants}}},
                                   {"childNode", QJsonArray{}}};
            QJsonObject instance = definition;
            instance["type"] = "INSTANCE";
            instance["guid"] = componentId + ":fixture";
            instance["name"] = name + " fixture";
            instance["mainStateGroup"] = "";
            instance["mainComponent"] = componentId;
            instance["props"] = QJsonArray{};
            return qMakePair(
                QJsonObject{{"pixComponentTreeDslNodes", QJsonArray{definition}},
                            {"pixTreeNodes", QJsonArray{}}},
                instance);
        };

        {
            const auto sample = standaloneFixture("482:491", "ComponentLab/Menu", {});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* menu = qobject_cast<commonWidget::Z_Menu*>(widget.get());
            QVERIFY(menu);
            QCOMPARE(menu->actions().size(), 8);
            QVERIFY(menu->actions().at(2)->isSeparator());
            QVERIFY(menu->actions().at(3)->isCheckable());
            QVERIFY(menu->actions().at(3)->isChecked());
            QVERIFY(menu->actions().at(5)->menu());
            QVERIFY(!menu->actions().at(6)->isEnabled());
            QCOMPARE(menu->actions().at(1)->shortcut(), QKeySequence("Ctrl+O"));
            const auto trace = converter.trace();
            QCOMPARE(trace.size(), 1);
            QCOMPARE(trace.first().toObject()["calls"].toArray().first().toObject()["api"].toString(),
                     QString("navigation.populateMenu"));
        }
        {
            const auto sample = standaloneFixture("487:23", "ComponentLab/MenuBar",
                                                  {{"Show corner", "False"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* menuBar = qobject_cast<commonWidget::Z_MenuBar*>(widget.get());
            QVERIFY(menuBar);
            QCOMPARE(menuBar->actions().size(), 5);
            QVERIFY(menuBar->actions().first()->menu());
            QVERIFY(menuBar->cornerWidget());
            QVERIFY(menuBar->cornerWidget()->isHidden());
            QCOMPARE(menuBar->property("showCorner").toBool(), false);
            commonWidget::navigation::setMenuBarCornerVisible(menuBar, true);
            QVERIFY(!menuBar->cornerWidget()->isHidden());
        }
        {
            const auto sample = fixture("494:370", "ComponentLab/ToolBar",
                                        {{"Orientation", "Vertical"},
                                         {"Placement", "Floating"},
                                         {"Capacity", "Overflow"},
                                         {"Presentation", "TextBesideIcon"},
                                         {"Show handle", "False"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* toolBar = qobject_cast<commonWidget::Z_ToolBar*>(widget.get());
            QVERIFY(toolBar);
            QCOMPARE(toolBar->orientation(), Qt::Vertical);
            QCOMPARE(toolBar->size(), QSize(120, 220));
            QCOMPARE(toolBar->property("figmaPlacementPreview").toString(),
                     QString("Floating"));
            QVERIFY(toolBar->graphicsEffect());
            QVERIFY(!toolBar->findChild<QWidget*>("figmaToolBarHandle"));
            auto* overflow = toolBar->findChild<QToolButton*>("figmaOverflowButton");
            QVERIFY(overflow);
            QVERIFY(overflow->menu());
            QCOMPARE(overflow->popupMode(), QToolButton::InstantPopup);
            const auto actionButtons = toolBar->findChildren<QToolButton*>();
            int commandButtonCount = 0;
            for (auto* button : actionButtons) {
                if (!button->defaultAction())
                    continue;
                ++commandButtonCount;
                QCOMPARE(button->toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
            }
            QCOMPARE(commandButtonCount, 2);
            const auto trace = converter.trace().first().toObject()["calls"].toArray();
            QCOMPARE(trace.first().toObject()["api"].toString(),
                     QString("navigation.configureToolBar"));
            QCOMPARE(trace.first().toObject()["properties"].toObject()["Show handle"].toString(),
                     QString("false"));
        }
        {
            const auto sample = standaloneFixture("500:849", "ComponentLab/TabWidget", {});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* tabs = qobject_cast<commonWidget::Z_TabWidget*>(widget.get());
            QVERIFY(tabs);
            QCOMPARE(tabs->count(), 4);
            QCOMPARE(tabs->currentIndex(), 0);
            QCOMPARE(tabs->tabText(0), QString("Overview"));
            QCOMPARE(tabs->widget(2)->objectName(), QString("tabDemoBehavior"));
            QVERIFY(!tabs->isTabEnabled(3));
            QVERIFY(tabs->tabsClosable());
            QVERIFY(tabs->tabBar()->tabButton(0, QTabBar::LeftSide) == nullptr);
            QVERIFY(tabs->tabBar()->tabButton(0, QTabBar::RightSide) == nullptr);
            QVERIFY(tabs->tabBar()->tabButton(2, QTabBar::LeftSide) != nullptr ||
                    tabs->tabBar()->tabButton(2, QTabBar::RightSide) != nullptr);
            const auto trace = converter.trace().first().toObject()["calls"].toArray();
            QCOMPARE(trace.first().toObject()["api"].toString(),
                     QString("tabs.populateTabWidget"));
        }
        {
            const auto sample = standaloneFixture("505:555", "ComponentLab/ToolBox", {});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* box = qobject_cast<commonWidget::Z_ToolBox*>(widget.get());
            QVERIFY(box);
            QCOMPARE(box->count(), 3);
            QCOMPARE(box->currentIndex(), 1);
            QCOMPARE(box->itemText(0), QString("&General"));
            QCOMPARE(box->widget(1)->objectName(), QString("toolBoxDemoRules"));
            QVERIFY(!box->isItemEnabled(2));
            const auto trace = converter.trace().first().toObject()["calls"].toArray();
            QCOMPARE(trace.first().toObject()["api"].toString(),
                     QString("toolbox.populateToolBox"));
        }
        {
            const auto sample = fixture("507:443", "ComponentLab/DockWidget",
                                        {{"Area", "Bottom"},
                                         {"Placement", "Floating"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* dock = qobject_cast<commonWidget::Z_DockWidget*>(widget.get());
            QVERIFY(dock);
            commonWidget::Z_DockWidget nativeBaseline;
            const auto originalFeatures = dock->features();
            const auto originalAllowedAreas = dock->allowedAreas();
            QCOMPARE(dock->isFloating(), nativeBaseline.isFloating());
            QCOMPARE(dock->windowTitle(), QString("Inspector"));
            QCOMPARE(dock->property("figmaDockAreaPreview").toString(), QString("Bottom"));
            QCOMPARE(dock->property("figmaDockPlacementPreview").toString(),
                     QString("Floating"));
            QVERIFY(dock->widget());
            QCOMPARE(dock->widget()->objectName(), QString("dockDemoContent"));
            auto* caption = dock->findChild<QLabel*>("dockDemoCaption");
            QVERIFY(caption);
            QCOMPARE(caption->text(), QString::fromUtf8("Bottom dock · Floating"));
            QCOMPARE(dock->features(), originalFeatures);
            QCOMPARE(dock->allowedAreas(), originalAllowedAreas);
            QCOMPARE(dock->isFloating(), nativeBaseline.isFloating());
            const auto trace = converter.trace().first().toObject()["calls"].toArray();
            QCOMPARE(trace.first().toObject()["api"].toString(),
                     QString("dock.configureDockWidget"));
            QCOMPARE(trace.first().toObject()["properties"].toObject()["Area"].toString(),
                     QString("Bottom"));
        }
        {
            const auto sample = fixture("511:124", "ComponentLab/MdiSubWindow",
                                        {{"State", "Shaded"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* sub = qobject_cast<commonWidget::Z_MdiSubWindow*>(widget.get());
            QVERIFY(sub);
            QCOMPARE(sub->property("figmaMdiSnapshotState").toString(), QString("Shaded"));
            QVERIFY(!sub->isShaded());
            QVERIFY(sub->isEnabled());
            QVERIFY(sub->widget());
            QCOMPARE(sub->widget()->objectName(), QString("mdiDocument"));
            const auto call = converter.trace().first().toObject()["calls"].toArray().first().toObject();
            QCOMPARE(call["api"].toString(), QString("mdi.configureSubWindow"));
            QCOMPARE(call["properties"].toObject()["State"].toString(), QString("Shaded"));
        }
        {
            const auto sample = fixture("511:280", "ComponentLab/MdiArea",
                                        {{"Presentation", "Tile"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* area = qobject_cast<commonWidget::Z_MdiArea*>(widget.get());
            QVERIFY(area);
            QCOMPARE(area->property("figmaMdiPresentationPreview").toString(),
                     QString("Tile"));
            QCOMPARE(area->viewMode(), QMdiArea::SubWindowView);
            QCOMPARE(area->subWindowList().size(), 4);
            QVERIFY(!area->subWindowList()[0]->geometry().intersects(
                area->subWindowList()[1]->geometry()));
            const auto call = converter.trace().first().toObject()["calls"].toArray().first().toObject();
            QCOMPARE(call["api"].toString(), QString("mdi.configureArea"));
            QCOMPARE(call["properties"].toObject()["Presentation"].toString(),
                     QString("Tile"));
        }
        {
            const auto sample = fixture("516:62", "ComponentLab/StackedWidget",
                                        {{"State", "Disabled"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* stack = qobject_cast<commonWidget::Z_StackedWidget*>(widget.get());
            QVERIFY(stack);QCOMPARE(stack->count(), 1);QVERIFY(!stack->isEnabled());
            QCOMPARE(stack->property("figmaFlowStatePreview").toString(),QString("Disabled"));
            QCOMPARE(converter.trace().first().toObject()["calls"].toArray().first().toObject()["api"].toString(),QString("pageflow.configureStack"));
        }
        {
            const auto sample = fixture("516:113", "ComponentLab/WizardPage",
                                        {{"Kind", "Final"}, {"Completion", "Incomplete"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* page = qobject_cast<commonWidget::Z_WizardPage*>(widget.get());
            QVERIFY(page);QVERIFY(!page->isCommitPage());
            QCOMPARE(page->property("figmaFlowKindPreview").toString(),QString("Final"));
            QVERIFY(page->isComplete());
            QCOMPARE(page->property("figmaFlowCompletionPreview").toString(),QString("Incomplete"));
        }
        {
            const auto sample = fixture("516:669", "ComponentLab/Wizard",
                                        {{"Style", "Modern"}, {"Phase", "Commit"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* wizard = qobject_cast<commonWidget::Z_Wizard*>(widget.get());
            QVERIFY(wizard);QCOMPARE(wizard->wizardStyle(),QWizard::ModernStyle);
            QCOMPARE(wizard->currentId(),0);QCOMPARE(wizard->pageIds().size(),4);
            QVERIFY(wizard->page(2)->isCommitPage());QVERIFY(wizard->page(3)->isFinalPage());
            QCOMPARE(wizard->property("figmaFlowPhasePreview").toString(),QString("Commit"));
        }

        {
            const auto sample = fixture("414:39", "ComponentLab/Label",
                                        {{"Alignment", "End"}, {"Wrap", "True"},
                                         {"State", "Disabled"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* label = qobject_cast<QLabel*>(widget.get());
            QVERIFY(label);QCOMPARE(label->text(),QString("Mapped label text"));
            QVERIFY(label->wordWrap());QVERIFY(!label->isEnabled());
            QCOMPARE(label->property("figmaLogicalAlignment").toString(),QString("End"));
            QCOMPARE(label->focusPolicy(),Qt::NoFocus);
            QCOMPARE(converter.trace().first().toObject()["calls"].toArray().first().toObject()["api"].toString(),
                     QString("display.configureLabel"));
        }
        {
            const auto sample = fixture("1116:18", "ComponentLab/LineEdit",
                                        {{"Size", "Large"}, {"State", "Error"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* edit = qobject_cast<QLineEdit*>(widget.get());
            QVERIFY(edit);QCOMPARE(edit->height(),40);QVERIFY(edit->isEnabled());
            QCOMPARE(edit->property("figmaLineEditSnapshotState").toString(),QString("Error"));
            QCOMPARE(converter.trace().first().toObject()["calls"].toArray().first().toObject()["api"].toString(),
                     QString("input.configureLineEdit"));
        }
        {
            const auto sample = fixture("467:195", "ComponentLab/Slider",
                                        {{"Orientation", "Vertical"}, {"State", "Dragging"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* slider = qobject_cast<QSlider*>(widget.get());
            QVERIFY(slider);QCOMPARE(slider->orientation(),Qt::Vertical);QVERIFY(slider->isEnabled());
            QVERIFY(!slider->isSliderDown());
            QCOMPARE(slider->property("figmaSliderSnapshotState").toString(),QString("Dragging"));
            QCOMPARE(converter.trace().first().toObject()["calls"].toArray().first().toObject()["api"].toString(),
                     QString("input.configureSlider"));
        }
        {
            const auto sample = fixture("471:477", "ComponentLab/ScrollBar",
                                        {{"Orientation", "Horizontal"}, {"State", "Disabled"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* bar = qobject_cast<QScrollBar*>(widget.get());
            QVERIFY(bar);QCOMPARE(bar->orientation(),Qt::Horizontal);QVERIFY(!bar->isEnabled());
            QCOMPARE(bar->focusPolicy(),Qt::NoFocus);
            QCOMPARE(converter.trace().first().toObject()["calls"].toArray().first().toObject()["api"].toString(),
                     QString("input.configureScrollBar"));
        }
        {
            const auto sample = fixture("465:189", "ComponentLab/TimePicker",
                                        {{"Format", "12h"}, {"State", "ReadOnly"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* edit = qobject_cast<QTimeEdit*>(widget.get());
            QVERIFY(edit);QVERIFY(edit->isReadOnly());QVERIFY(edit->isEnabled());
            QCOMPARE(edit->displayFormat(),QString("hh:mm:ss AP"));
            QCOMPARE(converter.trace().first().toObject()["calls"].toArray().first().toObject()["api"].toString(),
                     QString("input.configureTimePicker"));
        }
        {
            const auto sample = fixture("443:222", "ComponentLab/ToolButton",
                                        {{"Surface", "AutoRaise"}, {"Menu", "Split"},
                                         {"State", "CheckedFocus"},
                                         {"Show label", "False"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* tool = qobject_cast<QToolButton*>(widget.get());
            QVERIFY(tool);QVERIFY(tool->autoRaise());QVERIFY(tool->isCheckable());
            QVERIFY(tool->isChecked());QCOMPARE(tool->popupMode(),QToolButton::MenuButtonPopup);
            QCOMPARE(tool->toolButtonStyle(),Qt::ToolButtonIconOnly);
            QCOMPARE(tool->text(),QString("Mapped label"));
            QCOMPARE(tool->property("figmaToolSnapshotState").toString(),QString("CheckedFocus"));
            QCOMPARE(converter.trace().first().toObject()["calls"].toArray().first().toObject()["api"].toString(),
                     QString("actions.configureToolButton"));
        }
        {
            const auto sample = fixture("443:222", "ComponentLab/ToolButton",
                                        {{"Surface", "Framed"}, {"Menu", "None"},
                                         {"State", "Open"}, {"Show label", "True"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            QVERIFY_EXCEPTION_THROWN(converter.convert(sample.second, nullptr),std::invalid_argument);
        }
        {
            const auto sample = fixture("447:593", "ComponentLab/DialogButtonGroup",
                                        {{"Orientation", "Vertical"}, {"Centered", "True"},
                                         {"Configuration", "Destructive"},
                                         {"State", "Default"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* box = qobject_cast<QDialogButtonBox*>(widget.get());
            QVERIFY(box);QCOMPARE(box->orientation(),Qt::Vertical);QVERIFY(box->centerButtons());
            QCOMPARE(box->buttons().size(),2);
            auto* action=box->findChild<QPushButton*>("dialogDeleteAction");
            QVERIFY(action);QCOMPARE(box->buttonRole(action),QDialogButtonBox::DestructiveRole);
            QVERIFY(!action->isDefault());QVERIFY(box->button(QDialogButtonBox::Cancel)->isDefault());
            QCOMPARE(converter.trace().first().toObject()["calls"].toArray().first().toObject()["api"].toString(),
                     QString("actions.configureDialogButtonGroup"));
        }
        {
            const auto sample = fixture("450:102", "ComponentLab/CommandLinkButton",
                                        {{"Surface", "Flat"}, {"State", "CheckedFocus"},
                                         {"Show description", "False"},
                                         {"Show icon", "False"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* command = qobject_cast<QCommandLinkButton*>(widget.get());
            QVERIFY(command);QVERIFY(command->isFlat());QVERIFY(command->isCheckable());
            QVERIFY(command->isChecked());QCOMPARE(command->text(),QString("Mapped metric title"));
            QCOMPARE(command->description(),QString("Mapped consequence"));
            QVERIFY(!command->property("figmaCommandShowDescription").toBool());
            QVERIFY(!command->property("figmaCommandShowIcon").toBool());
            QCOMPARE(converter.trace().first().toObject()["calls"].toArray().first().toObject()["api"].toString(),
                     QString("actions.configureCommandLinkButton"));
        }
        {
            const auto sample = fixture("410:57", "ComponentLab/ProgressBar",
                                        {{"Orientation", "Vertical"}, {"State", "Determinate"},
                                         {"Inverted", "True"}, {"Show label", "False"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* progress = qobject_cast<QProgressBar*>(widget.get());
            QVERIFY(progress);QCOMPARE(progress->orientation(),Qt::Vertical);
            QVERIFY(progress->invertedAppearance());QVERIFY(!progress->isTextVisible());
            QCOMPARE(progress->format(),QString("68%"));QCOMPARE(progress->focusPolicy(),Qt::NoFocus);
            QCOMPARE(converter.trace().first().toObject()["calls"].toArray().first().toObject()["api"].toString(),
                     QString("display.configureProgressBar"));
        }
        {
            const auto sample = fixture("410:57", "ComponentLab/ProgressBar",
                                        {{"Orientation", "Vertical"}, {"State", "Completed"},
                                         {"Inverted", "False"}, {"Show label", "True"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            QVERIFY_EXCEPTION_THROWN(converter.convert(sample.second, nullptr),std::invalid_argument);
        }
        {
            const auto sample = fixture("397:39", "ComponentLab/MetricCard",
                                        {{"State", "Default"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* metric = qobject_cast<commonWidget::Z_MetricCard*>(widget.get());
            QVERIFY(metric);
            QCOMPARE(metric->state(), QString("Default"));
            QCOMPARE(metric->titleText(), QString("Mapped metric title"));
            QCOMPARE(metric->valueText(), QString("98.6%"));
            QCOMPARE(metric->detailText(), QString("+2.4% mapped"));
            QCOMPARE(metric->focusPolicy(), Qt::NoFocus);
            QCOMPARE(metric->sizeHint(), QSize(360, 176));
            QCOMPARE(converter.trace().first().toObject()["calls"].toArray().first().toObject()["api"].toString(),
                     QString("display.configureMetricCard"));
        }
        {
            const auto sample = fixture("436:79", "ComponentLab/Statistic",
                                        {{"Size", "Compact"}, {"State", "Default"},
                                         {"Show title", "True"}, {"Show prefix", "True"},
                                         {"Show suffix", "True"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* statistic = qobject_cast<commonWidget::Z_Statistic*>(widget.get());
            QVERIFY(statistic);
            QCOMPARE(statistic->size(), QString("Compact"));
            QCOMPARE(statistic->state(), QString("Default"));
            QCOMPARE(statistic->titleText(), QString("Mapped metric title"));
            QCOMPARE(statistic->prefix(), QString("$"));
            QCOMPARE(statistic->valueText(), QString("93,241.5"));
            QCOMPARE(statistic->suffix(), QString("USD"));
            QVERIFY(statistic->showTitle());
            QVERIFY(statistic->showPrefix());
            QVERIFY(statistic->showSuffix());
            QCOMPARE(statistic->focusPolicy(), Qt::NoFocus);
            QCOMPARE(statistic->sizeHint(), QSize(300, 150));
            QCOMPARE(converter.trace().first().toObject()["calls"].toArray().first().toObject()["api"].toString(),
                     QString("display.configureStatistic"));
        }
        {
            const auto sample = fixture("436:79", "ComponentLab/Statistic",
                                        {{"Size", "Default"}, {"State", "Default"},
                                         {"Show title", "Maybe"}, {"Show prefix", "False"},
                                         {"Show suffix", "False"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            QVERIFY_EXCEPTION_THROWN(converter.convert(sample.second, nullptr), std::runtime_error);
        }
        {
            const auto sample = fixture("2132:21", "ComponentLab/Badge",
                                        {{"Kind", "Dot"}, {"Tone", "Danger"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* badge = qobject_cast<commonWidget::Z_Badge*>(widget.get());
            QVERIFY(badge);
            QCOMPARE(badge->kind(), QString("Dot"));
            QCOMPARE(badge->tone(), QString("Danger"));
            QCOMPARE(badge->label(), QString("Mapped label"));
            QCOMPARE(badge->focusPolicy(), Qt::NoFocus);
        }
        {
            const auto sample = fixture("2132:69", "ComponentLab/StatusIndicator",
                                        {{"Size", "Medium"}, {"Tone", "Process"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* indicator = qobject_cast<commonWidget::Z_StatusIndicator*>(widget.get());
            QVERIFY(indicator);
            QCOMPARE(indicator->size(), QString("Medium"));
            QCOMPARE(indicator->tone(), QString("Process"));
            QCOMPARE(indicator->label(), QString("Mapped label"));
            QVERIFY(indicator->showLabel());
            QCOMPARE(indicator->focusPolicy(), Qt::NoFocus);
        }
        {
            const auto sample = fixture("2132:102", "ComponentLab/Link",
                                        {{"State", "Disabled"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* link = qobject_cast<commonWidget::Z_Link*>(widget.get());
            QVERIFY(link);
            QCOMPARE(link->text(), QString("Mapped label"));
            QCOMPARE(link->snapshotState(), QString("Disabled"));
            QVERIFY(!link->isEnabled());
            QCOMPARE(link->focusPolicy(), Qt::StrongFocus);
        }
        {
            const auto sample = fixture("2132:150", "ComponentLab/Tag",
                                        {{"Size", "Medium"}, {"Tone", "Warning"},
                                         {"Dismissible", "True"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* tag = qobject_cast<commonWidget::Z_Tag*>(widget.get());
            QVERIFY(tag);
            QCOMPARE(tag->size(), QString("Medium"));
            QCOMPARE(tag->tone(), QString("Warning"));
            QCOMPARE(tag->label(), QString("Mapped label"));
            QVERIFY(tag->isDismissible());
            QCOMPARE(tag->focusPolicy(), Qt::NoFocus);
            const auto actions = tag->findChildren<QToolButton*>();
            QCOMPARE(actions.size(), 1);
            QCOMPARE(actions.first()->focusPolicy(), Qt::StrongFocus);
            QSignalSpy dismissed(tag, &commonWidget::Z_Tag::dismissed);
            actions.first()->click();
            QCOMPARE(dismissed.count(), 1);
        }
        {
            const auto sample = fixture("2132:189", "ComponentLab/Tooltip",
                                        {{"Placement", "Right"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* tooltip = qobject_cast<commonWidget::Z_Tooltip*>(widget.get());
            QVERIFY(tooltip);
            QCOMPARE(tooltip->placement(), QString("Right"));
            QCOMPARE(tooltip->content(), QString("Mapped content"));
            QVERIFY(tooltip->showArrow());
            QCOMPARE(tooltip->focusPolicy(), Qt::NoFocus);
        }
        {
            const auto sample = fixture("2147:768", "ComponentLab/SearchInput",
                                        {{"State", "Error"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* search = qobject_cast<commonWidget::Z_SearchInput*>(widget.get());
            QVERIFY(search);
            QCOMPARE(search->snapshotState(), QString("Error"));
            QCOMPARE(search->focusPolicy(), Qt::NoFocus);
            QVERIFY(search->lineEdit()->focusPolicy() != Qt::NoFocus);
        }
        {
            const auto sample = fixture("2147:985", "ComponentLab/NumberInput",
                                        {{"Type", "Decimal"}, {"State", "Focus"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* number = qobject_cast<commonWidget::Z_NumberInput*>(widget.get());
            QVERIFY(number);
            QCOMPARE(number->type(), QString("Decimal"));
            QCOMPARE(number->snapshotState(), QString("Focus"));
        }
        {
            const auto sample = fixture("2147:1192", "ComponentLab/OverflowMenuButton",
                                        {{"State", "Disabled"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* overflow = qobject_cast<commonWidget::Z_OverflowMenuButton*>(widget.get());
            QVERIFY(overflow);
            QVERIFY(!overflow->isEnabled());
        }
        {
            const auto sample = fixture("2148:1240", "ComponentLab/SegmentedControl",
                                        {{"Segments", "4"}, {"State", "Default"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* segmented = qobject_cast<commonWidget::Z_SegmentedControl*>(widget.get());
            QVERIFY(segmented);
            QCOMPARE(segmented->segmentCount(), 4);
            QVERIFY(segmented->isEnabled());
        }
        {
            const auto sample = fixture("2147:35", "ComponentLab/Timeline",
                                        {{"Density", "Compact"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* timeline = qobject_cast<commonWidget::Z_Timeline*>(widget.get());
            QVERIFY(timeline);
            QCOMPARE(timeline->density(), QString("Compact"));
            QCOMPARE(timeline->sizeHint(), QSize(420, 190));
            QCOMPARE(timeline->focusPolicy(), Qt::NoFocus);
        }
        {
            const auto sample = fixture("2150:82", "ComponentLab/AnchorNavigation",
                                        {{"Orientation", "Horizontal"},
                                         {"State", "Scrolled"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* anchor = qobject_cast<commonWidget::Z_AnchorNavigation*>(widget.get());
            QVERIFY(anchor);
            QCOMPARE(anchor->orientation(), QString("Horizontal"));
            QCOMPARE(anchor->snapshotState(), QString("Scrolled"));
            QCOMPARE(anchor->currentIndex(), 1);
            QCOMPARE(anchor->sizeHint(), QSize(520, 64));
            QCOMPARE(anchor->focusPolicy(), Qt::NoFocus);
            QCOMPARE(anchor->findChildren<QToolButton*>().size(), 3);
        }
        {
            const auto sample = fixture("2148:244", "ComponentLab/Filter",
                                        {{"Layout", "Panel"},
                                         {"State", "Applied"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* filter = qobject_cast<commonWidget::Z_Filter*>(widget.get());
            QVERIFY(filter);
            QCOMPARE(filter->layoutMode(), QString("Panel"));
            QCOMPARE(filter->snapshotState(), QString("Applied"));
            QCOMPARE(filter->sizeHint(), QSize(430, 112));
            QCOMPARE(filter->focusPolicy(), Qt::NoFocus);
            QCOMPARE(filter->findChildren<QToolButton*>().size(), 3);
        }
        {
            const auto sample = fixture("2147:1390", "ComponentLab/ColorPicker",
                                        {{"State", "Focus"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* colorPicker = qobject_cast<commonWidget::Z_ColorPicker*>(widget.get());
            QVERIFY(colorPicker);
            QCOMPARE(colorPicker->snapshotState(), QString("Focus"));
            QCOMPARE(colorPicker->color(), QColor("#5B5BD6"));
            QCOMPARE(colorPicker->colorText(), QString("#5B5BD6"));
            QCOMPARE(colorPicker->sizeHint(), QSize(300, 40));
            QCOMPARE(colorPicker->focusPolicy(), Qt::StrongFocus);
        }
        {
            const auto sample = fixture("2148:640", "ComponentLab/Transfer",
                                        {{"State", "Loading"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* transfer = qobject_cast<commonWidget::Z_Transfer*>(widget.get());
            QVERIFY(transfer);
            QCOMPARE(transfer->snapshotState(), QString("Loading"));
            QCOMPARE(transfer->sizeHint(), QSize(520, 210));
            QCOMPARE(transfer->focusPolicy(), Qt::NoFocus);
            QCOMPARE(transfer->findChildren<QListView*>().size(), 2);
            QCOMPARE(transfer->findChildren<QToolButton*>().size(), 2);
        }
        {
            const auto sample = fixture("2148:818", "ComponentLab/Upload",
                                      {{"State", "Uploading"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* upload = qobject_cast<commonWidget::Z_Upload*>(widget.get());
            QVERIFY(upload);
            QCOMPARE(upload->snapshotState(), QString("Uploading"));
            QCOMPARE(upload->sizeHint(), QSize(390, 150));
            QCOMPARE(upload->focusPolicy(), Qt::StrongFocus);
            QCOMPARE(upload->progress(), 0);
        }
        {
            const auto sample = fixture("2148:1018", "ComponentLab/NumericKeypad",
                                        {{"Density", "Compact"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* keypad = qobject_cast<commonWidget::Z_NumericKeypad*>(widget.get());
            QVERIFY(keypad);
            QCOMPARE(keypad->density(), QString("Compact"));
            QCOMPARE(keypad->sizeHint(), QSize(260, 250));
            QCOMPARE(keypad->focusPolicy(), Qt::NoFocus);
            QCOMPARE(keypad->findChildren<QToolButton*>().size(), 12);
        }
        {
            const auto sample = fixture("2147:515", "ComponentLab/Stepper",
                                        {{"State", "Completed"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* stepper = qobject_cast<commonWidget::Z_Stepper*>(widget.get());
            QVERIFY(stepper);
            QCOMPARE(stepper->snapshotState(), QString("Completed"));
            QCOMPARE(stepper->focusPolicy(), Qt::NoFocus);
        }
        {
            const auto sample = fixture("2148:438", "ComponentLab/Pagination",
                                        {{"Density", "Compact"}, {"State", "Disabled"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* pagination = qobject_cast<commonWidget::Z_Pagination*>(widget.get());
            QVERIFY(pagination);
            QCOMPARE(pagination->density(), QString("Compact"));
            QCOMPARE(pagination->snapshotState(), QString("Disabled"));
            QVERIFY(!pagination->isEnabled());
            QCOMPARE(pagination->sizeHint(), QSize(316, 44));
            QCOMPARE(pagination->findChildren<QToolButton*>().size(), 9);
            QCOMPARE(pagination->findChildren<QLabel*>("paginationEllipsis").size(), 2);
        }
        {
            const auto sample = fixture("2148:74", "ComponentLab/PageHeader",
                                        {{"Density", "Compact"}, {"Actions", "Off"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* pageHeader = qobject_cast<commonWidget::Z_PageHeader*>(widget.get());
            QVERIFY(pageHeader);
            QCOMPARE(pageHeader->density(), QString("Compact"));
            QVERIFY(!pageHeader->actionsVisible());
        }
        {
            const auto sample = fixture("2150:436", "ComponentLab/TitleBar",
                                        {{"State", "Maximized"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* titleBar = qobject_cast<commonWidget::Z_TitleBar*>(widget.get());
            QVERIFY(titleBar);
            QVERIFY(titleBar->isMaximizedState());
            QCOMPARE(titleBar->findChildren<QToolButton*>().size(), 3);
        }
        {
            const auto sample = fixture("380:113", "ComponentLab/Navbar",
                                        {{"Density", "Compact"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* navbar = qobject_cast<commonWidget::Z_Navbar*>(widget.get());
            QVERIFY(navbar);
            QCOMPARE(navbar->density(), QString("Compact"));
            QCOMPARE(navbar->sizeHint(), QSize(1200, 56));
            QCOMPARE(navbar->focusPolicy(), Qt::NoFocus);
            QVERIFY(navbar->searchInput());
            QCOMPARE(navbar->notificationButton()->focusPolicy(), Qt::StrongFocus);
            QCOMPARE(navbar->profileButton()->focusPolicy(), Qt::StrongFocus);
        }
        {
            const auto sample = fixture("392:216", "ComponentLab/Sidebar",
                                        {{"Density", "Compact"}, {"Collapsed", "True"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* sidebar = qobject_cast<commonWidget::Z_Sidebar*>(widget.get());
            QVERIFY(sidebar);
            QCOMPARE(sidebar->density(), QString("Compact"));
            QVERIFY(sidebar->isCollapsed());
            QCOMPARE(sidebar->sizeHint(), QSize(80, 466));
            QCOMPARE(sidebar->focusPolicy(), Qt::NoFocus);
            QCOMPARE(sidebar->findChildren<QToolButton*>().size(), 7);
        }
        {
            const auto sample = fixture("520:120", "ComponentLab/Splitter",
                                        {{"Orientation", "Vertical"}, {"State", "Default"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* splitter = qobject_cast<commonWidget::Z_Splitter*>(widget.get());
            QVERIFY(splitter);
            QCOMPARE(splitter->orientation(), Qt::Vertical);
            QCOMPARE(splitter->handleWidth(), 12);
            QCOMPARE(splitter->focusPolicy(), Qt::NoFocus);
            QVERIFY(splitter->showGrip());
        }
        {
            const auto sample = fixture("531:912", "ComponentLab/ScrollArea",
                                        {{"Bars", "Both"}, {"Resize", "Resizable"},
                                         {"State", "Focus"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* area = qobject_cast<commonWidget::Z_ScrollArea*>(widget.get());
            QVERIFY(area);
            QCOMPARE(area->bars(), QString("Both"));
            QCOMPARE(area->resizeMode(), QString("Resizable"));
            QCOMPARE(area->snapshotState(), QString("Focus"));
            QCOMPARE(area->focusPolicy(), Qt::StrongFocus);
        }
        {
            const auto sample = fixture("423:859", "ComponentLab/Calendar",
                                        {{"WeekStart", "Sunday"}, {"Grid", "False"},
                                         {"State", "Disabled"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* calendar = qobject_cast<commonWidget::Z_CalendarWidget*>(widget.get());
            QVERIFY(calendar);
            QCOMPARE(calendar->weekStart(), QString("Sunday"));
            QVERIFY(!calendar->grid());
            QCOMPARE(calendar->snapshotState(), QString("Disabled"));
            QVERIFY(!calendar->isEnabled());
            QCOMPARE(calendar->horizontalHeaderFormat(), QCalendarWidget::SingleLetterDayNames);
        }
        {
            const auto sample = fixture("1312:96", "ComponentLab/DesktopBottomNavigation",
                                        {{"IconPlacement", "Start"}});
            d2c::ComponentConverter converter(sample.first, mapping);
            std::unique_ptr<QWidget> widget(converter.convert(sample.second, nullptr));
            auto* bottom = qobject_cast<commonWidget::Z_DesktopBottomNavigation*>(widget.get());
            QVERIFY(bottom);
            QCOMPARE(bottom->iconPlacement(), QString("Start"));
            QCOMPARE(bottom->sizeHint(), QSize(840, 122));
            QCOMPARE(bottom->focusPolicy(), Qt::NoFocus);
            QCOMPARE(bottom->findChildren<QToolButton*>().size(), 3);
            QCOMPARE(bottom->findChildren<QPushButton*>().size(), 3);
        }
        } catch (const std::exception& error) {
            QFAIL(error.what());
        }
    }

    // 更改实例显示名称和位置后仍按母版引用匹配，排除依赖名称猜组件的实现。
    void independentOfInstanceName() {
        const auto raw = read("../docs/fixtures/layout/design-inputs/node-12-382-full/canonical.raw.json");
        d2c::ComponentConverter engine(raw, read("../docs/fixtures/layout/mapping/component-mapping.json"));
        auto n = raw["pixTreeNodes"].toArray()[0].toObject()["childNode"].toArray()[0].toObject();
        n["guid"] = "test:renamed";
        n["name"] = "Unrelated page action";
        n["left"] = 987;
        std::unique_ptr<QWidget> w(engine.convert(n, nullptr));
        QVERIFY(qobject_cast<commonWidget::Z_PushButton*>(w.get()));
        n["mainStateGroup"] = "missing";
        QVERIFY_EXCEPTION_THROWN(engine.convert(n, nullptr), std::runtime_error);
    }
};

int main(int argc, char** argv) {
    // 主线安装器提供已生成的样式和资源，测试使用与展示一致的组件环境。
    QApplication app(argc, argv);
    component_theme::install(app, "standard-light");
    ComponentTests test;
    return QTest::qExec(&test, argc, argv);
}

#include "ComponentTests.moc"
