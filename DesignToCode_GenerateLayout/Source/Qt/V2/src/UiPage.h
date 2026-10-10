#pragma once
// 一页静态文件的加载与节点查询接口：共同前缀关联 .ui、.components.json、.qss。
#include "JsonIO.h"
#include "WidgetRegistry.h"
#include <QJsonArray>
#include <QMap>
#include <QLayout>
#include <memory>

namespace v2 {
class UiPage {
public:
    // prefix 不带扩展名；registry 负责创建清单中声明的自定义控件。
    UiPage(const QString& prefix, std::shared_ptr<WidgetRegistry> registry);
    // 每个实例只加载一次。返回的页面由 parent 或调用方管理，UiPage 保存非拥有指针。
    QWidget* load(QWidget* parent = nullptr);

    // id 使用设计节点 ID，如 21:450；找不到时返回空指针。
    QWidget* widget(const QString& id) const {
        return nodes.value(id);
    }

    QLayout* layout(const QString& id) const {
        return layoutNodes.value(id);
    }

    QString rootId() const {
        return manifest["rootId"].toString();
    }

    QJsonObject trace() const;
    // 将扩展控件交给 XML 中唯一且布局为空的宿主管理，供后续仪表等组件接入。
    void mountExtension(const QString& objectName, QWidget* content);

private:
    QString prefix;
    QJsonObject manifest;
    std::shared_ptr<WidgetRegistry> registry;
    // 页面存活期间使用查询和挂载接口；页面销毁后这些观察指针不再有效。
    QMap<QString, QWidget*> nodes;
    QMap<QString, QLayout*> layoutNodes;
    QWidget* page = nullptr;
};
}
