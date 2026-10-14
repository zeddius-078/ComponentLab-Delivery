// 尺寸求值在此集中完成，组件转换器无需重复解释布局字段。
#include "LayoutConverter.h"
#include "DesignComponentCatalog.h"
#include <QJsonDocument>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QSet>
#include <QVBoxLayout>
#include <QSpacerItem>
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

int integerMetric(const QJsonObject& o, const QString& key, int fallback = 0) {
    // 网格行列与跨度必须是整数；把小数静默舍入会改变设计节点的占位关系。
    if (!o.contains(key))
        return fallback;
    const auto value = o[key];
    const auto number = value.toDouble();
    if (!value.isDouble() || !qIsFinite(number) || number < 0 || number > QWIDGETSIZE_MAX ||
        number != std::floor(number))
        throw std::runtime_error(("INVALID_LAYOUT_INDEX: " + key).toStdString());
    return value.toInt();
}

QJsonObject layoutHelperDefinition(const QJsonObject& mapping,
                                   const QJsonObject& contract,
                                   const QString& expectedRole) {
    const auto rules = mapping["layoutHelpers"].toObject();
    const auto idField = rules["canonicalIdField"].toString();
    if (idField.isEmpty())
        throw std::runtime_error("INVALID_LAYOUT_HELPER_RULE");
    const auto canonicalId = contract[idField].toString();
    if (canonicalId.isEmpty())
        throw std::runtime_error("MISSING_LAYOUT_HELPER_ID");
    bool catalogMatch = false;
    for (const auto& entry : commonWidget::designLibraryEntries()) {
        if (entry.canonicalId != canonicalId)
            continue;
        if (entry.kind != QStringLiteral("LayoutHelper"))
            throw std::runtime_error("CANONICAL_ENTRY_IS_NOT_LAYOUT_HELPER");
        catalogMatch = true;
        break;
    }
    if (!catalogMatch)
        throw std::runtime_error("UNKNOWN_LAYOUT_HELPER");
    const auto definition = rules["definitions"].toObject()[canonicalId].toObject();
    if (definition.isEmpty())
        throw std::runtime_error("UNMAPPED_LAYOUT_HELPER");
    if (definition["role"].toString() != expectedRole)
        throw std::runtime_error("LAYOUT_HELPER_ROLE_MISMATCH");
    return definition;
}
}

