// 把组件身份、变体属性、文字槽和资源槽转换为受控的 Qt API 调用。
#include "ComponentConverter.h"
#include "ComponentFactory.h"
#include "IconAssetCatalog.h"
#include "NavigationAppearance.h"
#include "DockWidgetAppearance.h"
#include "LabelAppearance.h"
#include "MdiAppearance.h"
#include "PageFlowAppearance.h"
#include "ProgressAppearance.h"
#include "TabAppearance.h"
#include "ToolBoxAppearance.h"
#include "Z_DesignComponents.h"
#include "Z_PushButton.h"
#include "ButtonAppearanceAdapter.h"
#include "CommandLinkAppearance.h"
#include "DialogButtonBoxAppearance.h"
#include "ToolButtonAppearance.h"
#include "LineEditAppearance.h"
#include "SliderAppearance.h"
#include "ScrollBarAppearance.h"
#include "DateTimeAppearance.h"
#include <QJsonDocument>
#include <QLabel>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QProgressBar>
#include <QDockWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QStackedWidget>
#include <QWizard>
#include <QWizardPage>
#include <QToolBar>
#include <QToolButton>
#include <QDialogButtonBox>
#include <QCommandLinkButton>
#include <QLineEdit>
#include <QSlider>
#include <QScrollBar>
#include <QTimeEdit>
#include <QTabWidget>
#include <QToolBox>
#include <QSet>
#include <stdexcept>
#include <functional>

