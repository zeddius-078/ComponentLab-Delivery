# ComponentLabSourceSDK

Qt 5 组件 **源码交付包**（Source SDK）。不包含预编译库：组件实现随业务工程本地编译。

## 使用方式

### 方式一：add_subdirectory（推荐）

```cmake
add_subdirectory(<本目录> ComponentLabSourceSDK)
target_link_libraries(my_app PRIVATE GeneratedComponentSDK::Components)
```

### 方式二：独立构建

```powershell
cmake -S <本目录> -B <cache> -G Ninja -DCMAKE_PREFIX_PATH=<Qt 5.15>
cmake --build <cache>
```

## 代码引用

```cpp
#include <MyDesignSystem/MyDesignSystem.h>
```

## 说明

- 组件源码、公开头文件、QRC/QSS/图标/字体/Token 资源均来自
  `DesignToCode_GenerateComponent` 的同一事实源（本包 `manifest.json` 记录
  来源哈希与逐文件清单）。
- **不要与二进制 SDK 同时链接**：`GeneratedComponentSDK::Components`
  （`find_package(GeneratedComponentSDK)`）与本包定义同一组组件符号。
- 运行时剖面名：`standard-light` / `standard-dark` / `wide-light` / `wide-dark`。
- MSVC 14.5x + Qt 5.15.2 组合下，本包自动强制包含 `qt5152_msvc_compat.h`。
