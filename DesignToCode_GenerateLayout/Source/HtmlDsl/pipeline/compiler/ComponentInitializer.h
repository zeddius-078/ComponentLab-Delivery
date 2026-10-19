#pragma once

// 页面初始化代码生成器：只消费组件转换器已经确认的 API 调用，不重新解析设计属性或组件映射。
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QString>
#include <functional>

namespace v2 {
namespace component_initializer {

// 将已验证的组件调用记录转换为页面构造函数中的初始化代码。
// token-sdk 使用 ComponentFactory::apply；public-qt（direct=true）直接调用公开 Qt 方法。
// 字面量转换由 UiCompiler 提供，保证字符串转义和类型检查沿用既有规则。
inline QString emitInitialization(const QMap<QString, QJsonObject>& specs,
                                  const std::function<QString(const QString&)>& stringLiteral,
                                  const std::function<QString(const QJsonValue&)>& valueLiteral,
                                  bool direct = false) {
    QString code;
    // QMap 已按 objectName 排序；逐个组件发射调用，保证重复生成的代码顺序稳定。
    for (auto it = specs.cbegin(); it != specs.cend(); ++it) {
        // 一个组件的调用顺序来自映射转换器，初始化阶段不重新排序。
        for (const auto& item : it.value()["calls"].toArray()) {
            const auto call = item.toObject();
            if (direct) {
                code += "    ui_->" + it.key() + "->" + call["api"].toString() + "(" +
                        valueLiteral(call["value"]) + ");\n";
            } else {
                code += "    if (!factory.apply(ui_->" + it.key() + ", " +
                        stringLiteral(call["api"].toString()) + ", QJsonValue(" +
                        valueLiteral(call["value"]) + "), &error))\n"
                        "        throw std::runtime_error(error.toStdString());\n";
            }
        }
    }
    return code;
}

} // namespace component_initializer
} // namespace v2
