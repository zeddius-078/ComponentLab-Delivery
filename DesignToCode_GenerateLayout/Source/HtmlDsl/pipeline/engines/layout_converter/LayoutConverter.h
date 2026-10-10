#pragma once
// 共享布局转换接口：将 Pixso 自布局字段映射为 Qt 横向/纵向布局。
#include <QJsonObject>
#include <QJsonArray>
#include <QBoxLayout>
#include <QWidget>
#include <functional>

namespace d2c {
class LayoutConverter {
public:
    // factory 负责子控件内容，owner 提供 Qt 父对象；布局引擎只安排尺寸和位置。
    using Factory = std::function<QWidget*(const QJsonObject&, QWidget*)>;

    // raw 可用于查找实例母版的尺寸策略；mapping 对应 layout-mapping.json。
    explicit LayoutConverter(QJsonObject mapping, QJsonObject raw = {})
        : mapping(std::move(mapping)), raw(std::move(raw)) {}

    // owner 必须尚无布局；返回值归 owner 所有，不支持的语义以异常报告。
    QBoxLayout* convert(const QJsonObject& container, QWidget* owner, const Factory& factory);

    // 输出方向、内边距、间距、对齐和实际添加顺序，用于与设计输入对照。
    QJsonArray trace() const {
        return records;
    }

private:
    QJsonObject mapping;
    QJsonObject raw;
    QJsonArray records;
};
}
