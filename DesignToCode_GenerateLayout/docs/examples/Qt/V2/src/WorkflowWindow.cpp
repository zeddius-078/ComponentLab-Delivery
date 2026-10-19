// 页面行为由 workflow.json 中的节点 ID 绑定，视觉初始值从静态页面清单读取。
#include "WorkflowWindow.h"
#include "Z_PushButton.h"
#include "Z_InputField.h"
#include <QLineEdit>
#include <QLayout>
#include <QDateTime>
#include <stdexcept>

namespace v2 {
using commonWidget::Z_InputField;
using commonWidget::Z_PushButton;

WorkflowWindow::WorkflowWindow(const QString& directory, const QString& resultOverride) {
    // 两页共享组件注册表；根节点及具体控件类型不一致时，在交互前报告错误。
    flow = readJson(directory + "/workflow.json");
    auto registry = std::make_shared<WidgetRegistry>(
        readJson(directory + "/review.components.json")["profile"].toString());
    reviewScene = std::make_unique<UiPage>(directory + "/review", registry);
    resultScene = std::make_unique<UiPage>(
        resultOverride.isEmpty() ? directory + "/result" : resultOverride, registry);
    // 页面根 ID 必须与流程配置一致，防止入口页或结果页加载错文件后仍建立交互。
    if (reviewScene->rootId() != flow["entry"].toString() ||
        resultScene->rootId() != flow["result"].toString())
        throw std::runtime_error("FLOW_ROOT_MISMATCH");
    addWidget(reviewScene->load(this));
    addWidget(resultScene->load(this));
    auto* input = qobject_cast<Z_InputField*>(reviewScene->widget(flow["input"].toString()));
    auto* submit = qobject_cast<Z_PushButton*>(reviewScene->widget(flow["confirm"].toString()));
    auto* returnButton = qobject_cast<Z_PushButton*>(resultScene->widget(flow["back"].toString()));
    // 流程 ID 必须绑定正确控件类型，否则连接信号会产生空指针或无效交互。
    if (!input || !submit || !returnButton)
        throw std::runtime_error("FLOW_BINDING_CLASS_MISMATCH");
    connect(submit, &QAbstractButton::clicked, this, &WorkflowWindow::confirm);
    connect(input, &Z_InputField::submitted, this, [this](const QString&) {
        confirm();
    });
    connect(input, &Z_InputField::valueChanged, this, [input] {
        input->setStatus({});
        input->lineEdit()->setAccessibleDescription({});
    });
    connect(returnButton, &QAbstractButton::clicked, this, &WorkflowWindow::back);
    // 样例按钮只记录并发出事件，尚未接入工单提交等业务服务。
    // 逐个绑定声明的演示按钮，只发出本地事件，避免猜测未定义业务动作。
    for (const auto& value : flow["demoActions"].toArray()) {
        const auto id = value.toString();
        auto* button = qobject_cast<Z_PushButton*>(resultScene->widget(id));
        // 演示动作必须指向按钮；错误映射在窗口展示前暴露。
        if (!button)
            throw std::runtime_error("DEMO_ACTION_BINDING_MISMATCH");
        connect(button, &QAbstractButton::clicked, this, [this, id, button] {
            record(id);
            setWindowTitle("V2 / " + order + " / " + button->text() + " 已点击");
            emit demoAction(id);
        });
    }
    setWindowTitle("工单复核 / V1 Holdout → V2");
    resize(qMax(widget(0)->width(), widget(1)->width()),
           qMax(widget(0)->height(), widget(1)->height()));
    // 用户切换前完成两页创建、样式初始化和布局，避免切换后再首次构造页面。
    for (int i = 0; i < count(); ++i) {
        widget(i)->resize(size());
        widget(i)->ensurePolished();
        widget(i)->layout()->activate();
    }
    setCurrentIndex(0);
}

void WorkflowWindow::record(const QString& action) {
    log.append(
        QJsonObject{{"action", action},
                    {"timeUtc", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}});
}

void WorkflowWindow::confirm() {
    // 空值留在输入页并设置错误提示；通过后只更新运行状态及当前页。
    auto* input = qobject_cast<Z_InputField*>(reviewScene->widget(flow["input"].toString()));
    const auto value = input->value().trimmed();
    // 必填工单去空格后为空时保留当前页并提示；通过后才保存工单并跳转。
    if (flow["required"].toBool() && value.isEmpty()) {
        input->setStatus("error");
        input->lineEdit()->setAccessibleDescription("请输入工单编号");
        input->lineEdit()->setFocus();
        setWindowTitle("工单复核 / 请输入工单编号");
        record("required-empty");
        return;
    }
    order = value;
    record("confirm-review");
    setCurrentIndex(1);
    setWindowTitle("工单列表 / " + order);
    resultScene->widget(flow["back"].toString())->setFocus(Qt::OtherFocusReason);
}

void WorkflowWindow::back() {
    // 复用原页面保留输入值，返回后将焦点交回编辑框。
    setCurrentIndex(0);
    record("back");
    setWindowTitle("工单复核 / V1 Holdout → V2");
    qobject_cast<Z_InputField*>(reviewScene->widget(flow["input"].toString()))
        ->lineEdit()
        ->setFocus(Qt::OtherFocusReason);
}
}
