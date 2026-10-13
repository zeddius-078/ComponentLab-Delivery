# DesignToCode_GenerateComponent

本项目是组件与 Token 的唯一产出方。它将设计系统中的组件身份、属性、状态和 Token 映射为 C++ Qt 组件，并发布给其他项目使用。

当前能力目录以 Qt 5.15.2 为基线，共登记 86 个 Qt 类型。其中 59 个具体可视控件全部提升为 `Z_*` 组件并进入 SDK 工厂；其余 27 个抽象基类、布局、模型、行为和支持类型登记工程职责。当前 Figma 结构清单包含 9 个章节、73 个组件规范页、123 个组件集和 1471 个 Variant。开发映射目录另有 85 个公共组件身份：59 个 Native、6 个 Design System、20 个 Composite；85 项均有 SDK 运行时类与工厂构造入口，不再存在 `instantiable=false` 的公共类型。Figma 结构统计与开发映射统计是两条不同维度；可实例化也不等于已获得设计审批。

Native 命名固定为 `Q<Class> -> commonWidget::Z_<Class>`，并保持 Qt 类名大小写；Design System 与 Composite 使用 `ComponentLab/<Name> -> commonWidget::Z_<Name>`。现有兼容例外为 `ComponentLab/Input -> commonWidget::Z_InputField`。

| Qt 原生类 | 开发侧组件 | 组件源码 | Token 样式 |
| --- | --- | --- | --- |
| `QPushButton` | `Z_PushButton` | `runtime/component/Z_PushButton.h/.cpp` | `generated/button/<profile>/Z_PushButton.qss` |
| `QCheckBox` | `Z_CheckBox` | `runtime/component/Z_CheckBox.h/.cpp` | `generated/checkbox/<profile>/Z_CheckBox.qss` |
| `QLineEdit` | `Z_LineEdit` | `runtime/component/native/Z_LineEdit.h/.cpp` | `generated/native/<profile>/Z_LineEdit.qss` |
| `QTableView` | `Z_TableView` | `runtime/component/native/Z_TableView.h/.cpp` | `generated/native/<profile>/Z_TableView.qss` |

59 个 Native 组件的完整定义位于 `Source/ComponentKit/component-definitions/native-components.json`。Figma MCP 回读事实固化在 `figma-component-inventory.json`；`tools/generate_design_component_catalog.py` 将其与 85 项开发映射合并到 `generated-design-component-catalog.json`，并生成 SDK 的 C++ 查询接口。`tools/generate_native_components.py` 继续生成独立源码、QSS 模板、四套 Profile QSS 和聚合头文件。

组件注册结果输出为正式源码 `runtime/component/native/NativeComponentRegistry.h/.cpp`，其中包含 Qt 类、`Z_*` 类、设计组件身份和构造入口。项目不生成或编译 `.def` 清单。

组件 Token 解析链保持在本项目内：

```text
design-assets/tokens/*.json
        |
        +-- export_tokens.py                 规范化公共 Token 与 Profile
        +-- token_compiler.cpp               Button Token / QSS
        +-- checkbox_token_compiler.cpp      Checkbox Token / QSS
        +-- forms_token_compiler.cpp         Input / Select Token / QSS
        v
design-assets/generated
        v
build/sdk/resources/tokens/default
```

`Publish-ComponentSDK.ps1` 依次生成 Token、生成 59 个组件的源码与 QSS、检查生成漂移、编译静态库并发布 SDK。ComponentLab 与 GenerateLayout 只消费发布结果。

| 目录 | 职责 |
| --- | --- |
| `Source/ComponentKit/design-assets` | Token 输入、组件合同、QSS 模板和编译工具 |
| `Source/ComponentKit/component-definitions` | 59 个 Qt 可视组件定义、85 项开发映射，以及 73 页/123 集/1471 Variant 的 Figma 结构快照 |
| `Source/ComponentKit/runtime/component/native` | 每个原生提升组件独立的 `.h/.cpp` 与聚合头文件 |
| `Source/ComponentKit/runtime` | 专用组件、原生提升组件、绘制、状态和主题加载 |
| [Source/DesignEvidence](Source/DesignEvidence/README.md) | Pixso Raw 审计、Canonical 工程合同、归档与采集/标准化/校验工具 |
| `Source/Qt/MappingWorkbench` | 设计组件身份、属性值域与 Qt 类/API 的映射演示 |
| `build/sdk` | 全工作区唯一权威组件 SDK 发布目录 |
| `build/demo/component_mapping_workbench` | 组件身份、属性、状态和 Qt 映射的效果演示 |
| [docs](docs/README.md) | 组件映射规则、技术分层和构建说明 |

`build/sdk` 包含 `include/MyDesignSystem`、`lib`、`resources`、`cmake` 和发布清单，不包含 EXE 或 Qt 运行 DLL。开发项目只需包含 `MyDesignSystem/MyDesignSystem.h` 并链接 `GeneratedComponentSDK::Components`。

全量登记表位于 `Source/Qt/MappingWorkbench/mapping/qt-native-catalog.json`。`build/reports/Workbench/qt-native-meta-report.json` 保存本机 Qt 5.15.2 元对象核对结果。Ant Design 6.6.5 只提供设计侧通用组件语义参考，不进入运行依赖，也不替代 Qt 元数据与项目 Token 合同。

```cmake
set(GeneratedComponentSDK_DIR "<SDK目录>/cmake")
find_package(GeneratedComponentSDK 1.0 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE GeneratedComponentSDK::Components)
```

```cpp
#include <MyDesignSystem/MyDesignSystem.h>
```

焦点环由全局输入模式与逐组件开关共同决定。保留键盘焦点和 Space/Enter 激活、只隐藏焦点环时使用：

```cpp
commonWidget::FocusIndicator::instance().setEnabledFor(widget, false);
// 或经工厂配置：setFocusIndicatorEnabled / setFocusRingEnabled = false
```

Qt 5 原生没有“保留焦点但仅隐藏焦点环”的统一 QWidget 属性；如果组件本身不应进入键盘焦点链，可使用 `widget->setFocusPolicy(Qt::NoFocus)`。两者语义不同。

```powershell
$env:QT_ROOT = '<Qt5 MSVC x64 SDK>'
$env:VS_ROOT = '<Visual Studio 安装目录>'
$env:NINJA_ROOT = '<Ninja 目录>'
$env:CMAKE_EXE = '<cmake.exe>'

pwsh -File Source/Publish-ComponentSDK.ps1
pwsh -File Source/SDKConsumerTest/Build.ps1
pwsh -File Source/Qt/MappingWorkbench/Build.ps1 -GenerateTokens -Test -Deploy
python -B Source/CheckProject.py
```

组件或 Token 变化后先重新发布 SDK，再由两个消费项目运行各自的 `Verify-GeneratedComponentSDK.ps1`。消费项目直接引用该发布目录，不保存 SDK 副本。
