// 递归处理 FRAME、INSTANCE 和 PARAGRAPH；未知节点、填色和文本样式显式报错。
#include "SceneBuilder.h"
#include "EngineeringName.h"
#include "WidgetRegistry.h"
#include "Z_Select.h"
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QListView>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <cmath>
#include <stdexcept>

namespace v2 {
namespace {
QJsonObject find(const QJsonArray& entries, const QString& id) {
    // 按 GUID 查找直接引用，找不到时抛错，防止使用空母版继续解析。
    for (const auto& v : entries)
        // 仅返回 GUID 相同的节点，确保后续读取的是实例实际引用的定义。
        if (v.toObject()["guid"] == id)
            return v.toObject();
    throw std::runtime_error(("REFERENCE_MISSING: " + id).toStdString());
}

// 当前普通节点填色限定为单个 SOLID，透明度由 0~1 换算到 QColor 的 0~255。
QColor paint(const QJsonObject& node) {
    if (node.contains("fillPaints") && !node["fillPaints"].isArray())
        throw std::runtime_error("INVALID_NODE_PAINT_TYPE");
    const auto paints = node["fillPaints"].toArray();
    // 缺少填色时保留无背景结果，让后续绘制沿用控件默认行为。
    if (paints.isEmpty())
        return {};
    // 当前只支持单一纯色，拒绝渐变或多层填色，避免生成近似外观。
    if (paints.size() != 1 || paints.first().toObject()["type"] != "SOLID")
        throw std::runtime_error("UNSUPPORTED_NODE_PAINT");
    const auto c = paints.first().toObject()["color"].toObject();
    // Pixso 当前原始色通道采用 0..255 整数；缺值、字符串和未解析引用不得退化成黑色。
    for (const auto* channel : {"r", "g", "b"}) {
        const auto value = c[channel];
        const double number = value.toDouble();
        if (!value.isDouble() || !std::isfinite(number) || number < 0 || number > 255 ||
            number != std::floor(number))
            throw std::runtime_error((QString("INVALID_COLOR_CHANNEL: ") + channel).toStdString());
    }
    if (c.contains("a") && (!c["a"].isDouble() || !std::isfinite(c["a"].toDouble()) ||
                            c["a"].toDouble() < 0 || c["a"].toDouble() > 1))
        throw std::runtime_error("INVALID_COLOR_CHANNEL: a");
    return QColor(c["r"].toInt(), c["g"].toInt(), c["b"].toInt(), qRound(c["a"].toDouble(1) * 255));
}

void textSlots(const QJsonObject& node, const QString& name, QJsonArray& found) {
    // 槽名与文字字段同时命中才收集，排除同名容器。
    if (node["name"] == name && node.contains("nodeText"))
        found.append(node);
    // 递归母版后代寻找文字槽，覆盖嵌套标签的情况。
    for (const auto& child : node["childNode"].toArray())
        textSlots(child.toObject(), name, found);
}
}

QJsonObject mergeRules(QJsonObject base, const QJsonObject& extension) {
    // 扩展对象逐层覆盖基础规则；对象递归合并，标量与数组整体替换。
    for (auto it = extension.begin(); it != extension.end(); ++it)
        base[it.key()] =
            it.value().isObject()
                ? QJsonValue(mergeRules(base[it.key()].toObject(), it.value().toObject()))
                : it.value();
    return base;
}

SceneBuilder::SceneBuilder(QJsonObject input, QJsonObject componentRules, QJsonObject layoutRules,
                           QJsonObject ext, QJsonObject names)
    : raw(std::move(input)), extensions(std::move(ext)), engineeringNames(std::move(names)),
      components(raw, std::move(componentRules)),
      layouts(mergeRules(std::move(layoutRules), extensions["layout"].toObject()), raw) {
    // 一次生成仅对应一个根页面，多个根无法共享同一输出前缀。
    if (raw["pixTreeNodes"].toArray().size() != 1)
        throw std::runtime_error("EXPECTED_ONE_ROOT");
}

QWidget* SceneBuilder::build(QWidget* parent) {
    // 同一构建器只使用一次，防止二次 build 复用残留节点和调用轨迹。
    if (!nodes.isEmpty())
        throw std::runtime_error("SCENE_ALREADY_BUILT");
    auto* page = create(root(), parent);
    // 嵌套 Hug 容器可随内容收缩，用 Maximum 避免吞掉父容器剩余空间。
    for (auto* w : nodes)
        // 只收束布局容器的 Hug 策略，保留原生组件内部的尺寸行为。
        if (w->property("designType") == "FRAME") {
            auto policy = w->sizePolicy();
            // 仅把原本的 Hug/Preferred 改为 Maximum，已明确的 Fixed/Fill 不变。
            if (policy.horizontalPolicy() == QSizePolicy::Preferred)
                policy.setHorizontalPolicy(QSizePolicy::Maximum);
            // 纵向同样限制 Hug 容器吞占剩余空间，保留内容决定的高度。
            if (policy.verticalPolicy() == QSizePolicy::Preferred)
                policy.setVerticalPolicy(QSizePolicy::Maximum);
            w->setSizePolicy(policy);
        }
    page->resize(qRound(root()["width"].toDouble()), qRound(root()["height"].toDouble()));
    page->setMinimumSize(page->size());
    return page;
}

QWidget* SceneBuilder::option(const QJsonObject& node, QWidget* parent) {
    // 仅扩展规则指定的组件集进入选项适配，其余实例继续走通用组件映射。
    const auto rule = extensions["selectOption"].toObject();
    const auto family =
        find(raw["pixComponentTreeDslNodes"].toArray(), node["mainStateGroup"].toString());
    // 名称与 key 必须同时匹配扩展规则；其他组件交给通用转换器处理，避免误用选项适配。
    if (family["name"] != rule["designComponentName"] ||
        family["componentKey"] != rule["designComponentKey"])
        return components.convert(node, parent);
    const auto master = find(family["childNode"].toArray(), node["mainComponent"].toString());
    // 实例 key 必须匹配已定位母版，避免把另一个组件的属性用于选项适配。
    if (master["componentKey"] != node["componentKey"])
        throw std::runtime_error("INVALID_OPTION_MASTER_KEY");
    QJsonObject props;
    // 收集选项变体并检查重复与未知字段，避免未支持状态进入适配器。
    for (const auto& v : node["propRefMap"].toObject()["variant"].toArray()) {
        const auto p = v.toObject();
        const auto name = p["name"].toString();
        // 同名属性有歧义，拒绝后值覆盖前值。
        if (props.contains(name))
            throw std::runtime_error("DUPLICATE_OPTION_PROPERTY");
        // label 由文字槽解析，其余属性必须列入选项契约。
        if (name != "label" && !rule["properties"].toObject().contains(name))
            throw std::runtime_error("UNMAPPED_OPTION_PROPERTY");
        props[name] = p["value"];
    }
    const auto expected = rule["properties"].toObject();
    // 逐项要求当前变体匹配已验证契约，拒绝未实现的多选、主题或状态组合。
    for (auto it = expected.begin(); it != expected.end(); ++it)
        // 当前选项适配只实现契约声明的固定变体，值不一致就拒绝生成。
        if (props[it.key()] != it.value())
            throw std::runtime_error("UNSUPPORTED_OPTION_VARIANT");
    // 选项文字同样遵循母版槽位及实例覆盖；必须唯一命中，防止读错标签。
    QJsonArray found;
    textSlots(master, rule["textSlot"].toString(), found);
    // 文字槽必须唯一，缺失或多次命中都无法确定选项标签。
    if (found.size() != 1)
        throw std::runtime_error("OPTION_TEXT_SLOT_AMBIGUOUS");
    QJsonValue labelValue = found.first().toObject()["nodeText"];
    // 只应用命中文字槽 GUID 的实例覆盖，未覆盖时保留母版标签。
    for (const auto& v : node["props"].toArray()) {
        const auto p = v.toObject();
        // 只有命中文字槽且明确提供文本的覆盖记录才替换标签，其他记录保持原有用途。
        if (p["componentId"] == found.first().toObject()["guid"] && p.contains("nodeText"))
            labelValue = p["nodeText"];
    }
    if (!labelValue.isString())
        throw std::runtime_error("INVALID_TEXT_VALUE");
    const auto label = labelValue.toString();
    extra.append(QJsonObject{{"nodeId", node["guid"]},
                             {"componentName", family["name"]},
                             {"componentKey", family["componentKey"]},
                             {"adapter", rule["qtAdapter"]},
                             {"publicApi", rule["publicApi"]},
                             {"label", label},
                             {"state", props["state"]},
                             {"profile", props["Profile"]},
                             {"controlSize", rule["controlSize"]},
                             {"textSource", "master slot + instance props override"}});
    return createSelectOption(extra.last().toObject(), parent);
}

QWidget* SceneBuilder::create(const QJsonObject& node, QWidget* parent) {
    // 每个页面节点只登记一次；FRAME 的子层级由共享布局引擎回调此函数构造。
    const auto id = node["guid"].toString();
    // 空或重复 ID 会破坏页面节点绑定，因此在创建控件前检查。
    if (id.isEmpty() || nodes.contains(id))
        throw std::runtime_error("DUPLICATE_OR_EMPTY_NODE_ID");
    QWidget* w = nullptr;
    // 实例走组件契约，FRAME 递归布局，PARAGRAPH 创建标签；其他类型报错。
    if (node["type"] == "INSTANCE")
        w = option(node, parent);
    else if (node["type"] == "FRAME") {
        auto owned = std::make_unique<QWidget>(parent);
        w = owned.get();
        const auto sizing = node["autoLayout"].toObject();
        const bool structural = parent && node["fillPaints"].toArray().isEmpty() &&
            node["strokePaints"].toArray().isEmpty() &&
            node["effects"].toArray().isEmpty() &&
            sizing["stackChildPrimarySizing"].toString("FIXED") == "FIXED" &&
            sizing["stackChildCounterSizing"].toString("FIXED") == "FIXED" &&
            sizing["autoLayoutWidthResize"].toInt(-1) == 1 &&
            sizing["autoLayoutHeightResize"].toInt(-1) == 1 &&
            node["visible"].toBool(true);
        w->setProperty("structuralFrame", structural);
        const auto bg = paint(node);
        // 只有显式有效纯色才安装背景，避免把缺少填色的容器强制涂黑。
        if (bg.isValid()) {
            auto p = w->palette();
            p.setColor(QPalette::Window, bg);
            w->setPalette(p);
            w->setAutoFillBackground(true);
        }
        layouts.convert(node, w, [this](const QJsonObject& child, QWidget* owner) {
            return create(child, owner);
        });
        owned.release();
    } else if (node["type"] == "PARAGRAPH") {
        if (!node["nodeText"].isString())
            throw std::runtime_error("INVALID_TEXT_VALUE");
        const auto color = paint(node);
        if (!color.isValid())
            throw std::runtime_error("MISSING_TEXT_COLOR");
        // 普通文字目前限定 Regular/400，其他字重需要补充映射再生成。
        if (node["fontStyle"] != "Regular" || node["fontWeight"].toInt() != 400)
            throw std::runtime_error("UNSUPPORTED_TEXT_STYLE");
        const double size = node["fontSize"].toDouble();
        // 限制字体为合理正数，防止非法 QSS 或异常超大标签破坏几何。
        if (!std::isfinite(size) || size <= 0 || size > 4096)
            throw std::runtime_error("INVALID_TEXT_SIZE");
        auto* label = new QLabel(node["nodeText"].toString(), parent);
        w = label;
        label->setTextFormat(Qt::PlainText);
        // 普通文字生成局部 QSS；转义字体名，避免引号或换行破坏样式语法。
        auto family = node["fontFamily"].toString();
        family.replace('\\', "\\\\").replace('"', "\\\"").replace('\n', ' ').replace('\r', ' ');
        label->setStyleSheet(QString("QLabel { font-family: \"%1\"; font-size: %2px; color: %3; "
                                     "background: transparent; }")
                                 .arg(family)
                                 .arg(qRound(size))
                                 .arg(color.name(QColor::HexArgb)));
    } else
        throw std::runtime_error(("UNMAPPED_NODE_TYPE: " + node["type"].toString()).toStdString());
    const QString name = engineeringName(node["name"].toString(), engineeringNames[id].toString());
    if (usedNames.contains(name))
        throw std::runtime_error("DUPLICATE_ENGINEERING_NAME");
    usedNames.insert(name);
    w->setObjectName(name);
    w->setProperty("designNodeId", id);
    w->setProperty("designName", node["name"].toString());
    w->setProperty("designType", node["type"].toString());
    nodes[id] = w;
    return w;
}

QJsonObject SceneBuilder::trace() const {
    // 此处为构建期 Qt 对象的测量结果；原始设计关系由 DesignTree 独立投影。
    QJsonArray geometry;
    // 逐节点保存 Qt 求得的几何与策略，用于对照设计结果而非替代原始设计树。
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        const auto r = it.value()->geometry();
        geometry.append(
            QJsonObject{{"nodeId", it.key()},
                        {"qtClass", it.value()->metaObject()->className()},
                        {"geometry", QJsonArray{r.x(), r.y(), r.width(), r.height()}},
                        {"enabled", it.value()->isEnabled()},
                        {"sizeHint", QJsonArray{it.value()->sizeHint().width(),
                                                it.value()->sizeHint().height()}},
                        {"horizontalPolicy", int(it.value()->sizePolicy().horizontalPolicy())},
                        {"verticalPolicy", int(it.value()->sizePolicy().verticalPolicy())}});
    }
    return {{"components", components.trace()},
            {"adapters", extra},
            {"layouts", layouts.trace()},
            {"geometry", geometry}};
}
}
