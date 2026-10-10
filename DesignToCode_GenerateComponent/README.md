# DesignToCode_GenerateComponent

本项目是组件与 Token 的唯一产出方。它将设计系统中的组件身份、属性、状态和 Token 映射为 C++ Qt 组件，并发布给其他项目使用。

当前能力目录以 Qt 5.15.2 为基线，共登记 86 个 Qt 类型。其中 59 个具体可视控件全部提升为 `Z_*` 组件并进入 SDK 工厂；其余 27 个抽象基类、布局、模型、行为和支持类型登记工程职责。Qt 元对象扫描已核对 400 个 `Q_PROPERTY`，报告中的错误数为 0。

组件命名固定为 `Q<Class> -> commonWidget::Z_<Class>`，并保持 Qt 类名的大小写。

| Qt 原生类 | 开发侧组件 | 组件源码 | Token 样式 |
| --- | --- | --- | --- |
| `QPushButton` | `Z_PushButton` | `runtime/component/Z_PushButton.h/.cpp` | `generated/button/<profile>/Z_PushButton.qss` |
| `QCheckBox` | `Z_CheckBox` | `runtime/component/Z_CheckBox.h/.cpp` | `generated/checkbox/<profile>/Z_CheckBox.qss` |
| `QLineEdit` | `Z_LineEdit` | `runtime/component/native/Z_LineEdit.h/.cpp` | `generated/native/<profile>/Z_LineEdit.qss` |
| `QTableView` | `Z_TableView` | `runtime/component/native/Z_TableView.h/.cpp` | `generated/native/<profile>/Z_TableView.qss` |

59 个组件的完整定义位于 `Source/ComponentKit/component-definitions/native-components.json`。`tools/generate_native_components.py` 从该文件确定性生成独立源码、QSS 模板、四套 Profile QSS 和聚合头文件。Button 与 Checkbox 保留专用实现，其余组件使用独立薄包装文件继承 Qt 行为，并预留项目扩展位置。

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
| `Source/ComponentKit/component-definitions` | 59 个 Qt 可视组件的命名、设计身份、类别和实现方式 |
| `Source/ComponentKit/runtime/component/native` | 每个原生提升组件独立的 `.h/.cpp` 与聚合头文件 |
| `Source/ComponentKit/runtime` | 专用组件、原生提升组件、绘制、状态和主题加载 |
| [Source/DesignEvidence](Source/DesignEvidence/README.md) | Pixso Raw 审计、Canonical 工程合同、归档与采集/标准化/校验工具 |
| `Source/Qt/MappingWorkbench` | 设计组件身份、属性值域与 Qt 类/API 的映射演示 |
| `build/sdk` | 供开发项目导入的产品化组件 SDK |
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

组件或 Token 变化后先重新发布 SDK，再由两个消费项目运行各自的 `Import-GeneratedComponentSDK.ps1`。
