# 组件 SDK 上手指南

本项目把设计组件身份、属性、状态和 Token 编译为 Qt 5.15.2 C++ `Z_*` 组件，并发布 `GeneratedComponentSDK`。

## 环境

```powershell
$env:QT_ROOT = '<Qt5 MSVC x64 SDK>'
$env:VS_ROOT = '<Visual Studio 安装目录>'
$env:NINJA_ROOT = '<Ninja 目录>'
$env:CMAKE_EXE = '<cmake.exe>'
```

## 发布与验证

```powershell
pwsh -File Source/Publish-ComponentSDK.ps1
pwsh -File docs/validation/SDKConsumerTest/Build.ps1
pwsh -File docs/examples/MappingWorkbench/Build.ps1 -GenerateTokens -Test -Deploy
python -B Source/CheckProject.py
```

发布结果位于 `build/sdk`，包含公共头文件、静态库、Token 与映射资源、CMake 包和 manifest。映射工作台位于 `build/demo/component_mapping_workbench`，不进入 SDK。

## 开发者接入

```cmake
set(GeneratedComponentSDK_DIR "<SDK目录>/cmake")
find_package(GeneratedComponentSDK 1.0 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE GeneratedComponentSDK::Components)
```

```cpp
#include <MyDesignSystem/MyDesignSystem.h>
```

业务工程只引用公共头和库。新增组件的完整交付至少包括组件合同、Token/QSS、`.h/.cpp`、工厂注册、公共头导出和独立消费测试。