QLayout* LayoutConverter::convert(const QJsonObject& node, QWidget* owner,
                                   const Factory& factory) {
    // 规则表限定布局方向与字段名；padding 顺序固定为左、上、右、下。
    const auto a = node["autoLayout"].toObject();
    const auto mode = a["stackMode"].toString();
    const auto target = mapping["modes"].toObject()[mode].toString();
    // 只实例化规则明确映射的四类布局，未知方向无法稳定还原。
    if (target != "QHBoxLayout" && target != "QVBoxLayout" &&
        target != "QGridLayout" && target != "QFormLayout")
        throw std::runtime_error("UNMAPPED_LAYOUT_MODE");
    const auto helperRules = mapping["layoutHelpers"].toObject();
    const auto helperField = helperRules["contractField"].toString();
    if (helperField.isEmpty())
        throw std::runtime_error("INVALID_LAYOUT_HELPER_RULE");
    if (node.contains(helperField) && !node[helperField].isObject())
        throw std::runtime_error("INVALID_LAYOUT_HELPER_CONTRACT");
    const auto containerContract = node[helperField].toObject();
    QJsonObject containerDefinition;
    int layoutContainerColumns = 0;
    QString layoutContainerDensity;
    if (!containerContract.isEmpty()) {
        containerDefinition = layoutHelperDefinition(
            mapping, containerContract, QStringLiteral("container"));
        if (containerDefinition["requiredQtLayout"].toString() != target)
            throw std::runtime_error("LAYOUT_CONTAINER_MODE_MISMATCH");
        const auto columnsField = containerDefinition["columnsField"].toString();
        const auto densityField = containerDefinition["densityField"].toString();
        layoutContainerColumns = integerMetric(containerContract, columnsField);
        if (!containerDefinition["columns"].toArray().contains(layoutContainerColumns))
            throw std::runtime_error("UNMAPPED_LAYOUT_CONTAINER_COLUMNS");
        layoutContainerDensity = containerContract[densityField].toString();
        if (!containerDefinition["densities"].toArray().contains(layoutContainerDensity))
            throw std::runtime_error("UNMAPPED_LAYOUT_CONTAINER_DENSITY");
    }
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
    const bool linear = target == "QHBoxLayout" || target == "QVBoxLayout";
    const bool horizontal = target == "QHBoxLayout";
    QLayout* layout = nullptr;
    // 生成对象类型直接由映射合同决定，避免把 Grid/Form 降级为视觉相似的盒式布局。
    if (target == "QHBoxLayout")
        layout = new QHBoxLayout(owner);
    else if (target == "QVBoxLayout")
        layout = new QVBoxLayout(owner);
    else if (target == "QGridLayout")
        layout = new QGridLayout(owner);
    else
        layout = new QFormLayout(owner);
    // padding 必须按左、上、右、下四项取值，数量不符表示规则本身有误。
    if (pads.size() != 4)
        throw std::runtime_error("INVALID_PADDING_RULE");
    layout->setContentsMargins(metric(a, pads[0].toString()), metric(a, pads[1].toString()),
                               metric(a, pads[2].toString()), metric(a, pads[3].toString()));
    const auto gap = metric(a, mapping["gapField"].toString());
    // 线性布局使用单一 spacing；Grid/Form 将同一设计间距投影到水平和垂直方向。
    if (auto* box = qobject_cast<QBoxLayout*>(layout))
        box->setSpacing(gap);
    else if (auto* grid = qobject_cast<QGridLayout*>(layout)) {
        grid->setHorizontalSpacing(a.contains("gridColumnGap")
                                       ? metric(a, "gridColumnGap")
                                       : gap);
        grid->setVerticalSpacing(a.contains("gridRowGap") ? metric(a, "gridRowGap") : gap);
    } else if (auto* form = qobject_cast<QFormLayout*>(layout)) {
        form->setHorizontalSpacing(a.contains("formColumnGap")
                                       ? metric(a, "formColumnGap")
                                       : gap);
        form->setVerticalSpacing(a.contains("formRowGap") ? metric(a, "formRowGap") : gap);
    }
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
    const auto horizontalAlignment = [&](const QString& value) -> Qt::Alignment {
        // Grid/Form 的单元格对齐只接受水平方向语义，垂直方向由独立字段处理。
        if (value == "start") return Qt::AlignLeft;
        if (value == "center") return Qt::AlignHCenter;
        if (value == "end") return Qt::AlignRight;
        throw std::runtime_error("UNMAPPED_HORIZONTAL_ALIGNMENT");
    };
    const auto verticalAlignment = [&](const QString& value) -> Qt::Alignment {
        // Grid 单元格的垂直对齐必须单独映射，不能复用 QBoxLayout 的主轴方向。
        if (value == "start") return Qt::AlignTop;
        if (value == "center") return Qt::AlignVCenter;
        if (value == "end") return Qt::AlignBottom;
        throw std::runtime_error("UNMAPPED_VERTICAL_ALIGNMENT");
    };
    if (auto* form = qobject_cast<QFormLayout*>(layout)) {
        // 表单属性直接对应 Qt 的公开枚举，未知值拒绝生成，避免改变标签/字段关系。
        const auto policies = mapping["formPolicies"].toObject();
        const auto growth = a["formFieldGrowthPolicy"].toString("allNonFixedFieldsGrow");
        const auto wrap = a["formRowWrapPolicy"].toString("dontWrapRows");
        const auto growthValue = policies["fieldGrowthPolicy"].toObject()[growth].toString();
        const auto wrapValue = policies["rowWrapPolicy"].toObject()[wrap].toString();
        if (growthValue.isEmpty() || wrapValue.isEmpty())
            throw std::runtime_error("UNMAPPED_FORM_POLICY");
        if (growthValue == "FieldsStayAtSizeHint")
            form->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
        else if (growthValue == "ExpandingFieldsGrow")
            form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        else if (growthValue == "AllNonFixedFieldsGrow")
            form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        else
            throw std::runtime_error("UNMAPPED_FORM_POLICY");
        if (wrapValue == "DontWrapRows")
            form->setRowWrapPolicy(QFormLayout::DontWrapRows);
        else if (wrapValue == "WrapLongRows")
            form->setRowWrapPolicy(QFormLayout::WrapLongRows);
        else if (wrapValue == "WrapAllRows")
            form->setRowWrapPolicy(QFormLayout::WrapAllRows);
        else
            throw std::runtime_error("UNMAPPED_FORM_POLICY");
        if (a.contains("formLabelAlignment"))
            form->setLabelAlignment(horizontalAlignment(
                mapping["alignmentValues"].toObject()[a["formLabelAlignment"].toString()].toString()));
        if (a.contains("formAlignment"))
            form->setFormAlignment(horizontalAlignment(
                mapping["alignmentValues"].toObject()[a["formAlignment"].toString()].toString()));
    }
    if (auto* grid = qobject_cast<QGridLayout*>(layout)) {
        // 行列拉伸是容器属性，先于子项加入，确保 Fill 子项获得确定的分配结果。
        const auto setStretch = [&](const QString& key, bool row) {
            const auto values = a[key].toArray();
            for (int i = 0; i < values.size(); ++i) {
                const auto value = values[i];
                if (!value.isDouble() || value.toDouble() < 0 || !qIsFinite(value.toDouble()) ||
                    value.toDouble() != std::floor(value.toDouble()))
                    throw std::runtime_error("INVALID_GRID_STRETCH");
                if (row) grid->setRowStretch(i, value.toInt());
                else grid->setColumnStretch(i, value.toInt());
            }
        };
        setStretch("gridRowStretch", true);
        setStretch("gridColumnStretch", false);
    }
    bool hasCrossFill = false;
    auto children = node["childNode"].toArray();
    QJsonArray order;
    QJsonArray items;
    QSet<QString> occupiedCells;
    QSet<QString> formRoles;
    int layoutContainerPlacementMode = 0; // 0 unknown, 1 automatic, 2 explicit
    int layoutContainerItemIndex = 0;
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
        // 网格位置和表单角色属于子项契约，与容器的 autoLayout 分开保存也要纳入同一覆盖链。
        merge(child["layoutItem"].toObject());
        // 只合并指向母版根的覆盖；内部标签或图标的布局不决定实例外部空间。
        for (const auto& value : child["props"].toArray()) {
            const auto override = value.toObject();
            // 过滤子部件覆盖，保持父布局尺寸来源为组件根。
            if (override["componentId"] != child["mainComponent"])
                continue;
            merge(override["autoLayout"].toObject());
            merge(override["layoutItem"].toObject());
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
        if (child.contains(helperField) && !child[helperField].isObject())
            throw std::runtime_error("INVALID_LAYOUT_HELPER_CONTRACT");
        const auto childHelper = child[helperField].toObject();
        if (!childHelper.isEmpty()) {
            const auto spacerDefinition = layoutHelperDefinition(
                mapping, childHelper, QStringLiteral("spacer"));
            auto* box = qobject_cast<QBoxLayout*>(layout);
            if (!box)
                throw std::runtime_error("SPACER_REQUIRES_BOX_LAYOUT");
            const auto directionField = spacerDefinition["directionField"].toString();
            const auto sizeField = spacerDefinition["sizeField"].toString();
            const auto behaviorField = spacerDefinition["behaviorField"].toString();
            const auto direction = spacerDefinition["directions"].toObject()
                                       [childHelper[directionField].toString()].toString();
            if (direction.isEmpty())
                throw std::runtime_error("UNMAPPED_SPACER_DIRECTION");
            if ((direction == QStringLiteral("horizontal")) != horizontal)
                throw std::runtime_error("SPACER_DIRECTION_MISMATCH");
            const auto requestedBehavior = childHelper[behaviorField].toString(
                spacerDefinition["defaultBehavior"].toString());
            const auto behavior = spacerDefinition["behaviors"].toObject()
                                      [requestedBehavior].toString();
            if (behavior.isEmpty())
                throw std::runtime_error("UNMAPPED_SPACER_BEHAVIOR");
            const int size = metric(childHelper, sizeField);
            if (child["guid"].toString().isEmpty())
                throw std::runtime_error("MISSING_LAYOUT_HELPER_NODE_ID");
            const auto visibility = mapping["visibilityField"].toString();
            if (child.contains(visibility)) {
                if (!child[visibility].isBool())
                    throw std::runtime_error("INVALID_VISIBILITY");
                if (!child[visibility].toBool())
                    throw std::runtime_error("HIDDEN_LAYOUT_HELPER_UNSUPPORTED");
            }
            const QSizePolicy::Policy mainPolicy = behavior == QStringLiteral("stretch")
                ? QSizePolicy::Expanding
                : behavior == QStringLiteral("minimum")
                ? QSizePolicy::Minimum
                : QSizePolicy::Fixed;
            auto* spacer = direction == QStringLiteral("horizontal")
                ? new QSpacerItem(size, 0, mainPolicy, QSizePolicy::Fixed)
                : new QSpacerItem(0, size, QSizePolicy::Fixed, mainPolicy);
            box->addSpacerItem(spacer);
            order.append(child["guid"]);
            items.append(QJsonObject{{"nodeId", child["guid"]},
                                     {"kind", "LayoutHelper"},
                                     {"canonicalId", childHelper[helperRules["canonicalIdField"].toString()]},
                                     {"direction", direction},
                                     {"size", size},
                                     {"behavior", behavior}});
            continue;
        }
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
            // Grid/Form 没有单一主轴；缺少专用尺寸字段时按 Fixed 处理，避免借用错误的方向语义。
            if (!linear) {
                const auto key = width ? "layoutWidthSizing" : "layoutHeightSizing";
                return ca[key].toString("FIXED");
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
        Qt::Alignment itemAlignment = childCrossFill ? Qt::Alignment{} : axis(counter, !horizontal);
        if (!linear) {
            // Grid 单元格可分别声明横纵对齐；缺省值沿用 start，保证输出稳定。
            const auto h = mapping["alignmentValues"].toObject()
                               [ca["gridHorizontalAlign"].toString("flex-start")].toString();
            const auto v = mapping["alignmentValues"].toObject()
                               [ca["gridVerticalAlign"].toString("flex-start")].toString();
            itemAlignment = horizontalAlignment(h) | verticalAlignment(v);
        }
        if (auto* box = qobject_cast<QBoxLayout*>(layout)) {
            box->addWidget(widget, 0, itemAlignment);
        } else if (auto* grid = qobject_cast<QGridLayout*>(layout)) {
            const auto gridRules = mapping["grid"].toObject();
            const auto rowKey = gridRules["rowField"].toString("gridRow");
            const auto columnKey = gridRules["columnField"].toString("gridColumn");
            const auto rowSpanKey = gridRules["rowSpanField"].toString("gridRowSpan");
            const auto columnSpanKey = gridRules["columnSpanField"].toString("gridColumnSpan");
            const bool hasRow = ca.contains(rowKey);
            const bool hasColumn = ca.contains(columnKey);
            if (hasRow != hasColumn)
                throw std::runtime_error("MISSING_GRID_PLACEMENT");
            int row = 0;
            int column = 0;
            if (!containerDefinition.isEmpty()) {
                const int placementMode = hasRow ? 2 : 1;
                if (layoutContainerPlacementMode &&
                    layoutContainerPlacementMode != placementMode)
                    throw std::runtime_error("PARTIAL_LAYOUT_CONTAINER_PLACEMENT");
                layoutContainerPlacementMode = placementMode;
                if (placementMode == 1) {
                    row = layoutContainerItemIndex / layoutContainerColumns;
                    column = layoutContainerItemIndex % layoutContainerColumns;
                } else {
                    row = integerMetric(ca, rowKey);
                    column = integerMetric(ca, columnKey);
                }
            } else {
                if (!hasRow)
                    throw std::runtime_error("MISSING_GRID_PLACEMENT");
                row = integerMetric(ca, rowKey);
                column = integerMetric(ca, columnKey);
            }
            const int rowSpan = integerMetric(ca, rowSpanKey, 1);
            const int columnSpan = integerMetric(ca, columnSpanKey, 1);
            if (rowSpan < 1 || columnSpan < 1)
                throw std::runtime_error("INVALID_GRID_SPAN");
            if (!containerDefinition.isEmpty() && layoutContainerPlacementMode == 1 &&
                (rowSpan != 1 || columnSpan != 1))
                throw std::runtime_error("AUTO_LAYOUT_CONTAINER_SPAN_UNSUPPORTED");
            // 跨度覆盖的每个格子都登记，重复占用直接失败，避免 Qt 以隐式顺序重排设计内容。
            for (int r = row; r < row + rowSpan; ++r)
                for (int c = column; c < column + columnSpan; ++c) {
                    const auto cell = QString::number(r) + ":" + QString::number(c);
                    if (occupiedCells.contains(cell))
                        throw std::runtime_error("GRID_CELL_OVERLAP");
                    occupiedCells.insert(cell);
                }
            grid->addWidget(widget, row, column, rowSpan, columnSpan, itemAlignment);
            items.append(QJsonObject{{"nodeId", child["guid"]}, {"row", row},
                                     {"column", column}, {"rowSpan", rowSpan},
                                     {"columnSpan", columnSpan}});
            ++layoutContainerItemIndex;
        } else if (auto* form = qobject_cast<QFormLayout*>(layout)) {
            const auto formRules = mapping["form"].toObject();
            const auto rowKey = formRules["rowField"].toString("formRow");
            const auto roleKey = formRules["roleField"].toString("formRole");
            if (!ca.contains(rowKey) || !ca.contains(roleKey))
                throw std::runtime_error("MISSING_FORM_PLACEMENT");
            const int row = integerMetric(ca, rowKey);
            const auto role = ca[roleKey].toString();
            const auto roleValue = formRules["roles"].toObject()[role].toString();
            if (roleValue.isEmpty())
                throw std::runtime_error("UNMAPPED_FORM_ROLE");
            const auto roleKeyUsed = QString::number(row) + ":" + roleValue;
            if (formRoles.contains(roleKeyUsed))
                throw std::runtime_error("DUPLICATE_FORM_ROLE");
            formRoles.insert(roleKeyUsed);
            if (roleValue == "LabelRole")
                form->setWidget(row, QFormLayout::LabelRole, widget);
            else if (roleValue == "FieldRole")
                form->setWidget(row, QFormLayout::FieldRole, widget);
            else if (roleValue == "SpanningRole")
                form->setWidget(row, QFormLayout::SpanningRole, widget);
            else
                throw std::runtime_error("UNMAPPED_FORM_ROLE");
            items.append(QJsonObject{{"nodeId", child["guid"]}, {"row", row},
                                     {"role", roleValue}});
        }
        order.append(child["guid"]);
    }
    if (linear)
        layout->setAlignment(axis(primary, horizontal) |
                             (hasCrossFill ? Qt::Alignment{} : axis(counter, !horizontal)));
    QJsonObject record{
        {"nodeId", node["guid"]},
        {"qtLayout", target},
        {"spacing", gap},
        {"padding",
         QJsonArray{layout->contentsMargins().left(), layout->contentsMargins().top(),
                    layout->contentsMargins().right(), layout->contentsMargins().bottom()}},
        {"primary", primary},
        {"counter", counter},
        {"defaultedMetricFields", defaultedFields},
        {"childOrder", order},
        {"items", items}};
    if (!containerDefinition.isEmpty())
        record["layoutHelper"] = QJsonObject{
            {"canonicalId", containerContract[helperRules["canonicalIdField"].toString()]},
            {"columns", layoutContainerColumns},
            {"density", layoutContainerDensity}};
    records.append(record);
    return layout;
}
}
