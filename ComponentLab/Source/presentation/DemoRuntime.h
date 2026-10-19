#pragma once
#include <MyDesignSystem/MyDesignSystem.h>

namespace button_demo {
void install(QApplication& app, const QString& profile);
QIcon icon(const QString& assetId);
QString validate(const commonWidget::Z_PushButton& button);
} // namespace button_demo
