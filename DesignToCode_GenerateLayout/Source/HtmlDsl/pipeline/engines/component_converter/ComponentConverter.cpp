// 把组件身份、变体属性、文字槽和资源槽转换为受控的 Qt API 调用。
#include "ComponentConverter.h"
#include "Z_PushButton.h"
#include "Z_CheckBox.h"
#include "Z_InputField.h"
#include "ButtonAppearanceAdapter.h"
#include <QJsonDocument>
#include <QMap>
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
      fields(std::make_shared<field::Tokens>(mapping["profile"].toString())) {}

QWidget* ComponentConverter::convert(const QJsonObject& n, QWidget* parent) {
    // 用组件集 key 和名称双重匹配规则，再确认实例指向的母版 key。
    const auto family =
        find(raw["pixComponentTreeDslNodes"].toArray(), n["mainStateGroup"].toString());
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
        // 两个身份字段同时匹配才接受，防止同名但来自另一组件集的实例被套用规则。
        if (candidate["designComponentKey"] == family["componentKey"] &&
            candidate["designComponentName"] == family["name"]) {
            // 已有命中规则时再次匹配表示配置有歧义，拒绝按遍历顺序任意选取。
            if (!rule.isEmpty())
                fail("AMBIGUOUS_COMPONENT_MAPPING", n, family["name"].toString());
            rule = candidate;
        }
    }
    // 没有契约时立即报告未映射，避免使用默认按钮掩盖设计侧缺项。
    if (rule.isEmpty())
        fail("UNMAPPED_COMPONENT", n, family["name"].toString());
    const auto master = find(family["childNode"].toArray(), n["mainComponent"].toString());
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
    if (props.value("Profile") != mapping["profile"].toString())
        fail("PROFILE_MISMATCH", n, props.value("Profile"));
    const auto propertyRules = rule["propertyMappings"].toObject();
    // 逐项核对契约必填属性，缺少数据时在创建 QWidget 前终止。
    for (const auto& required : propertyRules.keys())
        // 每个必填映射属性都要有设计值，缺失时报告错误，防止默认值掩盖不完整设计。
        if (!props.contains(required))
            fail("MISSING_PROPERTY", n, required);
    QJsonObject values;
    // 将一般变体翻译成 API 参数；Profile 和 state 分别由主题检查及状态规则处理。
    for (auto it = props.cbegin(); it != props.cend(); ++it) {
        // 跳过已分配给专门流程的字段，避免将状态或主题误当通用 setter。
        if (it.key() == "Profile" || it.key() == "state")
            continue;
        const auto spec = propertyRules[it.key()].toObject();
        const auto domain = spec["values"].toObject();
        // 只接受契约列出的值，防止拼写错误被转换为空参数。
        if (!domain.contains(it.value()))
            fail("UNMAPPED_PROPERTY", n, it.key() + "=" + it.value());
        values[spec["api"].toString()] = domain[it.value()];
    }
    const auto states = rule["stateMappings"].toObject();
    // 只有明确映射的初始状态可以生成，未知状态不降级为 default。
    if (!states.contains(props.value("state")))
        fail("UNMAPPED_STATE", n, props.value("state"));
    const auto cls = rule["qtClass"].toString();
    // 转换期间由 unique_ptr 托管，后续任何 setter 失败都会释放本次控件。
    std::unique_ptr<QWidget> owned;
    using namespace commonWidget;
    Z_PushButton* button = nullptr;
    Z_CheckBox* check = nullptr;
    Z_InputField* input = nullptr;
    // 按类白名单创建已有组件，分别注入其所需 Token 服务；未知类终止生成。
    if (cls == "commonWidget::Z_PushButton")
        owned.reset(button = new Z_PushButton(parent));
    else if (cls == "commonWidget::Z_CheckBox")
        owned.reset(check = new Z_CheckBox(catalog, {}, parent));
    else if (cls == "commonWidget::Z_InputField")
        owned.reset(input = new Z_InputField(fields, parent));
    else
        fail("UNREGISTERED_QT_CLASS", n, cls);
    auto* w = owned.get();
    w->setObjectName("node_" + n["guid"].toString().replace(':', '_'));
    w->setProperty("designNodeId", n["guid"].toString());
    w->setProperty("designComponent", family["name"].toString());
    QJsonArray calls;
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
            const auto source = resource["source"].toString();
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
    // API 白名单：映射表只能调用此处已实现的接口，未知调用立即终止。
    const auto invoke = [&](const QString& api, const QJsonValue& value) {
        // 禁止 QJsonValue 的默认转换掩盖错误：字符串 true 不是布尔值，数字也不是文字。
        static const QSet<QString> booleanApis{"setEnabled", "setDanger", "setGhost",
                                               "setLoading", "setAllowClear", "setReadOnly"};
        if (booleanApis.contains(api) ? !value.isBool() : !value.isString())
            fail("INVALID_API_VALUE_TYPE", n, api);
        const auto text = value.toString();
        // 按钮通过自己的 setEnabled 保存请求状态，保证材质刷新后仍保留禁用态。
        // 按 API 名和控件类型联合分派；同名 setter 仅在对应控件上执行，末尾拒绝未注册组合。
        if (api == "setEnabled" && button)
            button->setEnabled(value.toBool());
        else if (api == "setEnabled")
            w->setEnabled(value.toBool());
        else if (api == "setText" && button)
            button->setText(text);
        else if (api == "setText" && check)
            check->setText(text);
        else if (api == "setType" && button) {
            const QMap<QString, ButtonType> types{{"Primary", ButtonType::Primary},
                                                  {"Default", ButtonType::Default},
                                                  {"Dashed", ButtonType::Dashed},
                                                  {"Text", ButtonType::Text},
                                                  {"Link", ButtonType::Link}};
            // 映射字符串须命中 ButtonType 枚举，避免 QMap 缺省值伪装为合法类型。
            if (!types.contains(text))
                fail("INVALID_ENUM", n, text);
            button->setType(types[text]);
        } else if (api == "setSize" && button) {
            const QMap<QString, ButtonSize> sizes{{"XS", ButtonSize::XS},
                                                  {"S", ButtonSize::S},
                                                  {"M", ButtonSize::M},
                                                  {"L", ButtonSize::L}};
            // 仅接受已声明尺寸枚举，避免未知尺寸静默落入默认项。
            if (!sizes.contains(text))
                fail("INVALID_ENUM", n, text);
            button->setSize(sizes[text]);
        } else if (api == "setDanger" && button)
            button->setDanger(value.toBool());
        else if (api == "setGhost" && button)
            button->setGhost(value.toBool());
        else if (api == "setLoading" && button)
            button->setLoading(value.toBool());
        else if (api == "setLoading" && input)
            input->setLoading(value.toBool());
        else if (api == "setControlSize" && check)
            check->setControlSize(text);
        else if (api == "setControlSize" && input)
            input->setControlSize(text);
        else if (api == "setAppearance" && check)
            check->setAppearance(text);
        else if (api == "setAppearance" && input)
            input->setAppearance(text);
        else if (api == "setCheckValue" && check) {
            if (text != "unchecked" && text != "checked" && text != "mixed")
                fail("INVALID_ENUM", n, api + "=" + text);
            check->setChecked(text == "checked");
            check->setIndeterminate(text == "mixed");
        } else if (api == "setKind" && input) {
            const QMap<QString, Z_InputField::Kind> kinds{
                {"Text", Z_InputField::Kind::Text},
                {"Password", Z_InputField::Kind::Password},
                {"Search", Z_InputField::Kind::Search},
                {"TextArea", Z_InputField::Kind::TextArea}};
            // 输入框种类须对应已实现的编辑行为，未知值拒绝生成。
            if (!kinds.contains(text))
                fail("INVALID_ENUM", n, text);
            input->setKind(kinds[text]);
        } else if (api == "setVariant" && input)
            input->setVariant(text);
        else if (api == "setAllowClear" && input)
            input->setAllowClear(value.toBool());
        else if (api == "setReadOnly" && input)
            input->setReadOnly(value.toBool());
        else if (api == "setStatus" && input)
            input->setStatus(text);
        else if (api == "setLabel" && input)
            input->setLabel(text);
        else if (api == "setHint" && input)
            input->setHint(text);
        else if (api == "setPlaceholder" && input)
            input->setPlaceholder(text);
        else if (api == "setValue" && input)
            input->setValue(text);
        else if (api == "setIconStyles" && button) {
            const QMap<QString, IconContainerStyle> styles{
                {"Bare", IconContainerStyle::Bare},
                {"Filled", IconContainerStyle::Filled},
                {"Outlined", IconContainerStyle::Outlined}};
            // 图标容器样式限定为契约枚举，保证前后图标使用同一合法样式。
            if (!styles.contains(text))
                fail("INVALID_ENUM", n, text);
            button->setLeadingIconStyle(styles[text]);
            button->setTrailingIconStyle(styles[text]);
        } else if (api == "setContentMode" && button) {
            if (!QSet<QString>{"Text", "Leading", "Trailing", "Dual", "IconOnly"}.contains(text))
                fail("INVALID_ENUM", n, api + "=" + text);
            button->setIconOnly(false);
            button->setLeadingIconVisible(text == "Leading" || text == "Dual" ||
                                          text == "IconOnly");
            button->setTrailingIconVisible(text == "Trailing" || text == "Dual");
            button->setIconOnly(text == "IconOnly");
        } else if (api == "prefixPolicy" && input && text == "none") {
        } else if (api == "editorTextMode" && input) {
            if (text != "setPlaceholder" && text != "setValue")
                fail("INVALID_ENUM", n, api + "=" + text);
        } else
            fail("UNREGISTERED_API", n, api);
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
    for (const auto& s : states[props.value("state")].toArray())
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
