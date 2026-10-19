// 运行期按 XML 创建页面，并把设计节点 ID 与实际 QWidget 一一绑定。
#include "UiPage.h"
#include <QUiLoader>
#include <QLayout>
#include <QBoxLayout>
#include <QFile>
#include <QSet>

namespace v2 {
namespace {
// 只重载自定义组件的创建过程，标准控件和布局继续由 QUiLoader 解析。
class Loader : public QUiLoader {
public:
    Loader(const QJsonObject& manifest, const WidgetRegistry& registry) : registry(registry) {
        // 把设计节点 ID 转成 XML objectName 索引，为自定义控件创建前的身份核对做准备。
        for (const auto& c : manifest["components"].toArray()) {
            auto spec = c.toObject();
            const auto id = spec["nodeId"].toString();
            const auto name = manifest["nodes"].toObject()[id].toString();
            // 空名称或重复绑定会导致组件身份歧义，在加载 XML 前拒绝。
            if (name.isEmpty() || specs.contains(name))
                throw std::runtime_error("INVALID_COMPONENT_BINDING");
            specs[name] = spec;
        }
    }

    QString failure;
    QSet<QString> created;
    QMap<QString, QJsonObject> specs;

protected:
    QWidget* createWidget(const QString& type, QWidget* parent, const QString& name) override {
        // 类型、objectName 和清单三者一致才允许创建；避免未知组件被静默降级。
        try {
            // 已注册类型交给受控工厂；标准 Qt 控件沿用 QUiLoader，未知自定义类型报错。
            if (registry.contains(type)) {
                // XML 类型必须与同名组件清单一致，防止拼接了另一页面的参数。
                if (!specs.contains(name) || specs[name]["qtClass"] != type)
                    throw std::runtime_error("UI_COMPONENT_CLASS_MISMATCH");
                // 每份组件参数只能实例化一次，避免重复名称覆盖节点关联。
                if (created.contains(name))
                    throw std::runtime_error("DUPLICATE_UI_COMPONENT");
                auto* w = registry.create(type, specs[name], parent);
                w->setObjectName(name);
                created.insert(name);
                return w;
            }
            // qt-native 等库配置：清单组件类可以是公开 Qt 内置类，其初始化参数已由
            // 生成器序列化进 XML 属性；这里交给 QUiLoader 创建，不再要求注册工厂。
            if (specs.contains(name)) {
                if (specs[name]["qtClass"] != type)
                    throw std::runtime_error("UI_COMPONENT_CLASS_MISMATCH");
                // 带命名空间的类没有内置构造路径，仍必须注册；未知内置类由 QUiLoader 返回空。
                if (type.contains("::"))
                    throw std::runtime_error(("UNREGISTERED_UI_CLASS: " + type).toStdString());
                auto* w = QUiLoader::createWidget(type, parent, name);
                if (!w)
                    throw std::runtime_error(("UNREGISTERED_UI_CLASS: " + type).toStdString());
                if (created.contains(name))
                    throw std::runtime_error("DUPLICATE_UI_COMPONENT");
                w->setObjectName(name);
                created.insert(name);
                return w;
            }
            // 非组件节点（FRAME/文本容器）继续由 QUiLoader 按标准控件创建。
            return QUiLoader::createWidget(type, parent, name);
            // 先保存异常信息，待 loader.load 返回后由外层统一抛出并释放半成品页面。
        } catch (const std::exception& e) {
            failure = QString::fromUtf8(e.what());
            return nullptr;
        }
    }

private:
    const WidgetRegistry& registry;
};
}

UiPage::UiPage(const QString& source, std::shared_ptr<WidgetRegistry> factories)
    : prefix(source), manifest(readJson(source + ".components.json")),
      registry(std::move(factories)) {
    // 只接受当前参数协议版本，避免旧字段被当作空值加载。
    if (manifest["schemaVersion"].toInt() != 2)
        throw std::runtime_error("UNSUPPORTED_UI_MANIFEST");
}

QWidget* UiPage::load(QWidget* parent) {
    // 加载器只持有一份页面，重复 load 会造成节点引用与对象寿命混乱。
    if (page)
        throw std::runtime_error("UI_ALREADY_LOADED");
    QFile file(prefix + ".ui");
    // XML 缺失时终止，避免显示空窗口掩盖部署遗漏。
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("UI_FILE_MISSING");
    Loader loader(manifest, *registry);
    std::unique_ptr<QWidget> owned(loader.load(&file, parent));
    // 优先反馈工厂捕获的具体错误，让外层 unique_ptr 释放半成品页面。
    if (!loader.failure.isEmpty())
        throw std::runtime_error(loader.failure.toStdString());
    // Qt 加载器可能直接失败；保留其错误信息便于定位 XML 问题。
    if (!owned)
        throw std::runtime_error(("UI_LOAD_FAILED: " + loader.errorString()).toStdString());
    // 全部声明组件都必须出现在 XML 中，数量不符表示页面与参数未成套。
    if (loader.created.size() != loader.specs.size())
        throw std::runtime_error("UI_COMPONENT_MISSING");
    // 遍历实际控件核对唯一名称及 designNodeId；根控件也参与检查。
    const auto declared = manifest["nodes"].toObject();
    QSet<QString> declaredLayouts;
    for (const auto& id : manifest["layoutNodeIds"].toArray())
        declaredLayouts.insert(id.toString());
    const auto all = owned->findChildren<QWidget*>();
    // 逐个设计 ID 查找实际 QWidget，建立后续控制器操作与来源追溯共用的索引。
    for (auto it = declared.begin(); it != declared.end(); ++it) {
        if (declaredLayouts.contains(it.key())) {
            auto matches = owned->findChildren<QLayout*>(it.value().toString());
            if (matches.size() != 1 ||
                matches.first()->property("designNodeId").toString() != it.key())
                throw std::runtime_error("UI_LAYOUT_BINDING_MISMATCH");
            layoutNodes[it.key()] = matches.first();
            continue;
        }
        QList<QWidget*> candidates;
        // 根控件不包含在 findChildren 结果内，需要独立参与绑定核对。
        if (owned->objectName() == it.value().toString())
            candidates.append(owned.get());
        // 查找所有后代中的同名控件，以发现漏项或重复名称，而非只取第一个。
        for (auto* w : all)
            // 收集所有同名对象用于唯一性判断，避免重名时任意选中其中一个。
            if (w->objectName() == it.value().toString())
                candidates.append(w);
        // 每个设计 ID 必须唯一对应 QWidget，并核对动态属性防止绑定串位。
        if (candidates.size() != 1 ||
            candidates.first()->property("designNodeId").toString() != it.key())
            throw std::runtime_error("UI_NODE_BINDING_MISMATCH");
        nodes[it.key()] = candidates.first();
    }
    // 页面 QSS 处理标题和背景，组件的 Token 样式已由应用入口及注册工厂接入。
    QFile qss(prefix + ".qss");
    // 页面 QSS 属于必需交付文件，缺失时拒绝以默认样式继续展示。
    if (!qss.open(QIODevice::ReadOnly))
        throw std::runtime_error("PAGE_QSS_MISSING");
    owned->setStyleSheet(QString::fromUtf8(qss.readAll()));
    owned->setProperty("structureSource", "ui-xml");
    page = owned.release();
    return page;
}

void UiPage::mountExtension(const QString& name, QWidget* content) {
    // 挂载会改变 content 的父对象；拒绝重复占用，保持一个 XML 宿主对应一个扩展。
    // 挂载前要求宿主页面已加载且扩展对象有效，避免操作空对象。
    if (!page || !content)
        throw std::runtime_error("INVALID_EXTENSION_MOUNT");
    auto hosts = page->findChildren<QWidget*>(name);
    // 根页面也可能就是挂载点，单独纳入搜索以统一根与子容器行为。
    if (page->objectName() == name)
        hosts.prepend(page);
    // 宿主必须唯一且有空布局，避免替换已有内容或把扩展放入错误容器。
    if (hosts.size() != 1 || !hosts.first()->layout() || hosts.first()->layout()->count() != 0)
        throw std::runtime_error("EXTENSION_REQUIRES_EMPTY_XML_HOST");
    content->setParent(hosts.first());
    hosts.first()->layout()->addWidget(content);
    content->show();
}

QJsonObject UiPage::trace() const {
    // 几何坐标相对父控件；这里记录运行对象，设计结构保存在独立设计树文件中。
    QJsonArray geometry, layouts, components, adapters;
    // 将选项适配与普通组件分别报告，方便对照编译期两类调用记录。
    for (const auto& c : manifest["components"].toArray()) {
        // 依据明确的适配器类名分类，其他记录保留为通用组件调用。
        if (c.toObject()["qtClass"] == "v2::SelectOptionAdapter")
            adapters.append(c);
        else
            components.append(c);
    }
    // 遍历已核验节点记录真实几何和策略，供测试与设计基线比较。
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        const auto* w = it.value();
        const auto r = w->geometry();
        geometry.append(
            QJsonObject{{"nodeId", it.key()},
                        {"qtClass", w->metaObject()->className()},
                        {"geometry", QJsonArray{r.x(), r.y(), r.width(), r.height()}},
                        {"enabled", w->isEnabled()},
                        {"sizeHint", QJsonArray{w->sizeHint().width(), w->sizeHint().height()}},
                        {"horizontalPolicy", int(w->sizePolicy().horizontalPolicy())},
                        {"verticalPolicy", int(w->sizePolicy().verticalPolicy())}});
        // 只报告页面 FRAME 的布局，避免把组件内部结构混入设计容器树。
        if (w->property("designType") == "FRAME" && w->layout()) {
            auto* l = w->layout();
            auto m = l->contentsMargins();
            layouts.append(
                QJsonObject{{"nodeId", it.key()},
                            {"qtLayout", l->metaObject()->className()},
                            {"spacing", l->spacing()},
                            {"padding", QJsonArray{m.left(), m.top(), m.right(), m.bottom()}}});
        }
    }
    for (auto it = layoutNodes.begin(); it != layoutNodes.end(); ++it) {
        auto* l = it.value();
        const auto r = l->geometry();
        const auto m = l->contentsMargins();
        geometry.append(QJsonObject{{"nodeId", it.key()},
                                    {"qtClass", l->metaObject()->className()},
                                    {"geometry", QJsonArray{r.x(), r.y(), r.width(), r.height()}}});
        layouts.append(QJsonObject{{"nodeId", it.key()},
                                   {"qtLayout", l->metaObject()->className()},
                                   {"spacing", l->spacing()},
                                   {"padding", QJsonArray{m.left(), m.top(), m.right(), m.bottom()}}});
    }
    return {{"structureSource", "ui-xml"},
            {"components", components},
            {"adapters", adapters},
            {"layouts", layouts},
            {"geometry", geometry}};
}
}
