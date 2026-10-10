# ComponentLab 源码

| 目录或文件 | 职责 |
| --- | --- |
| `delivery/demo` | Z-DesignSystem 组件树、逐组件演示窗口、对象树和 `.ui` |
| `delivery/tests` | 外观、状态、组合与目录边界测试 |
| `delivery/tools` | 构建、部署、检查和打包入口 |
| `presentation` | 演示专用主题安装、图标样例和组合控制器 |
| `GeneratedComponentDependency.cmake` | 构建时加载已导入 SDK |
| `Import-GeneratedComponentSDK.ps1` | 校验发布者与哈希后复制 SDK 到本项目 build |

本目录不保存组件实现、DesignSystem 源或 Token 编译器。组件能力来自经哈希校验导入的 `build/sdk`。
