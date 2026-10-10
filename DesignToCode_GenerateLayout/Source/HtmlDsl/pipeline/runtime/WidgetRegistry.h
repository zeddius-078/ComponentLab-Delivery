#pragma once
#include <QWidget>
#include <QJsonObject>
#include <QMap>
#include <functional>
#include <memory>

namespace commonWidget { class ComponentFactory; }

namespace v2 {
// 注册表把 XML 类名关联到 C++ 工厂；工厂创建组件内部结构，页面布局由 .ui 提供。
class WidgetRegistry {
public:
    // config 来自编译后的组件清单；返回控件交由 parent 或调用方管理。
    using Factory = std::function<QWidget*(const QJsonObject&, QWidget*)>;
    // 按 profile 复用主线已有 Token 产物，注册 Button、Checkbox、Input 和选项适配器。
    explicit WidgetRegistry(const QString& profile);
    ~WidgetRegistry();
    // 重复名称、空名称和空工厂会报错，防止同一 XML 类名含义发生覆盖。
    void registerType(const QString& className, Factory factory);

    bool contains(const QString& className) const;

    QWidget* create(const QString& className, const QJsonObject& config, QWidget* parent) const;

private:
    QMap<QString, Factory> factories;
    std::unique_ptr<commonWidget::ComponentFactory> componentFactory;
};

// 当前仅支持已明确的 disabled 选项形态，其他状态须补契约后再扩展。
QWidget* createSelectOption(const QJsonObject& config, QWidget* parent);
}
