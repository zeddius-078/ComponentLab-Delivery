# 组件 SDK 上手指南

## 1. 检查输入

组件维护输入位于 `Source/ComponentKit`。开始前检查 Registry、Contract、Recipe、Binding、Token 源和组件源码是否在同一版本。

```powershell
py -3.9 -B Source/CheckProject.py --source-only
```

## 2. 发布 SDK

```powershell
pwsh -File Source/Publish-ComponentSDK.ps1 -QtRoot <Qt5目录>
```

发布流程包括：准备输入副本、生成 Token 和 QSS、配置 CMake、编译组件、校验 manifest、执行独立消费者测试，然后原子更新 `build/sdk`。

## 3. 业务工程接入

```cmake
set(GeneratedComponentSDK_DIR "<SDK目录>/cmake")
find_package(GeneratedComponentSDK 1.0 CONFIG REQUIRED)
target_link_libraries(app PRIVATE GeneratedComponentSDK::Components)
```

```cpp
#include <MyDesignSystem/MyDesignSystem.h>

auto *button = new commonWidget::Z_PushButton(parent);
```

业务工程使用公开头文件、CMake target 和 SDK 资源。组件私有源码、生成副本和生产方路径不进入业务工程。

## 4. SDK 校验

```powershell
py -3.9 -B Source/SDKPackage.py verify --sdk build/sdk --configuration Release --project-root .
```

校验 manifest 的版本、工具链、配置、来源指纹、文件列表和逐文件哈希。ComponentLab 与 GenerateLayout 还会执行各自的消费侧校验。

## 5. 维护边界

| 需求 | 修改位置 |
| --- | --- |
| Token 原子值、主题和 Profile | `Source/ComponentKit/design-assets/DesignSystem/source` |
| 组件合法组合和状态 | Contract、Registry、Recipe |
| 设计变量绑定 | `source/figma-bindings` |
| Qt 类、属性和 API 映射 | `Source/Qt/MappingWorkbench` 和映射配置 |
| SDK 发布 | `Source/Publish-ComponentSDK.ps1` |

ComponentLab 只展示 SDK；GenerateLayout 只消费 SDK 并装配页面。
