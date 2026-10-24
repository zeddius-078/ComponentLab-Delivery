# GenerateLayout 源码

本目录是布局生成工具的维护源码，包含采集、适配、编译、构建和校验入口。

| 文件或目录 | 职责 |
| --- | --- |
| `HtmlDsl` | HTML + DSL 采集、适配、组件转换、布局转换和页面编译 |
| `pixso_native` | 独立的原生 Vue 导出静态解析、组件/布局映射及五文件页面生成；不调用 HtmlDsl |
| `pixso_native/dsm/component_parsers.json` | 六个组件族的上游 DSM 名称/导入测试配置；尚未通过实际 D2C⁺ 导出，不自动载入线上配置 |
| `Build.ps1` | 构建、测试并发布布局工具 |
| `CMakeLists.txt` | C++ 目标和测试配置 |
| `GeneratedComponentDependency.cmake` | 引入指定的 GenerateComponent SDK |
| `Verify-GeneratedComponentSDK.ps1` | 校验 SDK manifest、文件和 ABI |
| `NativeTools.ps1` | 定位固定工具并设置当前进程环境 |
| `CheckProject.py` | 项目结构和规则检查 |

工作区编排脚本是上一级 `../Source/Workspace.ps1`（从项目根目录执行时的路径），不在本源码目录内。

源码不保存 EXE、页面、日志或缓存。第一路径稳定工具经构建后发布到 `build/tools/layout`；第二路径目前从维护源码执行，生成和验证写入 `output/work/pixso-native`，通过本批次校验的页面另行交接到 `output/delivery`，尚未发布为受控工具副本。两个路径的输入、实现、映射和工作结果保持分离。

原生 Vue 入口的依赖、参数、export-first 策略及图标回退规则见[第二路径说明](../docs/Pixso原生DSM-D2C到Qt第二路径说明.md)。

组件实现和 Token 由 GenerateComponent 维护。Python 入口使用已有 Python 3.9，依赖缺失时停止并报告。
