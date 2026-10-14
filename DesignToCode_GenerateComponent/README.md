# DesignToCode_GenerateComponent

本项目是组件与 Token 的唯一产出方。它将设计系统中的组件身份、属性、状态和 Token 映射为 C++ Qt 组件，并发布给其他项目使用。

当前能力目录以 Qt 5.15.2 为基线，共登记 86 个 Qt 类型。其中 59 个具体可视控件提升为 `Z_*` 组件并进入 SDK 工厂；其余 27 个抽象基类、布局、模型、行为和支持类型登记工程职责。当前 Figma 结构清单包含 10 个章节、98 个组件规范页、148 个组件集和 1608 个 Variant。融合后的权威目录包含 110 个库条目：105 个 Widget/Composite 组件身份、2 个 LayoutHelper 和 3 个 Asset。公开 C++ Catalog 通过 `designLibraryEntries()` 暴露全量条目和 `kind`，通过 `designComponentDescriptors()` 仅暴露 105 个组件合同；LayoutHelper 和 Asset 不进入组件 Factory。105 个组件身份现均可实例化并按规范身份注册，当前组件工程缺口为 0。`Transfer` 已按 Default／Disabled／Loading 合同实现为受控双列表选择 Composite：SDK 组合两个 QListView 与方向命令并接收外部 QAbstractItemModel，页面选择模型/controller 持有集合查询、移动、排序、权限与错误恢复。`Upload` 已按 Default／Hover／Uploading／Success／Error 合同实现为受控文件获取与生命周期面板：SDK 发出浏览或本地文件拖放请求，页面和上传服务持有文件选择策略、校验、传输、鉴权、取消与重试。三个资产条目均已形成 SDK 公共目录：`IconAssetCatalog` 提供图标稳定语义 ID、元数据和 QRC 解析，`VisualAssetCatalog` 提供 `ImageAsset` 与二维 `ModelAsset` 的状态资源、适配方式、完整性摘要和 Profile 渲染。`Timeline` 已按 Figma 的 Comfortable／Compact 密度合同实现为只读事件历史，使用 SDK 内部 Figma SVG 状态标记，不承担 Stepper 的流程导航职责。`AnchorNavigation` 已按 Vertical／Horizontal × Default／Scrolled 合同实现为受控页内导航：SDK 发出锚点激活请求，页面拥有滚动生命周期并通过 `currentIndex` 回传当前章节。`Filter` 已按 Inline／Panel × Default／Applied 合同实现为受控条件组合面板：SDK 只管理可见结构与激活请求，条件模型、校验、清除、持久化和状态转换仍由外部 filter controller 持有。`ColorPicker` 已按 Default／Focus／Disabled 合同实现为独立的可聚焦颜色字段适配器：SDK 显示受控 QColor 并发出选择流程请求，不与 `Z_ColorDialog` 合并；颜色空间、Alpha、最近使用、预览与提交／取消由外部 color-selection controller 持有。`NumericKeypad` 已按 Comfortable／Compact 合同实现为 3×4 受控语义键盘：SDK 只输出语义键、退格和可选提交请求，目标输入框、locale 解析、数值约束、焦点路由与提交策略由外部 input controller 持有。Drawer 的规范 SDK/Factory 键为 `commonWidget::Z_Drawer`；旧 `Z_OverlayDrawer` 类型与 Factory 字符串暂作为轻量兼容入口，不重复计数。目录登记、可实例化、资产可消费与设计审批是彼此独立的维度。

Badge、StatusIndicator、Link、Tag、Tooltip 已按当前 Figma Component Set ID、属性轴和文本槽接入 GenerateLayout。Link 的 `snapshotState` 仅用于静态生成与证据态，空值恢复 Qt 原生交互判定；Tooltip 的目标解析以及 Tag 的删除、持久化和撤销仍由生成页面负责。

Native 命名固定为 `Q<Class> -> commonWidget::Z_<Class>`，并保持 Qt 类名大小写；Design System 与 Composite 使用 `ComponentLab/<Name> -> commonWidget::Z_<Name>`。现有兼容例外为 `ComponentLab/Input -> commonWidget::Z_InputField`。

| Qt 原生类 | 开发侧组件 | 组件源码 | Token 样式 |
| --- | --- | --- | --- |
| `QPushButton` | `Z_PushButton` | `runtime/component/Z_PushButton.h/.cpp` | `generated/button/<profile>/Z_PushButton.qss` |
| `QCheckBox` | `Z_CheckBox` | `runtime/component/Z_CheckBox.h/.cpp` | `generated/checkbox/<profile>/Z_CheckBox.qss` |
| `QLineEdit` | `Z_LineEdit` | `runtime/component/native/Z_LineEdit.h/.cpp` | `generated/native/<profile>/Z_LineEdit.qss` |
| `QTableView` | `Z_TableView` | `runtime/component/native/Z_TableView.h/.cpp` | `generated/native/<profile>/Z_TableView.qss` |

59 个 Native 组件的完整定义位于 `Source/ComponentKit/component-definitions/native-components.json`。Figma MCP 回读事实固化在 `figma-component-inventory.json`；`tools/generate_design_component_catalog.py` 将实时 Figma 清单、融合追踪表和已有工程映射合并到 `generated-design-component-catalog.json`，并生成 SDK 的 C++ 查询接口。`tools/generate_native_components.py` 继续生成独立源码、QSS 模板、四套 Profile QSS 和聚合头文件。

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
| `Source/ComponentKit/component-definitions` | 59 个 Qt 可视组件定义、110 项融合目录，以及 98 页/148 集/1608 Variant 的 Figma 结构快照 |
| `Source/ComponentKit/runtime/component/native` | 每个原生提升组件独立的 `.h/.cpp` 与聚合头文件 |
| `Source/ComponentKit/runtime` | 专用组件、原生提升组件、绘制、状态和主题加载 |
| [Source/DesignEvidence](Source/DesignEvidence/README.md) | Pixso Raw 审计、Canonical 工程合同、归档与采集/标准化/校验工具 |
| `Source/Qt/MappingWorkbench` | 设计组件身份、属性值域与 Qt 类/API 的映射演示 |
| `build/sdk` | 全工作区唯一权威组件 SDK 发布目录 |
| `build/demo/component_mapping_workbench` | 组件身份、属性、状态和 Qt 映射的效果演示 |
| [docs](docs/README.md) | 组件映射规则、技术分层和构建说明 |

`build/sdk` 包含 `include/MyDesignSystem`、`lib`、`resources`、`cmake` 和发布清单，不包含 EXE 或 Qt 运行 DLL。开发项目只需包含 `MyDesignSystem/MyDesignSystem.h` 并链接 `GeneratedComponentSDK::Components`。公共图标使用 `icon.action.search` 等稳定 ID，由 `IconAssetCatalog::resolve()` 解析；图片和二维原理图使用 `image.foundation.geometric-preview`、`model.foundation.schematic`，由 `VisualAssetCatalog` 解析和渲染。未知 ID 明确抛出 `UNMAPPED_RESOURCE`；已知但无图像载荷的 Empty/Error 状态抛出 `NO_ASSET_PAYLOAD`。消费者不应直接依赖 `:/icons/...`、`:/assets/...` 等文件路径。

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
