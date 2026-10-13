# ComponentLab 源码

| 目录或文件 | 职责 |
| --- | --- |
| `delivery/demo` | Z-DesignSystem 组件树、逐组件演示窗口、对象树和 `.ui` |
| `delivery/tests` | 外观、状态、组合与目录边界测试 |
| `delivery/tools` | 构建、部署、检查和打包入口 |
| `presentation` | 演示专用主题安装、图标样例和组合控制器 |
| `GeneratedComponentDependency.cmake` | 构建时加载工作区唯一权威 SDK |
| `Verify-GeneratedComponentSDK.ps1` | 校验权威 SDK 的发布者与逐文件哈希，不复制文件 |

本目录不保存组件实现、DesignSystem 源、Token 编译器或 SDK 副本。组件能力直接来自 `DesignToCode_GenerateComponent/build/sdk`；外部工作区通过 `GENERATED_COMPONENT_SDK_ROOT` 指定同一权威发布目录。
