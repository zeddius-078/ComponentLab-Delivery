# ComponentLab 源码

## 文件职责

| 路径 | 内容 |
| --- | --- |
| `delivery/demo` | 展示窗口、组件页、对象树和页面资源 |
| `presentation` | 展示侧状态控制和演示编排 |
| `delivery/tests` | Qt Test 与项目边界检查 |
| `delivery/tools` | 构建、部署、验证和打包入口 |
| `CMakeLists.txt` | ComponentLab 目标、测试和安装规则 |

源码引用 `GeneratedComponentSDK::Components`。组件实现和 Token 编译位于 GenerateComponent，页面生成位于 GenerateLayout。

## 修改方式

展示改动写在 `delivery/demo` 或 `presentation`；构建入口改在 `delivery/tools`。构建目录和 output 目录中的文件属于执行产物，修改不会成为维护源。

## 验证

在工作区根目录运行 `pwsh -File Source/Workspace.ps1 -Action verify`，或先校验 SDK，再运行本项目的 Build 和 Verify 脚本。
