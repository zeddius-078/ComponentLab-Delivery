#include "CaseWindow.h"
#include <QAbstractButton>
#include <QApplication>
#include <QDir>
#include <QJsonArray>
#include <QLayout>
#include <QMap>
#include <QListView>
#include <QTest>
#include <cmath>
#include <stdexcept>

namespace {
void require(bool condition, const char* reason) {
    // 明确失败原因并终止检查，禁止只输出截图就声明转换通过。
    if (!condition)
        throw std::runtime_error(reason);
}
}

CaseWindow::CaseWindow(const QString& directory)
    : data(directory), manifest(v2::readJson(directory + "/page.components.json")),
      registry(std::make_shared<v2::WidgetRegistry>(manifest["profile"].toString())),
      scene(std::make_unique<v2::UiPage>(directory + "/page", registry)), window(scene->load()) {
    window->setWindowTitle("Pixso 27:450 / HTML + DSL / Qt XML");
    // 根据生成清单寻找按钮，不在宿主中手工列举节点或改变组件结构。
    const auto nodes = manifest["nodes"].toObject();
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        auto* button = qobject_cast<QAbstractButton*>(scene->widget(it.key()));
        // 普通标题、容器和选项视图不绑定按钮事件，保留各自原生行为。
        if (!button)
            continue;
        QObject::connect(button, &QAbstractButton::clicked, window.get(), [this, button] {
            ++clickCount;
            window->setWindowTitle(
                QString("27:450 / %1 / clicked %2").arg(button->text()).arg(clickCount));
        });
    }
}

void CaseWindow::show() {
    window->show();
    window->ensurePolished();
    window->layout()->activate();
}

