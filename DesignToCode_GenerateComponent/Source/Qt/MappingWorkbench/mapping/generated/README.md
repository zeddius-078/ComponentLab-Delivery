# Qt 映射产物

.ui 保存结构，.h/.cpp 装配现有组件，.qss 读取当前运行时调色板。

这些文件使用 DesignToCode 的 mapping_core、私有 ComponentKit 和两份 QRC。
CMake 开启 AUTOMOC、AUTOUIC；把 MappingGenerated.cpp 和 MappingGenerated.ui 加入目标，链接 mapping_core。
创建 QApplication 后先调用 component_theme::install(app, profile)，再创建 MappingGenerated(profile)。

MappingGenerated.qss 是 mapping/page.qss 的可读副本；编译后的 .cpp 从 QRC 读取同一份页面样式。
组件 QSS 和材质仍由既有组件运行时读取。改变导出副本不会更改内嵌资源。
Qt Designer 展示结构占位；真实组件在装配后出现。

ui-contract.json 是本次输入，mapping-report.json 给出来源、依赖及文件哈希。
完整接入步骤见项目 `docs/mainSkill/generate-component-pipeline/references/设计组件映射指南.md`。
