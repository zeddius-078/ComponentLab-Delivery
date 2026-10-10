#pragma once
#include <MyDesignSystem/MyDesignSystem.h>
#include <QJsonObject>

namespace button_demo {
void install(QApplication& app, const QString& profile);
QIcon icon(const QString& assetId);
QJsonObject manifest();
QString readResource(const QString& path);
QString validate(const commonWidget::Z_PushButton& button);
} // namespace button_demo
