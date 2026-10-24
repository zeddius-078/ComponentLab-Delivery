#pragma once
// 共享组件转换接口：输入完整 DSL 和组件映射表，输出主线已有的 Qt 组件。
// 几何尺寸交给父布局处理；颜色和材质由 profile 对应的既有 Token 产物提供。
// libraryProfile 是可选的可切换组件库配置（library-profiles.json 中的一个 profile）：
// classRenames/componentOverrides 决定最终类名，apiPolicy/widgetSource 决定调用与创建方式。
#include "FieldChrome.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>
#include <memory>

namespace commonWidget { class ComponentFactory; }

namespace d2c {
class ComponentConverter {
public:
    // raw 含实例引用的母版定义；mapping 对应 HtmlDsl/pipeline/mapping/component-mapping.json。
    ComponentConverter(QJsonObject raw, QJsonObject mapping, QJsonObject libraryProfile = {},
                       bool layoutOnly = false);
    ~ComponentConverter();
    // 仅接受 INSTANCE；成功后由 parent 管理控件，parent 为空时由调用方释放。
    // 无映射、非法枚举或不支持的组合会抛出带节点上下文的异常。
    QWidget* convert(const QJsonObject& instance, QWidget* parent);

    // 身份事实源只来自普通 DSL 的组件引用；分离 DSL 的 name/componentName
    // 不参与 family 推断。空结果表示输入缺少可验证的普通实例身份。
    QJsonObject componentFamily(const QJsonObject& instance) const;
    QJsonObject componentMaster(const QJsonObject& instance) const;
    QWidget* structureFallback(const QJsonObject& instance, QWidget* parent,
                               const QString& detail);
    void discardTraceForNode(const QString& nodeId);

    // 记录成功转换的节点、目标类及 setter 调用，供生成器和测试读取。
    QJsonArray trace() const {
        return records;
    }

private:
    QJsonObject raw, mapping, profile;
    bool layoutOnly = false;
    QJsonArray records;
    std::unique_ptr<commonWidget::ComponentFactory> componentFactory;
};
}