namespace d2c {
namespace {
// 异常保留设计节点与可用属性，便于定位规则缺项；失败时不猜测替代控件。
[[noreturn]] void fail(const QString& code, const QJsonObject& node, const QString& component,
                       const QString& detail) {
    const QJsonObject error{{"code", code},
                            {"nodeId", node["guid"]},
                            {"nodeName", node["name"]},
                            {"componentName", component},
                            {"componentSetRef", node["mainStateGroup"]},
                            {"mainComponentRef", node["mainComponent"]},
                            {"detail", detail},
                            {"availableProperties", node["propRefMap"]}};
    throw std::runtime_error(QJsonDocument(error).toJson(QJsonDocument::Compact).toStdString());
}

// 在指定层级按 GUID 精确查找；递归层级由调用方选择。
QJsonObject find(const QJsonArray& list, const QString& guid) {
    // 只检查调用方传入的这一层，按 GUID 返回精确引用，避免跨组件集误匹配。
    for (const auto& value : list) {
        const auto n = value.toObject();
        // 按 GUID 返回唯一目标定义，保证组件集与母版查找使用稳定身份。
        if (n["guid"].toString() == guid)
            return n;
    }
    return {};
}

// 母版文字层名称作为槽位键；重名会让覆盖来源不明确，因此直接报错。
void collectTextSlots(const QJsonObject& node, QMap<QString, QJsonObject>& out) {
    const auto name = node["name"].toString();
    // 只有文字节点登记槽名；同名槽无法唯一确定实例覆盖目标，必须拒绝。
    if (node.contains("nodeText")) {
        // 同名文字槽无法确定覆盖目标，立即报告歧义，避免后值无声覆盖前值。
        if (out.contains(name))
            throw std::runtime_error("AMBIGUOUS_TEXT_SLOT");
        out[name] = node;
    }
    // 递归收集母版内部文字槽，使嵌套标签也能接收实例覆盖。
    for (const auto& v : node["childNode"].toArray())
        collectTextSlots(v.toObject(), out);
}

// 命中图标前缀即停止下钻，避免把图标内部路径当成额外图标；顺序按 DSL 逆序读取。
void collectGlyphs(const QJsonObject& node, const QString& prefix, QJsonArray& glyphs) {
    // 按映射约定的前缀收集图标节点，使图标资源与前后插槽的职责对应。
    if (node["name"].toString().startsWith(prefix)) {
        glyphs.append(node);
        return;
    }
    const auto children = node["childNode"].toArray();
    // 按 DSL 约定逆序收集图标，保持 leading/trailing 与视觉顺序对应。
    for (int i = children.size() - 1; i >= 0; --i)
        collectGlyphs(children[i].toObject(), prefix, glyphs);
}
}

ComponentConverter::ComponentConverter(QJsonObject input, QJsonObject rules)
    : raw(std::move(input)), mapping(std::move(rules)),
      catalog(std::make_shared<surface::Catalog>(mapping["profile"].toString())),
      componentFactory(std::make_unique<commonWidget::ComponentFactory>(
          mapping["profile"].toString())) {}

ComponentConverter::~ComponentConverter() = default;

QWidget* ComponentConverter::convert(const QJsonObject& n, QWidget* parent) {
    // 用公共设计身份（组件集或独立组件节点）和名称双重匹配规则，再确认实例指向的母版 key。
    const auto definitions = raw["pixComponentTreeDslNodes"].toArray();
    auto family = find(definitions, n["mainStateGroup"].toString());
    const bool standalonePublicComponent = family.isEmpty() &&
        !n["mainComponent"].toString().isEmpty();
    if (standalonePublicComponent)
        family = find(definitions, n["mainComponent"].toString());
    const auto fail = [&](const QString& code, const QJsonObject& node, const QString& detail) {
        d2c::fail(code, node, family["name"].toString(), detail);
    };
    // 组件映射依赖实例引用；普通 FRAME 或文本必须交给其他转换路径。
    if (n["type"] != "INSTANCE")
        fail("INVALID_NODE_TYPE", n, n["type"].toString());
    QJsonObject rule;
    // 遍历声明的映射规则，以名称和 key 双重确认身份；多次命中会造成歧义。
    for (const auto& r : mapping["rules"].toArray()) {
        const auto candidate = r.toObject();
        // 公共独立组件使用 designIdentityId；组件集和旧 key 路径继续兼容。
        const bool identityMatches = candidate.contains("designIdentityId")
            ? candidate["designIdentityId"] == family["guid"]
            : candidate.contains("designComponentSetId")
            ? candidate["designComponentSetId"] == family["guid"]
            : candidate["designComponentKey"] == family["componentKey"];
        if (identityMatches && candidate["designComponentName"] == family["name"]) {
            // 已有命中规则时再次匹配表示配置有歧义，拒绝按遍历顺序任意选取。
            if (!rule.isEmpty())
                fail("AMBIGUOUS_COMPONENT_MAPPING", n, family["name"].toString());
            rule = candidate;
        }
    }
    // 没有契约时立即报告未映射，避免使用默认按钮掩盖设计侧缺项。
    if (rule.isEmpty())
        fail("UNMAPPED_COMPONENT", n, family["name"].toString());
    const auto master = standalonePublicComponent
        ? family
        : find(family["childNode"].toArray(), n["mainComponent"].toString());
    // 核对母版引用及 key，防止实例属性被解释为另一母版的接口参数。
    if (master.isEmpty() || master["componentKey"] != n["componentKey"])
        fail("INVALID_COMPONENT_REFERENCE", n, family["name"].toString());
    // 先校验属性全集、Profile 和值域，再创建控件，减少半成品对象。
    QMap<QString, QString> props;
    // 将变体列表建立为属性字典；重复名称必须报错，避免后值静默覆盖前值。
    for (const auto& p : n["propRefMap"].toObject()["variant"].toArray()) {
        const auto o = p.toObject();
        const auto name = o["name"].toString();
        if (!o["name"].isString() || !o["value"].isString())
            fail("INVALID_PROPERTY_VALUE_TYPE", n, name);
        // 重复变体属性没有明确优先级，停止转换，避免不同解析顺序得到不同状态。
        if (props.contains(name))
            fail("DUPLICATE_PROPERTY", n, name);
        props[name] = o["value"].toString();
    }
    // 限定设计 Profile 与当前 catalog 一致，避免混用主题的尺寸和材质。
    if (props.contains("Profile") && props.value("Profile") != mapping["profile"].toString())
        fail("PROFILE_MISMATCH", n, props.value("Profile"));
    const QString stateProperty = rule["stateProperty"].toString(
        QStringLiteral("state"));
    const QString compositionApi = rule["compositionApi"].toString();
    const auto propertyRules = rule["propertyMappings"].toObject();
    // 逐项核对契约必填属性，缺少数据时在创建 QWidget 前终止。
    for (const auto& required : propertyRules.keys())
        // 每个必填映射属性都要有设计值，缺失时报告错误，防止默认值掩盖不完整设计。
        if (!props.contains(required))
            fail("MISSING_PROPERTY", n, required);
    QJsonObject values;
    QMap<QString, QString> compositionProperties;
    // 将一般变体翻译成 API 参数；Profile 和 state 分别由主题检查及状态规则处理。
    for (auto it = props.cbegin(); it != props.cend(); ++it) {
        // 跳过已分配给专门流程的字段，避免将状态或主题误当通用 setter。
        if (it.key() == "Profile" ||
            (it.key() == stateProperty && !propertyRules.contains(it.key())))
            continue;
        const auto spec = propertyRules[it.key()].toObject();
        const auto domain = spec["values"].toObject();
        // 只接受契约列出的值，防止拼写错误被转换为空参数。
        if (!domain.contains(it.value()))
            fail("UNMAPPED_PROPERTY", n, it.key() + "=" + it.value());
        const auto api = spec["api"].toString();
        const auto mapped = domain[it.value()];
        if (!compositionApi.isEmpty() && api == compositionApi) {
            if (!mapped.isString())
                fail("INVALID_API_VALUE_TYPE", n, api + ":" + it.key());
            compositionProperties[it.key()] = mapped.toString();
        } else {
            values[api] = mapped;
        }
    }
    const auto states = rule["stateMappings"].toObject();
    // 只有明确映射的初始状态可以生成，未知状态不降级为 default。
    if (!states.isEmpty() && !states.contains(props.value(stateProperty)))
        fail("UNMAPPED_STATE", n, props.value(stateProperty));
    const auto cls = rule["qtClass"].toString();
    // 转换期间由 unique_ptr 托管，后续任何 setter 失败都会释放本次控件。
    std::unique_ptr<QWidget> owned;
    using namespace commonWidget;
    Z_PushButton* button = nullptr;
    // 组件创建只通过 GenerateComponent 发布的 SDK Factory；GenerateLayout 不再维护平行类白名单。
    if (!commonWidget::ComponentFactory::isRegistered(cls))
        fail("UNREGISTERED_QT_CLASS", n, cls);
    owned.reset(componentFactory->create(cls, parent));
    button = qobject_cast<Z_PushButton*>(owned.get());
    auto* w = owned.get();
    w->setObjectName("node_" + n["guid"].toString().replace(':', '_'));
    w->setProperty("designNodeId", n["guid"].toString());
    w->setProperty("designComponent", family["name"].toString());
    QJsonArray calls;
    if (!compositionApi.isEmpty()) {
        if (compositionApi == QStringLiteral("navigation.populateMenu")) {
            auto* menu = qobject_cast<QMenu*>(w);
            if (!menu)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::navigation::populateMenu(menu);
        } else if (compositionApi == QStringLiteral("navigation.populateMenuBar")) {
            auto* menuBar = qobject_cast<QMenuBar*>(w);
            if (!menuBar)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::navigation::populateMenuBar(menuBar);
        } else if (compositionApi == QStringLiteral("navigation.configureToolBar")) {
            auto* toolBar = qobject_cast<QToolBar*>(w);
            if (!toolBar)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::navigation::configureToolBar(toolBar, compositionProperties);
        } else if (compositionApi == QStringLiteral("tabs.populateTabWidget")) {
            auto* tabs = qobject_cast<QTabWidget*>(w);
            if (!tabs)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::populateFigmaTabWidget(tabs);
        } else if (compositionApi == QStringLiteral("toolbox.populateToolBox")) {
            auto* toolBox = qobject_cast<QToolBox*>(w);
            if (!toolBox)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::populateFigmaToolBox(toolBox);
        } else if (compositionApi == QStringLiteral("dock.configureDockWidget")) {
            auto* dock = qobject_cast<QDockWidget*>(w);
            if (!dock)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaDockWidget(dock, compositionProperties);
        } else if (compositionApi == QStringLiteral("mdi.configureSubWindow")) {
            auto* sub = qobject_cast<QMdiSubWindow*>(w);
            if (!sub)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaMdiSubWindow(sub, compositionProperties);
        } else if (compositionApi == QStringLiteral("mdi.configureArea")) {
            auto* area = qobject_cast<QMdiArea*>(w);
            if (!area)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaMdiArea(area, compositionProperties);
        } else if (compositionApi == QStringLiteral("pageflow.configureStack")) {
            auto* stack = qobject_cast<QStackedWidget*>(w);
            if (!stack)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaStackedWidget(stack, compositionProperties);
        } else if (compositionApi == QStringLiteral("pageflow.configureWizardPage")) {
            auto* page = qobject_cast<QWizardPage*>(w);
            if (!page)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaWizardPage(page, compositionProperties);
        } else if (compositionApi == QStringLiteral("pageflow.configureWizard")) {
            auto* wizard = qobject_cast<QWizard*>(w);
            if (!wizard)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaWizard(wizard, compositionProperties);
        } else if (compositionApi == QStringLiteral("display.configureLabel")) {
            auto* label = qobject_cast<QLabel*>(w);
            if (!label)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaLabel(label, compositionProperties);
        } else if (compositionApi == QStringLiteral("display.configureProgressBar")) {
            auto* progress = qobject_cast<QProgressBar*>(w);
            if (!progress)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaProgressBar(progress, compositionProperties);
        } else if (compositionApi == QStringLiteral("display.configureMetricCard")) {
            auto* card = qobject_cast<commonWidget::Z_MetricCard*>(w);
            if (!card)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaMetricCard(card, compositionProperties);
        } else if (compositionApi == QStringLiteral("display.configureStatistic")) {
            auto* statistic = qobject_cast<commonWidget::Z_Statistic*>(w);
            if (!statistic)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaStatistic(statistic, compositionProperties);
        } else if (compositionApi == QStringLiteral("actions.configureToolButton")) {
            auto* tool = qobject_cast<QToolButton*>(w);
            if (!tool)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaToolButton(tool, compositionProperties);
        } else if (compositionApi == QStringLiteral("actions.configureDialogButtonGroup")) {
            auto* box = qobject_cast<QDialogButtonBox*>(w);
            if (!box)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaDialogButtonGroup(box, compositionProperties);
        } else if (compositionApi == QStringLiteral("actions.configureCommandLinkButton")) {
            auto* command = qobject_cast<QCommandLinkButton*>(w);
            if (!command)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaCommandLinkButton(command, compositionProperties);
        } else if (compositionApi == QStringLiteral("input.configureLineEdit")) {
            auto* edit = qobject_cast<QLineEdit*>(w);
            if (!edit)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaLineEdit(edit, compositionProperties);
        } else if (compositionApi == QStringLiteral("input.configureSlider")) {
            auto* slider = qobject_cast<QSlider*>(w);
            if (!slider)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaSlider(slider, compositionProperties);
        } else if (compositionApi == QStringLiteral("input.configureScrollBar")) {
            auto* bar = qobject_cast<QScrollBar*>(w);
            if (!bar)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaScrollBar(bar, compositionProperties);
        } else if (compositionApi == QStringLiteral("input.configureTimePicker")) {
            auto* edit = qobject_cast<QTimeEdit*>(w);
            if (!edit)
                fail("INVALID_COMPOSITION_TARGET", n, compositionApi);
            commonWidget::configureFigmaTimePicker(edit, compositionProperties);
        } else {
            fail("UNREGISTERED_COMPOSITION_API", n, compositionApi);
        }
        QJsonObject compositionCall{{"api", compositionApi}};
        if (!compositionProperties.isEmpty()) {
            QJsonObject propertyTrace;
            for (auto it = compositionProperties.cbegin(); it != compositionProperties.cend(); ++it)
                propertyTrace[it.key()] = it.value();
            compositionCall["properties"] = propertyTrace;
        }
        calls.append(compositionCall);
    }
    // 图标槽和图标资源只适用于按钮；其他控件沿用各自的参数路径。
    if (button) {
        // 图标来自母版 GLYPH 槽及实例名称覆盖，再通过资源表找到 QRC/本地路径。
        const auto resources = rule["resourceMappings"].toObject();
        const auto contentSlots = resources["contentSlots"].toObject();
        // 内容模式决定图标槽数量与位置，缺少规则时不能猜测 leading/trailing。
        if (!contentSlots.contains(props.value("content")))
            fail("UNMAPPED_CONTENT_MODE", n, props.value("content"));
        const auto targets = contentSlots[props.value("content")].toArray();
        QJsonArray glyphs;
        collectGlyphs(master, resources["glyphPrefix"].toString(), glyphs);
        // 要求设计图标数量与内容模式的目标槽一致，避免图标错位或被丢弃。
        if (glyphs.size() != targets.size())
            fail("RESOURCE_SLOT_COUNT_MISMATCH", n, family["name"].toString());
        // 逐槽解析设计图标，再写入对应按钮接口和调用轨迹。
        for (int i = 0; i < glyphs.size(); ++i) {
            auto glyph = glyphs[i].toObject();
            // 只应用指向当前图标 GUID 的名称覆盖，保留其他图标及文字覆盖。
            for (const auto& p : n["props"].toArray()) {
                const auto o = p.toObject();
                // 只应用当前图标节点的名称覆盖，保证实例换图标时能查到正确资源。
                if (o["componentId"] == glyph["guid"] && o.contains("name"))
                    glyph["name"] = o["name"];
            }
            const auto resource =
                resources["entries"].toObject()[glyph["name"].toString()].toObject();
            // 未登记的图标没有可信 QRC 路径，停止转换并报告具体槽名。
            if (resource.isEmpty())
                fail("UNMAPPED_RESOURCE", n, glyph["name"].toString());
            QString source;
            if (resource.contains("assetId")) {
                source = resource["assetId"].toString();
                try {
                    // Validate against the GenerateComponent SDK catalog. The stable ID is
                    // passed to the component; GenerateLayout never owns the concrete path.
                    commonWidget::IconAssetCatalog::resolve(source);
                } catch (const std::exception&) {
                    fail("UNMAPPED_RESOURCE", n, source);
                }
            } else {
                // Page-owned assets continue to use their generated QRC/local source.
                source = resource["source"].toString();
            }
            if (source.isEmpty())
                fail("UNMAPPED_RESOURCE", n, glyph["name"].toString());
            const auto target = targets[i].toString();
            // 依据契约选择前或后图标接口；未知槽直接拒绝，防止落到错误位置。
            if (target == "leading")
                button->setLeadingIconSource(source);
            else if (target == "trailing")
                button->setTrailingIconSource(source);
            else
                fail("UNMAPPED_RESOURCE_SLOT", n, target);
            // 独立图标模式缺少可见文本，使用资源契约的名称补足辅助阅读语义。
            if (props.value("content") == "IconOnly")
                button->setAccessibleName(resource["accessibleName"].toString());
            calls.append(QJsonObject{
                {"api", target == "leading" ? "setLeadingIconSource" : "setTrailingIconSource"},
                {"value", source},
                {"designGlyph", glyph["name"]},
                {"designGlyphId", glyph["guid"]}});
        }
    }
    // API 调用同样由 SDK Factory 校验和分派；映射层只负责设计值域到公开 API 的翻译。
    const auto invoke = [&](const QString& api, const QJsonValue& value) {
        static const QSet<QString> booleanApis{"setEnabled", "setDanger", "setGhost",
                                               "setLoading", "setAllowClear", "setReadOnly",
                                               "setActionsVisible", "setDismissible",
                                               "setCollapsed", "setGrid", "setShowGrip",
                                               "navigation.setMenuBarCornerVisible"};
        static const QSet<QString> numericApis{"setSegmentCount", "setCurrentIndex",
                                               "setMinimum", "setMaximum",
                                               "setSingleStep"};
        const bool flexibleValueApi = api == QStringLiteral("setValue");
        if ((flexibleValueApi && !value.isString() && !value.isDouble()) ||
            (booleanApis.contains(api) && !value.isBool()) ||
            (numericApis.contains(api) && !value.isDouble()) ||
            (!flexibleValueApi && !booleanApis.contains(api) &&
             !numericApis.contains(api) && !value.isString()))
            fail("INVALID_API_VALUE_TYPE", n, api);
        QString error;
        if (api == QStringLiteral("navigation.setMenuBarCornerVisible")) {
            auto* menuBar = qobject_cast<QMenuBar*>(w);
            if (!menuBar)
                fail("INVALID_COMPOSITION_TARGET", n, api);
            commonWidget::navigation::setMenuBarCornerVisible(menuBar, value.toBool());
        } else if (!componentFactory->apply(w, api, value, &error)) {
            fail("UNREGISTERED_API", n, api + ": " + error);
        }
        calls.append(QJsonObject{{"api", api}, {"value", value}});
    };
    // 先应用普通属性；材质适配推迟到文字和状态设置之后，保持刷新顺序确定。
    for (auto it = values.begin(); it != values.end(); ++it)
        // 外观适配另有固定执行时机；这里先应用其他属性，避免随后设置属性覆盖材质效果。
        if (it.key() != "ButtonAppearanceAdapter.setAppearance")
            invoke(it.key(), it.value());
    // 文字按“母版默认值 → 实例 props 覆盖”求值；when 控制当前内容模式使用哪些槽。
    QMap<QString, QJsonObject> textSlots;
    collectTextSlots(master, textSlots);
    // 逐条求值文字槽规则，只为当前内容模式启用的槽调用 setter。
    for (const auto& s : rule["slotMappings"].toArray()) {
        const auto slot = s.toObject();
        bool active = true;
        const auto when = slot["when"].toObject();
        // when 的条件须全部满足；任一不满足即关闭本槽，避免给 IconOnly 写入文本。
        for (auto it = when.begin(); it != when.end(); ++it)
            // 条件值不在允许范围时使该槽规则失配，避免给当前内容模式装配不适用的槽。
            if (!it.value().toArray().contains(props.value(it.key())))
                active = false;
        // 不适用的槽直接跳过，其母版默认文字也不应进入当前实例。
        if (!active)
            continue;
        const auto definition = textSlots.value(slot["name"].toString());
        // 缺失命名槽无法定位文本来源，报告契约与母版不一致。
        if (definition.isEmpty())
            fail("MISSING_SLOT", n, slot["name"].toString());
        auto value = definition[slot["field"].toString()];
        // 按文字槽 GUID 查找实例覆盖；未命中时保留母版文字。
        for (const auto& p : n["props"].toArray()) {
            const auto o = p.toObject();
            // 仅使用目标定义上对应字段的实例覆盖，未提供覆盖时保留母版默认值。
            if (o["componentId"] == definition["guid"] && o.contains(slot["field"].toString()))
                value = o[slot["field"].toString()];
        }
        auto api = slot["api"].toString();
        // 输入框文字需要按内容模式选择 value 或 placeholder 接口，不能统一写成标签。
        if (api == "editorText")
            api = values["editorTextMode"].toString();
        invoke(api, value);
    }
    // 普通属性和文字就绪后应用状态；材质适配最后执行并核对组合支持范围。
    // 按规则顺序应用初始状态，使禁用和 loading 覆盖普通属性初始化。
    for (const auto& s : states[props.value(stateProperty)].toArray())
        invoke(s.toObject()["api"].toString(), s.toObject()["value"]);
    // 按钮材质在所有属性就绪后应用，先核对 type、ghost 与材质组合是否受支持。
    if (button) {
        if (!values["ButtonAppearanceAdapter.setAppearance"].isString())
            fail("INVALID_API_VALUE_TYPE", n, "ButtonAppearanceAdapter.setAppearance");
        const auto appearance = values["ButtonAppearanceAdapter.setAppearance"].toString();
        const auto reason = catalog->buttonUnsupportedReason(appearance, props.value("type"),
                                                             props.value("ghost") == "true");
        // 不支持的组合在适配前终止，保留 catalog 提供的具体原因。
        if (!reason.isEmpty())
            fail("UNSUPPORTED_COMBINATION", n, reason);
        auto* adapter = new ButtonAppearanceAdapter(button, catalog);
        adapter->setAppearance(appearance);
        calls.append(
            QJsonObject{{"api", "ButtonAppearanceAdapter.setAppearance"}, {"value", appearance}});
    }
    records.append(QJsonObject{{"nodeId", n["guid"]},
                               {"componentName", family["name"]},
                               {"qtClass", cls},
                               {"calls", calls}});
    return owned.release();
}
}
