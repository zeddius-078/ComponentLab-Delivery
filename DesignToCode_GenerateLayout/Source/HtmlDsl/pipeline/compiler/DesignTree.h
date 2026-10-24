#pragma once
#include "VariableValidation.h"
// 平台中立的设计树投影：只读取原始设计字段，不依赖 QWidget 运行对象树。
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <functional>
#include <stdexcept>

namespace v2 {
// sourceHash 由调用方对输入文件计算，绑定本次树所依据的完整 DSL 或 B2 混合输入。
inline QJsonObject designTree(const QJsonObject& raw, const QString& sourceHash,
                              const QString& designProfile, const QString& runtimeProfile = {},
                              bool validateVariableBindings = true) {
    // 结构模式不读取变量值、Token 或组件外观。此时不能要求设计端提供
    // 多模式变量集的选择；严格模式仍完整执行原有 fail-closed 校验。
    const auto variableValidation = validateVariableBindings
        ? validateVariables(raw, designProfile, runtimeProfile)
        : QJsonObject{{"status", "NOT_APPLICABLE"},
                      {"reason", "STRUCTURE_ONLY_SKIPS_TOKEN_MODE_RESOLUTION"},
                      {"scope", "structure-only does not evaluate design variable values"}};
    QJsonArray nodes, roots;
    QSet<QString> ids;
    std::function<void(const QJsonObject&, const QJsonValue&)> walk;
    walk = [&](const QJsonObject& node, const QJsonValue& parent) {
        const auto id = node["guid"].toString();
        // 每个节点必须有唯一 ID，保证 XML、参数和设计树能一一对应。
        if (id.isEmpty() || ids.contains(id))
            throw std::runtime_error("INVALID_DESIGN_NODE_ID");
        ids.insert(id);
        QJsonArray children;
        // 先保存直接子节点 ID 与原顺序，避免 Qt 布局顺序反向污染设计关系。
        for (const auto& child : node["childNode"].toArray())
            children.append(child.toObject()["guid"]);
        // 按字段路径记录 ALIAS 与 boundVariables，保留引用关系，Token 数值由主线求解。
        QJsonObject bindings;
        std::function<void(const QJsonValue&, const QString&)> collect;
        collect = [&](const QJsonValue& value, const QString& path) {
            // 对象递归按字段名扩展路径，数组递归按索引扩展；标量没有可继续收集的绑定结构。
            if (value.isObject()) {
                const auto object = value.toObject();
                // 捕获变量别名或绑定对象本身，保留完整引用信息供后续 Token 对照。
                if (object["dataType"] == "ALIAS" || object.contains("boundVariables"))
                    bindings[path] = object;
                // 遍历本节点字段以记录绑定路径，后代节点交给 walk 独立处理。
                for (auto it = object.begin(); it != object.end(); ++it)
                    // 子节点由 walk 单独处理，避免把后代绑定重复计入当前节点。
                    if (it.key() != "childNode")
                        collect(it.value(), path.isEmpty() ? it.key() : path + "." + it.key());
            } else if (value.isArray()) {
                const auto array = value.toArray();
                // 按索引保留数组内绑定的位置，避免多个填色或属性引用相互覆盖。
                for (int i = 0; i < array.size(); ++i)
                    collect(array[i], path + "[" + QString::number(i) + "]");
            }
        };
        collect(node, "");
        // 缺失的坐标和尺寸保留为 null，防止补零后被误认为设计端明确给出的值。
        QJsonObject bounds;
        // 逐项复制位置与尺寸到中立 bounds，保留原始数值，供运行几何独立对照。
        for (const auto& key : {"left", "top", "width", "height"})
            bounds[key] = node.contains(key) ? node[key] : QJsonValue(QJsonValue::Null);
        QJsonValue component(QJsonValue::Null);
        // 只有组件实例带母版引用；普通容器和文本保持 componentRef 为 null。
        if (node.contains("mainComponent"))
            component = QJsonObject{{"nodeId", node["mainComponent"]},
                                    {"setId", node["mainStateGroup"]},
                                    {"key", node["componentKey"]},
                                    {"fileKey", node["publishFile"]}};
        nodes.append(QJsonObject{{"nodeId", id},
                                 {"name", node["name"]},
                                 {"type", node["type"]},
                                 {"parentId", parent},
                                 {"children", children},
                                 {"componentRef", component},
                                 {"bounds", bounds},
                                 {"layout", node["autoLayout"].toObject()},
                                 {"tokenBindings", bindings},
                                 {"variableModes", node["variableModeBySetMap"].toObject()},
                                 {"componentProperties", node["propRefMap"].toObject()}});
        // 完成当前节点记录后递归后代，并传入当前 ID 建立父子关系。
        for (const auto& child : node["childNode"].toArray())
            walk(child.toObject(), id);
    };
    // 根节点 parentId 为 null；children 维持设计数据原顺序，不套用 Qt 排布顺序。
    for (const auto& node : raw["pixTreeNodes"].toArray()) {
        roots.append(node.toObject()["guid"]);
        walk(node.toObject(), QJsonValue(QJsonValue::Null));
    }
    // 空输入没有可生成的设计页面，明确失败而不输出空树成功结果。
    if (roots.isEmpty())
        throw std::runtime_error("DESIGN_ROOT_REQUIRED");
    // B2 输入附带 HTML/DSL 来源哈希；provider 是格式标签，服务真实性依赖采集证据。
    return {{"schemaVersion", 1},
            {"variableValidation", variableValidation},
            {"provider", "pixso"},
            {"sourceSha256", sourceHash},
            {"inputKind", raw.contains("_provenance") ? "html-plus-dsl" : "full-dsl"},
            {"provenance", raw["_provenance"].toObject()},
            {"boundsSpace", "parent-local"},
            {"rootIds", roots},
            {"nodes", nodes}};
}
}
