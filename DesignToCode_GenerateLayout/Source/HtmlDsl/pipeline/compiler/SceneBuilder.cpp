// 递归处理 FRAME、INSTANCE 和 PARAGRAPH；未知节点、填色和文本样式显式报错。
#include "SceneBuilder.h"
#include "EngineeringName.h"
#include "WidgetRegistry.h"
#include "Z_Select.h"
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QListView>
#include <QListWidget>
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

QRect staticGeometry(const QJsonObject& node, const QJsonArray& fields) {
    // 静态布局只使用设计 DSL 明确给出的父级相对几何。缺值、NaN、负尺寸
    // 不能被默认化，否则结构预览会把不完整来源伪装成可定位页面。
    if (fields.size() != 4)
        throw std::runtime_error("INVALID_STATIC_LAYOUT_RULE");
    const auto number = [&](const QString& field, bool nonNegative) {
        const auto value = node[field];
        const auto result = value.toDouble();
        if (!value.isDouble() || !std::isfinite(result) ||
            (nonNegative && result < 0) || std::abs(result) > QWIDGETSIZE_MAX)
            throw std::runtime_error((QString("STATIC_GEOMETRY_REQUIRED: ") +
                                      node["guid"].toString() + "/" + field).toStdString());
        return qRound(result);
    };
    return QRect(number(fields[0].toString(), false), number(fields[1].toString(), false),
                 number(fields[2].toString(), true), number(fields[3].toString(), true));
}

void textSlots(const QJsonObject& node, const QString& name, QJsonArray& found) {
    // 槽名与文字字段同时命中才收集，排除同名容器。
    if (node["name"] == name && node.contains("nodeText"))
        found.append(node);
    // 递归母版后代寻找文字槽，覆盖嵌套标签的情况。
    for (const auto& child : node["childNode"].toArray())
        textSlots(child.toObject(), name, found);
}

bool nameMatches(const QJsonObject& family, const QJsonObject& rule,
                 const QString& designComponentNameKey) {
    // 名称必须精确命中登记名单；key 始终保持精确匹配，防止仅凭名称误配组件。
    if (family["componentKey"].toString() != rule["designComponentKey"].toString())
        return false;
    if (family["name"].toString() == rule[designComponentNameKey].toString())
        return true;
    const auto names = rule[designComponentNameKey + "s"].toArray();
    for (const auto& entry : names)
        if (family["name"].toString() == entry.toString())
            return true;
    return false;
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
                           QJsonObject ext, QJsonObject names, QJsonObject libraryProfile,
                           bool layoutOnly, QSet<QString> expanded, QSet<QString> expandedNames)
    : raw(std::move(input)), extensions(std::move(ext)), engineeringNames(std::move(names)),
      libraryProfile(std::move(libraryProfile)), componentRules(std::move(componentRules)),
      components(raw, this->componentRules, this->libraryProfile, layoutOnly),
      staticLayoutRules(layoutRules["structureOnlyStatic"].toObject()),
      layouts(mergeRules(std::move(layoutRules), extensions["layout"].toObject()), raw),
      structureOnly(layoutOnly),
      expandedInstances(std::move(expanded)),
      expandedComponents(std::move(expandedNames)) {
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
    const auto family = components.componentFamily(node);
    // 名称与 key 必须同时匹配扩展规则；其他组件交给通用转换器处理，避免误用选项适配。
    if (!nameMatches(family, rule, "designComponentName"))
        try {
            return components.convert(node, parent);
        } catch (const std::exception& error) {
            if (!structureOnly || (node["mainStateGroup"].toString().isEmpty() &&
                                   node["mainComponent"].toString().isEmpty()))
                throw;
            return components.structureFallback(node, parent,
                                                QString::fromUtf8(error.what()));
        }
    // SelectOption 适配器属于 token-sdk 交付；qt-native 没有对应公开 Qt 等价物，必须失败关闭。
    if (libraryProfile["widgetSource"].toString() == QStringLiteral("qt-builtin-map"))
        throw std::runtime_error("SELECT_OPTION_REQUIRES_TOKEN_SDK");
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
    // Profile 接受契约固定值、当前 SDK 上下文或指向当前上下文的显式别名；其余属性必须完全匹配契约。
    const auto profileAccepted = [&](const QString& value) {
        if (value == expected["Profile"].toString() ||
            value == componentRules["profile"].toString())
            return true;
        const auto alias = componentRules["profileAliases"].toObject()[value].toObject();
        return !alias.isEmpty() &&
               alias["runtimeProfile"].toString() == componentRules["profile"].toString();
    };
    // 逐项要求当前变体匹配已验证契约，拒绝未实现的多选、主题或状态组合。
    for (auto it = expected.begin(); it != expected.end(); ++it)
        // 当前选项适配只实现契约声明的固定变体，值不一致就拒绝生成。
        if (props[it.key()] != it.value() &&
            !(it.key() == "Profile" && profileAccepted(props[it.key()].toString())))
            throw std::runtime_error("UNSUPPORTED_OPTION_VARIANT");
    // 选项文字同样遵循母版槽位及实例覆盖；必须唯一命中，防止读错标签。
    const auto label = optionLabel(node, family);
    extra.append(QJsonObject{{"nodeId", node["guid"]},
                             {"componentName", family["name"]},
                             {"componentKey", family["componentKey"]},
                             {"adapter", rule["qtAdapter"]},
                             {"publicApi", rule["publicApi"]},
                             {"label", label},
                              {"state", props["state"]},
                              {"profile", componentRules["profile"].toString()},
                             {"controlSize", rule["controlSize"]},
                             {"textSource", "master slot + instance props override"}});
    return createSelectOption(extra.last().toObject(), parent);
}

