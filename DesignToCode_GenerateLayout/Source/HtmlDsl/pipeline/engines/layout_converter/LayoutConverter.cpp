// 尺寸求值在此集中完成，组件转换器无需重复解释布局字段。
#include "LayoutConverter.h"
#include <QJsonDocument>
#include <QtMath>
#include <stdexcept>
#include <cmath>

namespace d2c {
namespace {
// 缺失值走指定默认值；显式值须为有限、非负的数值，最终舍入为 Qt 整数像素。
int metric(const QJsonObject& o, const QString& key, int fallback = 0) {
    // 字段缺失才使用默认值；显式非法数值由下一项检查拒绝。
    if (!o.contains(key))
        return fallback;
    // 限制为 Qt 可表达的有限非负尺寸，避免溢出、NaN 或负间距进入布局。
    if (!o[key].isDouble() || o[key].toDouble() < 0 || !qIsFinite(o[key].toDouble()) ||
        o[key].toDouble() > QWIDGETSIZE_MAX)
        throw std::runtime_error(("INVALID_LAYOUT_METRIC: " + key).toStdString());
    return qRound(o[key].toDouble());
}
}

QBoxLayout* LayoutConverter::convert(const QJsonObject& node, QWidget* owner,
                                     const Factory& factory) {
    // 规则表限定布局方向与字段名；padding 顺序固定为左、上、右、下。
    const auto a = node["autoLayout"].toObject();
    const auto mode = a["stackMode"].toString();
    const auto target = mapping["modes"].toObject()[mode].toString();
    // 只实例化规则明确映射的横向或纵向布局，未知方向无法稳定还原。
    if (target != "QHBoxLayout" && target != "QVBoxLayout")
        throw std::runtime_error("UNMAPPED_LAYOUT_MODE");
    // 同一个 QWidget 只能由一个顶层布局管理，提前拒绝重复安装。
    if (owner->layout())
        throw std::runtime_error("OWNER_ALREADY_HAS_LAYOUT");
    // 正式生成要求五个布局数值明确存在；先校验再安装布局，失败不留下半成品。
    const auto pads = mapping["paddingFields"].toArray();
    if (pads.size() != 4)
        throw std::runtime_error("INVALID_PADDING_RULE");
    QStringList metricFields{mapping["gapField"].toString()};
    for (const auto& field : pads)
        metricFields.append(field.toString());
    QJsonArray defaultedFields;
    for (const auto& field : metricFields) {
        if (field.isEmpty())
            throw std::runtime_error("INVALID_LAYOUT_FIELD_RULE");
        if (!a.contains(field)) {
            if (mapping["requireExplicitMetrics"].toBool())
                throw std::runtime_error(("MISSING_LAYOUT_METRIC: " + field).toStdString());
            // 旧预览保留原行为，但缺省位置进入轨迹，不再伪装为显式设计零值。
            defaultedFields.append(field);
        }
        metric(a, field);
    }
    const bool horizontal = target == "QHBoxLayout";
    auto* layout = horizontal ? static_cast<QBoxLayout*>(new QHBoxLayout(owner))
                              : static_cast<QBoxLayout*>(new QVBoxLayout(owner));
    // padding 必须按左、上、右、下四项取值，数量不符表示规则本身有误。
    if (pads.size() != 4)
        throw std::runtime_error("INVALID_PADDING_RULE");
    layout->setContentsMargins(metric(a, pads[0].toString()), metric(a, pads[1].toString()),
                               metric(a, pads[2].toString()), metric(a, pads[3].toString()));
    layout->setSpacing(metric(a, mapping["gapField"].toString()));
    // 将设计侧主轴/交叉轴对齐统一为 start、center、end，再转换成 Qt 标志。
    const auto align = [&](const QString& field) {
        const auto value =
            a.contains(field)
                ? mapping["alignmentValues"].toObject()[a[field].toString()].toString()
                : mapping["missingAlignment"].toString();
        // 只接受已实现的三种对齐语义，避免分布式对齐被错误降级。
        if (value != "start" && value != "center" && value != "end")
            throw std::runtime_error("UNMAPPED_ALIGNMENT");
        return value;
    };
    const auto primary = align(mapping["primaryAlignmentField"].toString());
    const auto counter = align(mapping["counterAlignmentField"].toString());
    const auto axis = [&](const QString& value, bool x) -> Qt::Alignment {
        // 按物理轴选择 Qt 对齐标志，避免把水平居中用于纵向轴。
        if (x)
            return value == "center" ? Qt::AlignHCenter
                   : value == "end"  ? Qt::AlignRight
                                     : Qt::AlignLeft;
        return value == "center" ? Qt::AlignVCenter
               : value == "end"  ? Qt::AlignBottom
                                 : Qt::AlignTop;
    };
    bool hasCrossFill = false;
    auto children = node["childNode"].toArray();
    QJsonArray order;
    // 生成顺序必须有明确契约；当前转换器只实现 reverse-childNode。
    if (mapping["childOrder"] != "reverse-childNode")
        throw std::runtime_error("UNMAPPED_CHILD_ORDER");
    // 当前输入约定 childNode 与视觉顺序相反，按规则逆序加入布局。
    for (int index = children.size() - 1; index >= 0; --index) {
        const auto child = children[index].toObject();
        QJsonObject ca;
        const auto merge = [&](const QJsonObject& fields) {
            // 逐字段覆盖已解析布局，后调用的实例值优先于母版默认值。
            for (auto it = fields.begin(); it != fields.end(); ++it)
                ca[it.key()] = it.value();
        };
        // 尺寸字段优先级：母版默认值 → 实例 autoLayout → 指向母版的 props 覆盖。
        if (child["type"] == "INSTANCE" && !raw.isEmpty()) {
            // 先定位所属组件集，避免在另一集合中解析到同名母版。
            for (const auto& set : raw["pixComponentTreeDslNodes"].toArray()) {
                // 跳过其他组件集，只从实例声明的集合中继承尺寸。
                if (set.toObject()["guid"] != child["mainStateGroup"])
                    continue;
                // 在匹配集合中找到母版，取得实例尚未覆盖的布局默认值。
                for (const auto& master : set.toObject()["childNode"].toArray())
                    // 按母版 GUID 精确命中定义，实例的尺寸继承必须以正确母版为基础。
                    if (master.toObject()["guid"] == child["mainComponent"])
                        merge(master.toObject()["autoLayout"].toObject());
            }
        }
        merge(child["autoLayout"].toObject());
        // 只合并指向母版根的覆盖；内部标签或图标的布局不决定实例外部空间。
        for (const auto& value : child["props"].toArray()) {
            const auto override = value.toObject();
            // 过滤子部件覆盖，保持父布局尺寸来源为组件根。
            if (override["componentId"] != child["mainComponent"])
                continue;
            merge(override["autoLayout"].toObject());
            // 根覆盖可能把尺寸字段直接放在顶层，逐项补入统一布局对象。
            for (const auto& key : {"stackMode", "stackPrimarySizing", "stackCounterSizing",
                                    "stackChildPrimarySizing", "stackChildCounterSizing",
                                    "autoLayoutWidthResize", "autoLayoutHeightResize"})
                // 只复制覆盖记录中明确出现的字段，缺省字段继续使用已合并的母版值。
                if (override.contains(key))
                    ca[key] = override[key];
        }
        // 绝对定位不属于当前自布局实现，拒绝后可避免错误参与排布。
        if (ca["autoLayoutItemAbsolutePos"].toBool())
            throw std::runtime_error("UNMAPPED_ABSOLUTE_CHILD");
        QWidget* widget = factory(child, owner);
        // 工厂必须返回有效控件，避免后续尺寸设置访问空指针。
        if (!widget)
            throw std::runtime_error("NULL_COMPONENT_FACTORY_RESULT");
        // 父布局分配几何空间；子控件工厂负责内容和组件内部结构。
        const auto childMode = ca["stackMode"].toString();
        bool childCrossFill = false;
        bool childMainFill = false;
        // 先判断相对父容器的交叉轴 Fill，再处理宽高 resize 和子节点自身主/交叉轴。
        // 当前主轴只接受 FIXED；未验证的模式显式失败，避免隐式拉伸。
        const auto sizing = [&](bool width) {
            if (width == horizontal) {
                const auto value = mapping["parentPrimarySizingModes"].toObject()
                                       [ca["stackChildPrimarySizing"].toString("FIXED")].toString();
                if (value.isEmpty())
                    throw std::runtime_error("UNMAPPED_PARENT_PRIMARY_SIZE");
                if (value != "self")
                    return value;
            }
            // 宽高与父布局交叉轴一致时优先解释父级 Fill，覆盖子控件自身尺寸模式。
            if (width != horizontal) {
                const auto value = mapping["parentCounterSizingModes"]
                                       .toObject()[ca["stackChildCounterSizing"].toString("FIXED")]
                                       .toString();
                // 交叉轴模式必须存在映射，否则无法决定保留自身尺寸还是拉伸。
                if (value.isEmpty())
                    throw std::runtime_error("UNMAPPED_PARENT_COUNTER_SIZE");
                // 父级明确要求拉伸时直接采用，self 才继续解析子节点自身设置。
                if (value != "self")
                    return value;
            }
            const auto resizeKey = width ? "autoLayoutWidthResize" : "autoLayoutHeightResize";
            // 显式 resize 枚举优先于节点自身主/交叉轴默认值。
            if (ca.contains(resizeKey)) {
                const auto number = ca[resizeKey].toDouble(-1);
                // resize 是离散枚举，拒绝非数值或小数以免截断后误命中规则。
                if (!ca[resizeKey].isDouble() || number != std::floor(number))
                    throw std::runtime_error("UNVERIFIED_RESIZE_MODE");
                const auto resize = mapping["resizeModes"]
                                        .toObject()[width ? "width" : "height"]
                                        .toObject()[QString::number(number)]
                                        .toString();
                // 未列入规则的枚举值缺少已验证语义，直接拒绝。
                if (resize.isEmpty())
                    throw std::runtime_error("UNVERIFIED_RESIZE_MODE");
                // 具体尺寸模式直接返回；axis 表示还需按节点自身方向解释。
                if (resize != "axis")
                    return resize;
            }
            const bool primaryAxis = width == (childMode == "HORIZONTAL");
            auto key = primaryAxis ? "stackPrimarySizing" : "stackCounterSizing";
            return ca[key].toString("FIXED");
        };
        // Fixed 写固定尺寸，Hug 使用 sizeHint，Fill 释放最大尺寸并设置 Expanding。
        const auto applySize = [&](bool width) {
            const auto source = sizing(width);
            const auto resolved = mapping["sizeModes"].toObject()[source].toString();
            // 固定尺寸必须有源值，缺失时不能自动补零。
            if (resolved == "Fixed" && !child.contains(width ? "width" : "height"))
                throw std::runtime_error("MISSING_FIXED_SIZE");
            const int size = metric(child, width ? "width" : "height");
            // 按 Fixed、Hug、Fill 分配尺寸约束：锁定尺寸、使用内容建议值、或释放上限拉伸。
            if (resolved == "Fixed") {
                // 分别锁定当前轴的宽度或高度，避免一次求值误改另一轴。
                if (width)
                    widget->setFixedWidth(size);
                else
                    widget->setFixedHeight(size);
            } else if (resolved == "Hug") {
                auto policy = widget->sizePolicy();
                // 仅把当前轴改为 Preferred，使 Hug 的内容建议尺寸与另一轴的约束独立。
                if (width)
                    policy.setHorizontalPolicy(QSizePolicy::Preferred);
                else
                    policy.setVerticalPolicy(QSizePolicy::Preferred);
                widget->setSizePolicy(policy);
            } else if (resolved == "Fill") {
                auto policy = widget->sizePolicy();
                // 仅释放当前轴上限并设置拉伸策略，使 Fill 跟随父布局分配而不破坏另一轴。
                if (width) {
                    policy.setHorizontalPolicy(QSizePolicy::Expanding);
                    widget->setMaximumWidth(QWIDGETSIZE_MAX);
                } else {
                    policy.setVerticalPolicy(QSizePolicy::Expanding);
                    widget->setMaximumHeight(QWIDGETSIZE_MAX);
                }
                widget->setSizePolicy(policy);
                if (width == horizontal)
                    childMainFill = true;
                else {
                    childCrossFill = true;
                    hasCrossFill = true;
                }
            } else
                throw std::runtime_error("UNMAPPED_SIZE_MODE");
        };
        applySize(true);
        applySize(false);
        if (childMainFill && primary != "start")
            throw std::runtime_error("UNVERIFIED_MAIN_FILL_ALIGNMENT");
        const auto visibility = mapping["visibilityField"].toString();
        // 显式可见性必须是布尔值，避免字符串等被转换成意外的隐藏状态。
        if (child.contains(visibility) && !child[visibility].isBool())
            throw std::runtime_error("INVALID_VISIBILITY");
        widget->setVisible(child[visibility].toBool(mapping["missingVisibility"].toBool()));
        // 交叉轴 Fill 不再指定该轴对齐，否则 Qt 会按内容宽高缩回去。
        layout->addWidget(widget, 0, childCrossFill ? Qt::Alignment{} : axis(counter, !horizontal));
        order.append(child["guid"]);
    }
    layout->setAlignment(axis(primary, horizontal) |
                         (hasCrossFill ? Qt::Alignment{} : axis(counter, !horizontal)));
    records.append(QJsonObject{
        {"nodeId", node["guid"]},
        {"qtLayout", target},
        {"spacing", layout->spacing()},
        {"padding",
         QJsonArray{layout->contentsMargins().left(), layout->contentsMargins().top(),
                    layout->contentsMargins().right(), layout->contentsMargins().bottom()}},
        {"primary", primary},
        {"counter", counter},
        {"defaultedMetricFields", defaultedFields},
        {"childOrder", order}});
    return layout;
}
}
