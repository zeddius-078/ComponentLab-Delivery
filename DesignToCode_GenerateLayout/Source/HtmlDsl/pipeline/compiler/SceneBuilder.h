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
    // libraryProfile 为可切换组件库配置（可为空，空值等价 token-sdk 默认行为）。
    SceneBuilder(QJsonObject raw, QJsonObject componentRules, QJsonObject layoutRules,
                 QJsonObject extensions, QJsonObject engineeringNames = {},
                 QJsonObject libraryProfile = {});
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
    // qt-native 聚合：父框子节点全部为已登记 SelectOption 时返回这些子节点，否则返回空。
    QJsonArray optionGroup(const QJsonObject& node);
    // 按母版文字槽与实例覆盖解析选项标签；槽位必须唯一命中。
    QString optionLabel(const QJsonObject& node, const QJsonObject& family);
    QJsonObject raw, extensions, engineeringNames, libraryProfile, componentRules;
    d2c::ComponentConverter components;
    d2c::LayoutConverter layouts;
    QMap<QString, QWidget*> nodes;
    QSet<QString> usedNames;
    QJsonArray extra;
    QJsonArray optionGroups;
};
}
