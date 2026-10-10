#pragma once
#include <QString>

class QApplication;

namespace component_theme {
// 应用创建控件前调用一次。只消费已生成 Token，不读取设计稿、映射协议或 Demo 配置。
void install(QApplication &application, const QString &profile);
QString activeProfile();
} // namespace component_theme