QWidget* SceneBuilder::staticFrame(const QJsonObject& node, QWidget* parent) {
    // layoutMode NONE / 缺失 stackMode 是设计端明确未采用 Auto Layout 的信号。
    // 仅结构模式接受这种合法的绝对定位组合；严格模式继续由 LayoutConverter
    // 对缺少的布局合同失败关闭。
    const auto geometryFields = staticLayoutRules["geometryFields"].toArray();
    if (staticLayoutRules["strategy"].toString() != QStringLiteral("absolute-geometry") ||
        staticLayoutRules["childOrder"].toString() != QStringLiteral("childNode") ||
        geometryFields.size() != 4)
        throw std::runtime_error("INVALID_STATIC_LAYOUT_RULE");
    auto owned = std::make_unique<QWidget>(parent);
    auto* frame = owned.get();
    frame->setProperty("staticLayout", true);
    frame->setProperty("structuralFrame", false);
    const auto background = paint(node);
    if (background.isValid()) {
        auto palette = frame->palette();
        palette.setColor(QPalette::Window, background);
        frame->setPalette(palette);
        frame->setAutoFillBackground(true);
    }
    QJsonArray childIds;
    for (const auto& value : node["childNode"].toArray()) {
        const auto child = value.toObject();
        if (child.isEmpty())
            throw std::runtime_error("STATIC_CHILD_REQUIRED");
        auto* childWidget = create(child, frame);
        childWidget->setGeometry(staticGeometry(child, geometryFields));
        childWidget->setProperty("staticPositioned", true);
        childIds.append(child["guid"]);
    }
    staticLayouts.append(QJsonObject{{"nodeId", node["guid"]},
                                     {"strategy", staticLayoutRules["strategy"]},
                                     {"childOrder", staticLayoutRules["childOrder"]},
                                     {"children", childIds}});
    owned.release();
    return frame;
}

QJsonArray SceneBuilder::optionGroup(const QJsonObject& node) {
    // 仅 qt-native 配置存在聚合合同；其他配置继续走逐实例选项适配。
    if (libraryProfile["widgetSource"].toString() != QStringLiteral("qt-builtin-map"))
        return {};
    const auto children = node["childNode"].toArray();
    if (children.isEmpty())
        return {};
    const auto rule = extensions["selectOption"].toObject();
    // 全部子节点都必须是已登记 SelectOption 实例，任一例外即退回普通容器路径。
    for (const auto& value : children) {
        const auto child = value.toObject();
        if (child["type"] != "INSTANCE")
            return {};
        const auto family =
            find(raw["pixComponentTreeDslNodes"].toArray(), child["mainStateGroup"].toString());
        if (!nameMatches(family, rule, "designComponentName"))
            return {};
    }
    return children;
}

