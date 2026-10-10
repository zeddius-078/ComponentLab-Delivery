#pragma once
// 构建期场景装配：用真实组件和布局求出结构，供 UiCompiler 序列化为静态页面。
#include "ComponentConverter.h"
#include "LayoutConverter.h"
#include "JsonIO.h"
#include <QMap>
#include <QSet>

namespace v2 {
// 对象字段递归合并，数组和标量整体替换；扩展规则优先于基础规则。
QJsonObject mergeRules(QJsonObject base, const QJsonObject& extension);

class SceneBuilder {
public:
    // raw 为单根完整 DSL，extensions 提供本轮已明确的布局与 SelectOption 适配范围。
    SceneBuilder(QJsonObject raw, QJsonObject componentRules, QJsonObject layoutRules,
                 QJsonObject extensions, QJsonObject engineeringNames = {});
    // 同一实例只构建一次；页面归 parent 或调用方管理，内部 nodes 仅保存观察指针。
    QWidget* build(QWidget* parent = nullptr);

    QWidget* widget(const QString& id) const {
        return nodes.value(id);
    }

    // 返回组件调用、额外适配器、布局和测量值，生成器从中取得初始化参数。
    QJsonObject trace() const;

    QJsonObject root() const {
        return raw["pixTreeNodes"].toArray().first().toObject();
    }

private:
    QWidget* create(const QJsonObject& node, QWidget* parent);
    QWidget* option(const QJsonObject& node, QWidget* parent);
    QJsonObject raw, extensions, engineeringNames;
    d2c::ComponentConverter components;
    d2c::LayoutConverter layouts;
    QMap<QString, QWidget*> nodes;
    QSet<QString> usedNames;
    QJsonArray extra;
};
}