QJsonObject CaseWindow::verify(const QString& reportDirectory) {
    QDir().mkpath(reportDirectory);
    const auto expected = v2::readJson(data + "/layout-check.json");
    const auto design = v2::readJson(data + "/page.design-tree.json");
    QMap<QString, QString> parents;
    for (const auto& value : design["nodes"].toArray()) {
        const auto node = value.toObject();
        parents[node["nodeId"].toString()] = node["parentId"].toString();
    }
    auto trace = scene->trace();
    QJsonArray comparisons, interactions;
    double maxDelta = 0;
    // 每个设计节点都与 HTML 浏览器测量结果比较，坐标统一为父节点局部坐标。
    for (const auto& value : expected["bindings"].toArray()) {
        const auto binding = value.toObject();
        const auto id = binding["guid"].toString();
        auto* widget = scene->widget(id);
        auto* layout = scene->layout(id);
        require((widget != nullptr) != (layout != nullptr), "MISSING_RUNTIME_NODE");
        auto rect = widget ? widget->geometry() : layout->geometry();
        if (auto* parentLayout = scene->layout(parents.value(id)))
            rect.translate(-parentLayout->geometry().topLeft());
        const auto target = binding["htmlRect"].toArray();
        // 顶层窗口位置由操作系统决定，仅比较其内容原点及设计尺寸。
        const QJsonArray actual{id == scene->rootId() ? 0 : rect.x(),
                                id == scene->rootId() ? 0 : rect.y(), rect.width(), rect.height()};
        double delta = 0;
        // 对 x、y、width、height 四个量使用同一规则，不允许遗漏尺寸变化。
        for (int i = 0; i < 4; ++i)
            delta = qMax(delta, std::abs(actual[i].toDouble() - target[i].toDouble()));
        maxDelta = qMax(maxDelta, delta);
        comparisons.append(QJsonObject{
            {"nodeId", id}, {"expected", target}, {"actual", actual}, {"maxDelta", delta}});
    }
    // 检查生成参数中的文字、启用状态以及按钮真实鼠标点击响应；不定义业务提交行为。
    for (const auto& value : manifest["components"].toArray()) {
        const auto component = value.toObject();
        auto* widget = scene->widget(component["nodeId"].toString());
        auto* button = qobject_cast<QAbstractButton*>(widget);
        // 这里只验证按钮；同一清单中的选项适配器在下方按 adapter 字段筛选检查。
        if (!button)
            continue;
        // 生成清单提供断言目标，避免在测试中重复硬编码各按钮的文本与状态。
        for (const auto& callValue : component["calls"].toArray()) {
            const auto call = callValue.toObject();
            // 仅文字 setter 对应文本断言，其余方法由各自状态检查覆盖。
            if (call["api"] == "setText")
                require(button->text() == call["value"].toString(), "BUTTON_TEXT_MISMATCH");
            // 显式启用状态必须与实际控件一致，尤其用于验证设计中的禁用提交按钮。
            if (call["api"] == "setEnabled")
                require(button->isEnabled() == call["value"].toBool(), "BUTTON_ENABLED_MISMATCH");
        }
        const int before = clickCount;
        QTest::mouseClick(button, Qt::LeftButton);
        qApp->processEvents();
        const bool responded = clickCount != before;
        require(responded == button->isEnabled(), "BUTTON_CLICK_POLICY_MISMATCH");
        interactions.append(QJsonObject{{"nodeId", component["nodeId"]},
                                        {"kind", "button"},
                                        {"enabled", button->isEnabled()},
                                        {"clicked", responded},
                                        {"passed", true}});
    }
    // 选项适配器的根是 QWidget；列表位于其内部，需验证模型和真实点击后的选择结果。
    int optionsChecked = 0;
    for (const auto& value : manifest["components"].toArray()) {
        const auto adapter = value.toObject();
        // 该清单同时含按钮和选项，只有声明此适配器的项才使用列表模型检查。
        if (adapter["adapter"] != "SelectOptionAdapter")
            continue;
        require(adapter["state"] == "disabled", "UNSUPPORTED_OPTION_STATE_IN_CASE");
        auto* widget = scene->widget(adapter["nodeId"].toString());
        require(widget != nullptr, "OPTION_WIDGET_MISSING");
        auto* options = widget->findChild<QListView*>(QString(), Qt::FindDirectChildrenOnly);
        require(options && options->model(), "OPTION_MODEL_MISSING");
        require(options->model()->rowCount() == 1, "OPTION_ROW_COUNT_MISMATCH");
        const auto index = options->model()->index(0, 0);
        require(!(options->model()->flags(index) & Qt::ItemIsEnabled),
                "OPTION_DISABLED_STATE_MISMATCH");
        require(index.data(Qt::DisplayRole).toString() == adapter["label"].toString(),
                "OPTION_LABEL_MISMATCH");
        options->selectionModel()->clearSelection();
        QTest::mouseClick(options->viewport(), Qt::LeftButton, Qt::NoModifier,
                          options->visualRect(index).center());
        qApp->processEvents();
        require(options->selectionModel()->selectedIndexes().isEmpty(),
                "DISABLED_OPTION_ACCEPTED_SELECTION");
        ++optionsChecked;
        interactions.append(QJsonObject{{"nodeId", adapter["nodeId"]},
                                        {"kind", "disabled-option"},
                                        {"selectedAfterClick", false},
                                        {"passed", true}});
    }
    require(optionsChecked == 4, "OPTION_CHECK_COVERAGE_MISMATCH");
    // 截图统一去掉焦点和悬停，避免测试动作改变对照状态。
    if (qApp->focusWidget())
        qApp->focusWidget()->clearFocus();
    QTest::mouseMove(window.get(), QPoint(4, 4));
    QTest::qWait(400);
    require(window->grab().save(reportDirectory + "/qt.png"), "SCREENSHOT_WRITE_FAILED");
    trace["comparisons"] = comparisons;
    trace["interactions"] = interactions;
    trace["maxGeometryDelta"] = maxDelta;
    trace["status"] = maxDelta == 0 ? "PASS" : "GEOMETRY_MISMATCH";
    trace["platform"] = QGuiApplication::platformName();
    trace["rootSize"] = QJsonArray{window->width(), window->height()};
    // 更大窗口只检查节点仍在容器范围内；该测试不声明设计已定义新的响应式布局。
    const QSize original = window->size();
    window->resize(original + QSize(200, 120));
    qApp->processEvents();
    bool contained = true;
    const auto nodes = manifest["nodes"].toObject();
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        auto* widget = scene->widget(it.key());
        if (!widget)
            continue;
        // 窗口自身没有页面内父容器，边界检查仅覆盖内部节点。
        if (widget != window.get())
            contained = contained && widget->parentWidget()->rect().contains(widget->geometry());
    }
    trace["largerViewportContained"] = contained;
    window->resize(original);
    require(contained, "RESIZED_NODE_OUTSIDE_PARENT");
    return trace;
}
