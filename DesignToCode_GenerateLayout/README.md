# DesignToCode_GenerateLayout

本项目把 Pixso 页面转换为 Qt 页面结构。当前只维护 **HTML + DSL** 路线：HTML 提供浏览器实测的布局与文字尺寸，Pixso DSL 提供节点层级、组件身份、实例属性和 Token 引用。两类输入必须来自同一设计版本。生成器面向布局结构工作，不把 review、result 等示例页面写成固定分支。

正式试点以 **light-compact + uic 页面文件包** 交付。本项目维护布局生成工具链并消费权威组件 SDK；业务侧开发者使用工具生成页面，再自行导入业务工程。工具不写业务仓库。执行标准、分工、阶段证据和未完成项统一维护在[布局生成执行与验收规则](docs/designRules/workflow/布局生成执行与验收规则.md)。旧 QUiLoader 演示仅用于回归，不是正式页面交付合同。

| 目录 | 职责 |
| --- | --- |
| `Source/HtmlDsl` | 原始 HTML、Pixso DSL、合并规则、Qt XML 编译器和生成快照 |
| `Source/Qt` | ReferencePage、V1、V2 与节点案例的 Qt 运行器 |
| `Source/Validation` | 来源哈希、布局结构矩阵、SDK 引用和离线运行检查 |
| `../DesignToCode_GenerateComponent/build/sdk` | 工作区唯一权威组件 SDK；本项目直接读取，不保存副本 |
| `build/demo/page_examples` | 可运行的页面演示 |
| `build/reports` | 本轮构建和测试结果 |
| `build/delivery` | 正式模式的独立页面包；历史失败批次不能当成已验收版本 |
| [docs](docs/README.md) | 数据流、映射规则和开发入口 |

布局生成工具链通过命令行、输入清单和文件合同工作，不向业务工程提供需要链接的公共库 API，因此当前不命名为 SDK。后续只有在形成稳定的 C++ 公共 API、版本兼容策略和独立消费测试后，才评估单独发布 `LayoutGeneratorSDK`。

```text
同源 Pixso HTML + DSL
          |
          v
HTML 测量布局 + DSL 识别组件/状态/Token
          |
          v
.ui + .h + .cpp + .qss + .qrc
          |
          v
uic + GeneratedComponentSDK + 业务侧代码
```

追溯、输入哈希和生成文件哈希只存于包外报告。页面头文件暴露全部页面对象和布局访问器，SDK 组件内部对象不展开为页面 XML。

## 正式生成入口

环境仍使用项目已有的 `QT_ROOT / VS_ROOT / CMAKE_EXE / NINJA_ROOT / D2C_NODE_MODULES`，先构建 `v2_ui_compiler`。采集清单、明确的 Compact 映射和字体是输入，不从旧 Comfortable 输入静默切换。

```powershell
pwsh -File Source/HtmlDsl/Generate.ps1 -CaptureManifest '<capture.json>' -MappingDirectory '<Compact映射目录>' -Font '<字体文件>' -Batch '<批次>' -Page '<页面工程名>'
python -B Source/Validation/check_delivery.py --formal-batch '<三页验收批次>'
pwsh -File Source/Validation/formal/Verify-RelocatedConsumer.ps1 -Batch '<三页验收批次>' -SdkRoot '<已发布SDK目录>'
```

默认是正式模式，不接受 `-OutputDirectory`，只写本工具的 `build/delivery/<批次>/<页面>`，已有包拒绝覆盖。`-Formal` 保留为兼容开关，旧预览需显式传 `-Preview`。结构检查输出 `PASS_STRUCTURE`，不代表编译和视觉验收自动完成。当前验收批次及证据统一见执行计划。

## 预览回归

预览回归也必须显式提供页面清单。清单把 HTML、DSL、资源和页面根节点绑定在一起；预览清单的 `manifestType` 为 `preview-fixture`，不作为正式交付输入。

