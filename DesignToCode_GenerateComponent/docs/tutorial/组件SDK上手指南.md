# 组件 SDK 上手指南

本项目把设计组件身份、属性、状态和 Token 编译为 Qt 5.15.2 C++ `Z_*` 组件，并发布 `GeneratedComponentSDK`。

## 固定环境

遵循工作区 AGENTS.md，不安装、升级、降级或自动补包。以下变量仅指定已有安装路径，不是安装步骤；需要变更环境先问用户。Python 脚本目标 3.9，兼容测试不等于真实 3.9 实测。

## 已有工具路径

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

## 执行与结果边界

PrepareBuild.py 先把维护输入复制到 build/work，再生成与编译。仅准备输入使用 `python -B Source/PrepareBuild.py --copy-only`；不加参数会执行生成。不要在 Source 内直接运行写出文件的生成器，也不要编辑 build/work 当作维护源。

导航栏 container.height Token 消费问题按用户决定挂起；上述命令是验证流程，不是全链通过声明。只做文档或目标库类名合同变更无需发布 SDK。Analysis 类名合同由 GenerateLayout 管理，不在这里开发或登记假实现。

Python 兼容回归：`python -B -m unittest discover -s docs/validation -p test_python39_writes.py -v`。长期记录归档 docs/archive；build/reports 可删除并重建。
