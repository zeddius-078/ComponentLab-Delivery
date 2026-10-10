#pragma once
// 单页实验装配器：页面布局来自 XML，只负责打开页面、记录按钮事件和可复现检查。
#include "UiPage.h"
#include <QJsonObject>
#include <QString>
#include <memory>

class CaseWindow {
public:
    explicit CaseWindow(const QString& dataDirectory);
    void show();
    QJsonObject verify(const QString& reportDirectory);

private:
    QString data;
    QJsonObject manifest;
    std::shared_ptr<v2::WidgetRegistry> registry;
    std::unique_ptr<v2::UiPage> scene;
    std::unique_ptr<QWidget> window;
    int clickCount = 0;
};
