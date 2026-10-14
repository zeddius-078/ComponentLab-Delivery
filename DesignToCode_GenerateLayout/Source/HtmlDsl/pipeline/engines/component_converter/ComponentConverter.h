#pragma once
// 共享组件转换接口：输入完整 DSL 和组件映射表，输出主线已有的 Qt 组件。
// 几何尺寸交给父布局处理；颜色和材质由 profile 对应的既有 Token 产物提供。
#include "Surface.h"
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
    ComponentConverter(QJsonObject raw, QJsonObject mapping);
    ~ComponentConverter();
    // 仅接受 INSTANCE；成功后由 parent 管理控件，parent 为空时由调用方释放。
    // 无映射、非法枚举或不支持的组合会抛出带节点上下文的异常。
    QWidget* convert(const QJsonObject& instance, QWidget* parent);

    // 记录成功转换的节点、目标类及 setter 调用，供生成器和测试读取。
    QJsonArray trace() const {
        return records;
    }

private:
    QJsonObject raw, mapping;
    QJsonArray records;
    std::shared_ptr<const surface::Catalog> catalog;
    std::unique_ptr<commonWidget::ComponentFactory> componentFactory;
};
}