```powershell
pwsh -File Source/HtmlDsl/Generate.ps1 `
  -Preview `
  -CaptureManifest Source/HtmlDsl/inputs/result-page/capture.json `
  -ReviewCaptureManifest Source/HtmlDsl/inputs/holdout-19-45/capture.json
pwsh -File Source/Verify-GeneratedComponentSDK.ps1
pwsh -File Source/Qt/V2/Build.ps1 -Test
pwsh -File Source/Qt/V2/Verify.ps1
python -B Source/Validation/check_delivery.py
```

新页面先使用 `Source/HtmlDsl/pipeline/capture/capture_manifest.py create` 组装清单，再执行 `verify`。正式模式还必须提供独立 Pixso 布局证据 `layoutReference`；缺少清单或证据时，生成在适配前失败。

在 Qt Designer 查看 [V2 结果页](Source/HtmlDsl/generated/result-html.ui)。设计稿出现未映射组件时，回到 `DesignToCode_GenerateComponent` 增加设计契约、组件实现和 SDK 发布；本项目只装配已经发布的组件。

## 布局结构能力验证

`v2_ui_compiler.exe` 是当前 HTML + DSL 路线的通用布局编译器；`Source/Qt/V2` 是其运行与回归工作台，不代表只支持两个页面。当前布局分派已覆盖 `QHBoxLayout`、`QVBoxLayout`、`QGridLayout` 和 `QFormLayout`：前两者保持原有线性行为，Grid 使用行列与跨度，Form 使用行号和 Label/Field/Spanning 角色。完整字段、输入示例和验收边界见[布局转换器通用化规范](docs/designRules/layout/布局转换器通用化规范.md)。未知布局方向、缺失 Grid/Form 定位信息、绝对定位、未登记组件、缺失必需布局值和未支持节点类型会明确失败。

统一组件清单中的 `ComponentLab/Spacer` 与 `ComponentLab/LayoutContainer` 由本项目消费，不进入 QWidget Factory。`Spacer` 在横向或纵向 `QBoxLayout` 中生成 `QSpacerItem`，支持固定、最小和伸缩三种运行时策略；`LayoutContainer` 校验 Figma 的 `Columns=1/2/3` 与 `Density=Comfortable/Compact`，并由 `QGridLayout` 生成自动行列位置。两者都会先通过已发布组件 SDK 的 `designLibraryEntries()` 校验 canonical ID 和 `LayoutHelper` 类型。当前验证覆盖 LayoutConverter 和异常输入；上游设计归一化自动生成 `layoutHelper` 合同、ComponentLab 运行演示及端到端页面证据仍是后续门禁。

公共资产使用稳定语义 ID：图标为 `icon.*`，图片为 `image.*`，二维原理图为 `model.*`。`resourceMappings` 和顶层 `publicAssetMappings` 只登记语义 ID、所有权、状态和适配合同；实际 QRC 路径与资源存在性分别由已发布 SDK 的 `IconAssetCatalog`、`VisualAssetCatalog` 解析和校验。未知公共 ID 返回 `UNMAPPED_RESOURCE`，不会回退到本地同名文件；页面自有资源仍由页面包单独声明和发布。`ModelAsset` 当前仅代表二维原理图资源，不据名称推导 3D、命中检测或实时场景能力。

Badge、StatusIndicator、Link、Tag、Tooltip 分别通过组件集 ID `2132:21`、`2132:69`、`2132:102`、`2132:150`、`2132:189` 映射到已发布 SDK。编译器严格校验当前 Figma 变体轴并映射 `Label`／`Content` 文本槽；Link 的 State 写入公开 `snapshotState` 以保留静态生成态，空值恢复 Qt 原生交互；Tooltip 的目标解析与 `attachTo` 调用、Tag 的删除／持久化／撤销由生成页面负责。

`ComponentLab/Timeline` 通过组件集 ID `2147:35` 映射到已发布 SDK 的 `commonWidget::Z_Timeline`。编译器只传递 Figma 的 `Density=Comfortable/Compact`，事件内容由页面数据调用 `setEvents` 提供；GenerateLayout 不复制状态标记资源，也不把 Timeline 当作可点击的 Stepper。

`ComponentLab/Pagination` 通过组件集 ID `2148:438` 映射到已发布 SDK 的 `commonWidget::Z_Pagination`。编译器只传递 `Density=Default/Compact` 与 `State=Default/Disabled`；页码数据、当前页、总页数、加载与翻页结果由生成页面 controller 持有，SDK 负责边界翻页按钮、无边框页码、当前页实色块和长序列省略号。

`ComponentLab/AnchorNavigation` 通过组件集 ID `2150:82` 映射到已发布 SDK 的 `commonWidget::Z_AnchorNavigation`。编译器传递 `Orientation=Vertical/Horizontal` 与 `State=Default/Scrolled`；锚点标签、目标绑定和滚动生命周期由生成页面持有，SDK 只发出受控 `anchorActivated` 请求并接收 `currentIndex` 回传。

`ComponentLab/Filter` 通过组件集 ID `2148:244` 映射到已发布 SDK 的 `commonWidget::Z_Filter`。编译器只传递 `Layout=Inline/Panel` 与 `State=Default/Applied`；筛选条件、运算符、值、校验、清除、持久化和状态转换由生成页面的 filter controller 持有，SDK 只发出条件与应用请求。

`ComponentLab/NumericKeypad` 通过组件集 ID `2148:1018` 映射到已发布 SDK 的 `commonWidget::Z_NumericKeypad`。编译器只传递 `Density=Comfortable/Compact`；按键语义、允许集与小数分隔符由 SDK 提供，目标输入框、locale 解析、数值约束、退格处理、焦点路由和提交策略由生成页面的 input controller 持有。

`ComponentLab/ColorPicker` 通过组件集 ID `2147:1390` 映射到已发布 SDK 的 `commonWidget::Z_ColorPicker`。编译器只传递 `State=Default/Focus/Disabled`；当前 QColor 和显示格式由页面数据设置，`pickerRequested` 交给页面的 color-selection controller。GenerateLayout 不复制 `Z_ColorDialog`，也不持有颜色空间、Alpha、最近使用、预览和提交／取消状态。

`ComponentLab/Upload` 通过组件集 ID `2148:818` 映射到已发布 SDK 的 `commonWidget::Z_Upload`。编译器只传递五种设计状态；文件名、说明和进度由页面数据设置，`browseRequested` 与 `filesDropped` 交给页面 upload controller。GenerateLayout 不复制 `Z_FileDialog`，也不持有文件校验、网络传输、鉴权、取消和重试生命周期。

`ComponentLab/Transfer` 通过组件集 ID `2148:640` 映射到已发布 SDK 的 `commonWidget::Z_Transfer`。编译器只传递 Default／Disabled／Loading 设计状态；页面 selection controller 向两个 QListView 注入 QAbstractItemModel，并处理方向命令产生的移动请求。GenerateLayout 不复制列表实现，也不在 SDK 组件内部执行集合查询、移动、排序、权限判断或错误恢复。

结构矩阵验证命令：

```powershell
$env:QT_ROOT = 'D:\QT\5.15.2\msvc2019_64'
python -B Source/Validation/layout/test_shape_matrix.py
```

该测试调用同一个生成器，使用纵向根、横向根、嵌套结构以及 Grid/Form 夹具，检查 `.ui` 可解析、对象名唯一、行列/角色定位存在及重复生成字节一致。页面功能测试仍使用已有 V2 案例；二者分别回答“结构能否生成”和“特定演示流程是否正确”。

正式 `.ui` 使用 SDK 公开的 `commonWidget::*` 或 `designer_widgets::*` 类及原生布局/页面容器。`designer_widgets::SelectOption` 已在权威 SDK 发布；独立消费验证必须绑定该发布目录及其 manifest 哈希。本项目不保存第二套组件实现或 SDK 副本。