QString SceneBuilder::optionLabel(const QJsonObject& node, const QJsonObject& family) {
    const auto rule = extensions["selectOption"].toObject();
    const auto master = find(family["childNode"].toArray(), node["mainComponent"].toString());
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
    return labelValue.toString();
}

QWidget* SceneBuilder::create(const QJsonObject& node, QWidget* parent) {
    // 每个页面节点只登记一次；FRAME 的子层级由共享布局引擎回调此函数构造。
    const auto id = node["guid"].toString();
    // 空或重复 ID 会破坏页面节点绑定，因此在创建控件前检查。
    if (id.isEmpty() || nodes.contains(id))
        throw std::runtime_error("DUPLICATE_OR_EMPTY_NODE_ID");
    QWidget* w = nullptr;
    // 实例走组件契约，FRAME 递归布局，PARAGRAPH 创建标签；其他类型报错。
    if (node["type"] == "INSTANCE") {
        const auto family = components.componentFamily(node);
        const auto componentName = family["name"].toString();
        const bool requested = expandedInstances.contains(id) ||
            expandedComponents.contains(componentName);
        if (!requested) {
            w = option(node, parent);
        } else {
            const auto master = components.componentMaster(node);
            const auto children = master["childNode"].toArray();
            auto warning = [&](const QString& code, const QString& detail = QString()) {
                QJsonObject item{{"code", code}, {"nodeId", id}, {"component", componentName}};
                if (!detail.isEmpty()) item["detail"] = detail;
                warnings.append(item);
            };
            if (family.isEmpty() || master.isEmpty()) {
                warning("INSTANCE_EXPANSION_MASTER_EMPTY",
                        "ordinary DSL component identity or master is missing");
                w = option(node, parent);
            } else if (children.isEmpty()) {
                warning("INSTANCE_EXPANSION_EMPTY", "ordinary DSL master has no childNode");
                w = option(node, parent);
            } else {
                const auto before = nodes.keys();
                QWidget* staged = nullptr;
                try {
                    // Build the complete ownership tree detached from the page.
                    // A failed tree is destroyed as one unit; no child is deleted
                    // individually while a QLayout still owns it.
                    staged = option(node, nullptr);
                    if (staged->layout())
                        throw std::runtime_error("INSTANCE_EXPANSION_LAYOUT_EXISTS");
                    auto expanded = node;
                    expanded["childNode"] = children;
                    // 同一母版可能被多个实例展开；母版 GUID 不能直接复用，
                    // 否则全局节点登记会把第二个实例误判为重复节点。为每个
                    // 展开实例建立稳定的局部 GUID，同时保留原母版 GUID 作为来源元数据。
                    std::function<void(QJsonObject&)> qualifyIds =
                        [&](QJsonObject& current) {
                            const auto sourceId = current["guid"].toString();
                            if (!sourceId.isEmpty()) {
                                current["sourceDesignNodeId"] = sourceId;
                                current["guid"] = id + "/" + sourceId;
                            }
                            auto nested = current["childNode"].toArray();
                            for (int i = 0; i < nested.size(); ++i) {
                                auto child = nested[i].toObject();
                                qualifyIds(child);
                                nested[i] = child;
                            }
                            current["childNode"] = nested;
                        };
                    auto expandedChildren = expanded["childNode"].toArray();
                    for (int i = 0; i < expandedChildren.size(); ++i) {
                        auto child = expandedChildren[i].toObject();
                        qualifyIds(child);
                        expandedChildren[i] = child;
                    }
                    expanded["childNode"] = expandedChildren;
                    // 实例可能只携带覆盖后的部分布局字段；母版提供缺失
                    // 的 gap/padding 等结构度量，实例字段优先。
                    expanded["autoLayout"] = mergeRules(
                        master["autoLayout"].toObject(), node["autoLayout"].toObject());
                    if (structureOnly) {
                        std::function<void(QJsonObject&)> completeMetrics =
                            [&](QJsonObject& current) {
                                auto metrics = current["autoLayout"].toObject();
                                // 组件母版常不声明页面级 gap/padding；结构模式沿用
                                // LayoutConverter 的预览默认值 0，strict 仍保持显式门禁。
                                for (const auto& field : {"autoLayoutItemSpacing",
                                                          "autoLayoutPaddingLeft",
                                                          "autoLayoutPaddingTop",
                                                          "autoLayoutPaddingRight",
                                                          "autoLayoutPaddingBottom"})
                                    if (!metrics.contains(field)) metrics[field] = 0;
                                // 母版中部分绝对定位/装饰 Frame 没有 stackMode。
                                // structure-only 只需要可承载的结构布局：对含子节点的
                                // 未声明模式采用稳定的纵向默认值；strict 不经过此分支，
                                // 不会把缺失的设计布局伪装成正式合同。
                                if (!metrics.contains("stackMode") &&
                                    (current["type"] == "FRAME" || current["type"] == "SYMBOL"))
                                    metrics["stackMode"] = "VERTICAL";
                                current["autoLayout"] = metrics;
                                auto nested = current["childNode"].toArray();
                                for (int i = 0; i < nested.size(); ++i) {
                                    auto child = nested[i].toObject();
                                    completeMetrics(child);
                                    nested[i] = child;
                                }
                                current["childNode"] = nested;
                            };
                        completeMetrics(expanded);
                    }
                    layouts.convert(expanded, staged, [this](const QJsonObject& child, QWidget* owner) {
                        return create(child, owner);
                    });
                    staged->setProperty("instanceExpanded", true);
                    staged->setProperty("originalInstanceId", id);
                    staged->setProperty("originalComponentId", node["mainStateGroup"].toString());
                    staged->setParent(parent);
                    w = staged;
                } catch (const std::exception& error) {
                    delete staged;
                    for (const auto& key : nodes.keys())
                        if (!before.contains(key)) nodes.remove(key);
                    components.discardTraceForNode(id);
                    warning(QString::fromUtf8(error.what()) == "INSTANCE_EXPANSION_LAYOUT_EXISTS"
                                ? "INSTANCE_EXPANSION_LAYOUT_EXISTS"
                                : "INSTANCE_EXPANSION_FAILED",
                            QString::fromUtf8(error.what()));
                    w = option(node, parent);
                }
            }
        }
    }
    else if (node["type"] == "FRAME") {
        const auto stackMode = node["autoLayout"].toObject()["stackMode"].toString();
        bool configuredStaticMode = false;
        for (const auto& mode : staticLayoutRules["modes"].toArray())
            configuredStaticMode = configuredStaticMode || stackMode == mode.toString();
        const bool staticMissingMode = stackMode.isEmpty() &&
            staticLayoutRules["allowMissingStackMode"].toBool(false);
        if (structureOnly && staticLayoutRules["enabled"].toBool(false) &&
            (configuredStaticMode || staticMissingMode)) {
            w = staticFrame(node, parent);
        } else {
        const auto group = optionGroup(node);
        if (!group.isEmpty()) {
            // qt-native 聚合：父框整体生成为一个 QListWidget，每个选项是禁用 item。
            auto* list = new QListWidget(parent);
            w = list;
            const auto rule = extensions["selectOption"].toObject();
            const auto expected = rule["properties"].toObject();
            const auto profileAccepted = [&](const QString& value) {
                if (value == expected["Profile"].toString() ||
                    value == componentRules["profile"].toString())
                    return true;
                const auto alias =
                    componentRules["profileAliases"].toObject()[value].toObject();
                return !alias.isEmpty() &&
                       alias["runtimeProfile"].toString() == componentRules["profile"].toString();
            };
            QJsonArray items;
            for (const auto& value : group) {
                const auto child = value.toObject();
                const auto family = find(raw["pixComponentTreeDslNodes"].toArray(),
                                         child["mainStateGroup"].toString());
                const auto master =
                    find(family["childNode"].toArray(), child["mainComponent"].toString());
                if (master["componentKey"] != child["componentKey"])
                    throw std::runtime_error("INVALID_OPTION_MASTER_KEY");
                QJsonObject props;
                for (const auto& variant : child["propRefMap"].toObject()["variant"].toArray()) {
                    const auto entry = variant.toObject();
                    const auto name = entry["name"].toString();
                    if (props.contains(name))
                        throw std::runtime_error("DUPLICATE_OPTION_PROPERTY");
                    props[name] = entry["value"];
                }
                for (auto it = expected.begin(); it != expected.end(); ++it)
                    if (props[it.key()] != it.value() &&
                        !(it.key() == "Profile" &&
                          profileAccepted(props[it.key()].toString())))
                        throw std::runtime_error("UNSUPPORTED_OPTION_VARIANT");
                const auto label = optionLabel(child, family);
                auto* item = new QListWidgetItem(label, list);
                // 选项契约固定 disabled：去掉 ItemIsEnabled，其余保留只读呈现位。
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
                items.append(QJsonObject{{"nodeId", child["guid"]},
                                         {"label", label},
                                         {"enabled", false}});
            }
            const double dw = node["width"].toDouble();
            const double dh = node["height"].toDouble();
            // 聚合控件沿用设计实测几何，避免默认 sizeHint 改变页面占位。
            if (std::isfinite(dw) && std::isfinite(dh) && dw > 0 && dh > 0) {
                w->setMinimumSize(int(std::ceil(dw)), int(std::ceil(dh)));
                w->setMaximumSize(int(std::ceil(dw)), int(std::ceil(dh)));
            }
            optionGroups.append(QJsonObject{
                {"nodeId", node["guid"]},
                {"componentName", rule["designComponentName"]},
                {"componentKey", rule["designComponentKey"]},
                {"qtClass", "QListWidget"},
                {"state", "disabled"},
                {"profile", componentRules["profile"].toString()},
                {"items", items}});
        } else {
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
        }
        }
    } else if (node["type"] == "PARAGRAPH") {
        if (!node["nodeText"].isString())
            throw std::runtime_error("INVALID_TEXT_VALUE");
        auto* label = new QLabel(node["nodeText"].toString(), parent);
        w = label;
        label->setTextFormat(Qt::PlainText);
        if (!structureOnly) {
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
            // 普通文字生成局部 QSS；转义字体名，避免引号或换行破坏样式语法。
            auto family = node["fontFamily"].toString();
            family.replace('\\', "\\\\").replace('"', "\\\"").replace('\n', ' ').replace('\r', ' ');
            label->setStyleSheet(QString("QLabel { font-family: \"%1\"; font-size: %2px; color: %3; "
                                         "background: transparent; }")
                                     .arg(family)
                                     .arg(qRound(size))
                                     .arg(color.name(QColor::HexArgb)));
        }
    } else
        throw std::runtime_error(("UNMAPPED_NODE_TYPE: " + node["type"].toString()).toStdString());
    QString name;
    if (structureOnly) {
        // 结构模式直接使用 DSL 设计节点名作为 objectName；重名追加序号，空名回退 GUID。
        name = node["name"].toString().trimmed();
        if (name.isEmpty()) name = "node_" + id;
        if (usedNames.contains(name)) {
            int index = 2;
            QString candidate;
            do {
                candidate = name + "_" + QString::number(index++);
            } while (usedNames.contains(candidate));
            name = candidate;
        }
        usedNames.insert(name);
    } else {
        name = engineeringName(node["name"].toString(), engineeringNames[id].toString());
        if (usedNames.contains(name))
            throw std::runtime_error("DUPLICATE_ENGINEERING_NAME");
        usedNames.insert(name);
    }
    w->setObjectName(name);
    w->setProperty("designNodeId", id);
    if (node.contains("sourceDesignNodeId"))
        w->setProperty("sourceDesignNodeId", node["sourceDesignNodeId"].toString());
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
            {"optionGroups", optionGroups},
            {"staticLayouts", staticLayouts},
            {"layouts", layouts.trace()},
            {"geometry", geometry},
            {"warnings", warnings}};
}
}
