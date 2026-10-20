# GenerateLayout 源码

本目录是布局生成工具的维护源码，包含采集、适配、编译、构建和校验入口。

| 文件或目录 | 职责 |
| --- | --- |
| `HtmlDsl` | HTML + DSL 采集、适配、组件转换、布局转换和页面编译 |
| `Build.ps1` | 构建、测试并发布布局工具 |
| `CMakeLists.txt` | C++ 目标和测试配置 |
| `GeneratedComponentDependency.cmake` | 引入指定的 GenerateComponent SDK |
| `Verify-GeneratedComponentSDK.ps1` | 校验 SDK manifest、文件和 ABI |
| `NativeTools.ps1` | 定位固定工具并设置当前进程环境 |
| `CheckProject.py` | 项目结构和规则检查 |
| `Workspace.ps1` | doctor、build、verify、package、clean 编排 |

源码不保存 EXE、页面、日志或缓存。执行结果分别写入 `output/work`、`output/delivery` 和 `output/logs`；稳定工具经构建后发布到 `build/tools/layout`。

组件实现和 Token 由 GenerateComponent 维护。Python 入口使用已有 Python 3.9，依赖缺失时停止并报告。
