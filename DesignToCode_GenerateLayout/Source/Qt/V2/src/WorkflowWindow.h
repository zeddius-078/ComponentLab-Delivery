#pragma once
// 工单演示流程控制器：持有两页，负责输入校验、跳转、返回和演示事件。
#include "UiPage.h"
#include <QStackedWidget>

namespace v2 {
class WorkflowWindow : public QStackedWidget {
    Q_OBJECT
public:
    // dataDirectory 包含页面文件与 workflow.json；resultOverride 可切到 HTML 对照页面前缀。
    explicit WorkflowWindow(const QString& dataDirectory, const QString& resultOverride = {});

    // scene(false) 为输入页，scene(true) 为结果页，便于测试按设计 ID 查询控件。
    UiPage& scene(bool result) {
        return result ? *resultScene : *reviewScene;
    }

    QString workOrder() const {
        return order;
    }

    QJsonArray events() const {
        return log;
    }

signals:
    // 输出被点击的设计节点 ID，实际业务可在外部连接此信号。
    void demoAction(const QString& nodeId);

private:
    void confirm();
    void back();
    void record(const QString& action);
    QJsonObject flow;
    std::unique_ptr<UiPage> reviewScene, resultScene;
    QString order;
    QJsonArray log;
};
}
