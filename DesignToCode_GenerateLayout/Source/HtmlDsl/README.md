# HTML + DSL 转换链路

HTML 提供浏览器渲染后的布局和文字测量，DSL 提供设计节点身份、层级、属性、状态和 Token 引用。正式输入必须来自同一设计版本，并由 capture manifest 绑定。

| 内容 | 作用 |
| --- | --- |
| `Generate.ps1` | 正式页面生成和显式预览入口 |
| `PreviewStructure.py` | 结构预览与初始样式 |
| `pipeline` | 清单、适配、组件转换、布局转换和页面编译 |
| `tools` | 浏览器测量、路径保护和辅助工具 |

正式生成输出 `.ui/.h/.cpp/.qss/.qrc`；结构预览输出结构页和初始样式。两种模式分别验收，预览不替代组件合同和正式页面验收。

公共映射在 `docs/fixtures/layout/mapping`，执行结果在 `output`。Commerly 作为固定输入案例保留，不写入生成器分支。
