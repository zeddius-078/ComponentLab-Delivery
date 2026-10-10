// 组件契约的原生测试：保存的 Pixso 实例与受控夹具分别验证，结果写到 component 目录。
// 所有修改仅发生在内存副本，原始设计输入保持原样。
#include "ComponentConverter.h"
#include "LayoutConverter.h"
#include "ComponentTheme.h"
#include "Z_CheckBox.h"
#include "Z_InputField.h"
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QLabel>
#include <QSignalSpy>
#include <QTest>

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
        const auto raw = read("HtmlDsl/pipeline/design-inputs/holdout-19-45-full/canonical.raw.json");
        const auto root = raw["pixTreeNodes"].toArray()[0].toObject();
        d2c::ComponentConverter components(raw, read("HtmlDsl/pipeline/mapping/component-mapping.json"));
        d2c::LayoutConverter layouts(read("HtmlDsl/pipeline/mapping/layout-mapping.json"), raw);
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
            const auto raw = read("HtmlDsl/pipeline/design-inputs/acceptance-" + axis + "-full/canonical.raw.json");
            d2c::ComponentConverter engine(raw, read("HtmlDsl/pipeline/mapping/component-mapping.json"));
            int inputs = 0, checks = 0, enabledButtons = 0, disabledButtons = 0;
            // 逐个实例独立创建控件，核对状态值以及操作后是否保持相应约束。
            for (const auto& value :
                 raw["pixTreeNodes"].toArray()[0].toObject()["childNode"].toArray()) {
                const auto node = value.toObject();
                // 跳过布局或文本节点，本测试只覆盖具有组件映射契约的实例。
                if (node["type"] != "INSTANCE")
                    continue;
                std::unique_ptr<QWidget> widget(engine.convert(node, nullptr));
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
        const auto raw = read("HtmlDsl/pipeline/design-inputs/node-12-382-full/canonical.raw.json");
        const auto rules = read("HtmlDsl/pipeline/mapping/component-mapping.json");
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
    }

    // 从真实母版定义组成测试实例，分别覆盖前图标、后图标、双图标与纯图标模式。
    void resourceMappingFromRealDefinitions() {
        const auto raw = read("HtmlDsl/pipeline/design-inputs/node-12-382-full/canonical.raw.json");
        d2c::ComponentConverter engine(raw, read("HtmlDsl/pipeline/mapping/component-mapping.json"));
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
                // 只选取默认、纯色、Bare、亮色舒适档的对应母版，让本轮只比较图标模式差异。
                if (properties.value("content") == mode && properties.value("state") == "default" &&
                    properties.value("appearance") == "solid" &&
                    properties.value("iconStyle") == "Bare" &&
                    properties.value("Profile") == "light-comfortable") {
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
                QCOMPARE(button->leadingIconSource(), QString(":/icons/arrow-left.svg"));
            // 后侧或双侧模式必须解析到右箭头资源，验证后插槽映射。
            if (mode == "Trailing" || mode == "Dual")
                QCOMPARE(button->trailingIconSource(), QString(":/icons/arrow-right.svg"));
            // 独立图标模式同时核对资源与可访问名称，避免去掉文本后丢失操作含义。
            if (mode == "IconOnly") {
                QCOMPARE(button->leadingIconSource(), QString(":/icons/search.svg"));
                QCOMPARE(button->accessibleName(), QString::fromUtf8("搜索"));
            }
            button->show();
            QApplication::processEvents();
            QSignalSpy spy(button, &QPushButton::clicked);
            button->click();
            QCOMPARE(spy.count(), 1);
        }
        QFile output(QString(D2C_ROOT) + "/../build/reports/component/resource-calls.json");
        QVERIFY(output.open(QIODevice::WriteOnly));
        output.write(QJsonDocument(engine.trace()).toJson());
    }

    // Login 页的四个真实实例：核对文字覆盖、placeholder 和可调用的组件行为。
    void realInstances() {
        const auto raw = read("HtmlDsl/pipeline/design-inputs/node-12-382-full/canonical.raw.json");
        d2c::ComponentConverter engine(raw, read("HtmlDsl/pipeline/mapping/component-mapping.json"));
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
        QFile output(QString(D2C_ROOT) + "/../build/reports/component/runtime-calls.json");
        QVERIFY(output.open(QIODevice::WriteOnly));
        output.write(QJsonDocument(engine.trace()).toJson());
    }

    // 更改实例显示名称和位置后仍按母版引用匹配，排除依赖名称猜组件的实现。
    void independentOfInstanceName() {
        const auto raw = read("HtmlDsl/pipeline/design-inputs/node-12-382-full/canonical.raw.json");
        d2c::ComponentConverter engine(raw, read("HtmlDsl/pipeline/mapping/component-mapping.json"));
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
    component_theme::install(app, "light-comfortable");
    ComponentTests test;
    return QTest::qExec(&test, argc, argv);
}

#include "ComponentTests.moc"
