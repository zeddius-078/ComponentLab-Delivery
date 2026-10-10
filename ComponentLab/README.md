# ComponentLab

ComponentLab 是已发布组件的展示与验收程序。`Z-DesignSystem` 的组件树按“基础与按钮”等类别组织 62 个 SDK 组件，Button、Checkbox、Input、Select 排在首组最前面。每个树叶只打开一个组件的演示页面，页面包含“外观”“分组与状态”、操作区、预览区和实时对象树。对象树显示层级、几何、常用状态属性和组件动态属性。前四个组件保留完整交互，其余组件使用相同的页面结构，后续可逐个补充专用属性。组件映射页单独列出设计身份、SDK 类及其 Qt 基类。

| 目录 | 职责 |
| --- | --- |
| [Source](Source/README.md) | 演示页面、展示控制器、构建和验证入口 |
| [docs](docs/README.md) | 打开方式、Profile 切换和验收范围 |
| `build/sdk` | 从 GenerateComponent 导入并校验的开发 SDK；不在本项目生成 |
| `build/demo/component_gallery` | 62 个 Z-DesignSystem 组件的离线演示程序 |
| `build/cache` / `build/reports` | 可重建缓存 / 验证结果 |

ComponentLab 可以选择 Profile 或加载完整的外部 Token 变量集。组件树默认选中 Button，单个组件首次打开时通过 `ComponentFactory` 创建。组件类、`.h/.cpp`、QSS、属性合同和 Token 编译均由 GenerateComponent 提供。

```powershell
pwsh -File Source/Import-GeneratedComponentSDK.ps1 -Package '<GeneratedComponentSDK目录>'
pwsh -File Source/delivery/tools/Build.ps1 -CMake '<cmake.exe>'
python -B Source/delivery/tools/CheckProject.py
```

开发项目通过 `#include <MyDesignSystem/MyDesignSystem.h>` 和 `GeneratedComponentSDK::Components` 使用组件。双击 `build/demo/component_gallery/Run.cmd` 打开效果演示。
