# DesignToCode_GenerateLayout

本项目把 Pixso 页面转换为 Qt 页面结构。当前只维护 **HTML + DSL** 路线：HTML 提供浏览器实测的布局与文字尺寸，Pixso DSL 提供节点层级、组件身份、实例属性和 Token 引用。两类输入必须来自同一设计版本。生成器面向布局结构工作，不把 review、result 等示例页面写成固定分支。

正式交付使用 **wide-light Profile + 独立的 Compact 组件/布局密度 + uic 页面文件包**。Profile 只组合设备宽度与明暗主题，Density 仍是组件或布局属性，二者不得互相替代。本项目维护布局生成工具链并消费权威组件 SDK；业务侧开发者使用工具生成页面，再自行导入业务工程。工具不写业务仓库。执行标准与验证入口见[布局生成执行与验收规则](docs/designRules/workflow/布局生成执行与验收规则.md)。旧 QUiLoader 演示仅用于回归，不是正式页面交付合同。

| 目录 | 职责 |
| --- | --- |
| `Source` | 解析器、布局生成器及构建/采集/校验入口；禁止保存生成物 |
| `docs` | 不可随意删除：规范、历史记录、固定输入、映射规则、样例和回归测试 |
| `docs/fixtures/layout` | 只读 HTML、Pixso DSL、资源、映射规则和案例输入夹具 |
| `docs/examples` | ReferencePage、V1、V2 与节点案例的维护源码、模板和字体 |
| `docs/validation` | 来源哈希、布局结构矩阵、SDK 引用和离线运行检查 |
| `build` | 全部可再生，可整体删除；不得保存唯一维护源码或固定资产 |
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

环境仍使用项目已有的 `QT_ROOT / VS_ROOT / CMAKE_EXE / NINJA_ROOT / D2C_NODE_MODULES`，先构建 `v2_ui_compiler`。采集清单、`wide-light` 映射、独立 Density 值和字体是显式输入；生成器不接受旧 Profile 名称，也不把 Density 推导为 Profile。

```powershell
pwsh -File Source/Build.ps1 -Test
pwsh -File Source/HtmlDsl/Generate.ps1 -CaptureManifest '<capture.json>' -MappingDirectory '<Compact映射目录>' -Font '<字体文件>' -Batch '<批次>' -Page '<页面工程名>'
python -B docs/validation/check_delivery.py --formal-batch '<三页验收批次>'
pwsh -File docs/validation/formal/Verify-RelocatedConsumer.ps1 -Batch '<三页验收批次>' -SdkRoot '<已发布SDK目录>'
```

默认是正式模式，不接受 `-OutputDirectory`，只写本工具的 `build/delivery/<批次>/<页面>`，已有包拒绝覆盖。`-Formal` 保留为兼容开关，旧预览需显式传 `-Preview`。结构检查输出 `PASS_STRUCTURE`，不代表编译和视觉验收自动完成。验收规则见当前执行规范；历史批次与过程证据统一归档，不当作当前通过证据。

## 预览回归

预览回归也必须显式提供页面清单。清单把 HTML、DSL、资源和页面根节点绑定在一起；预览清单的 `manifestType` 为 `preview-fixture`，不作为正式交付输入。

```powershell
pwsh -File Source/HtmlDsl/Generate.ps1 `
  -Preview `
  -CaptureManifest docs/fixtures/layout/html-dsl/result-page/capture.json `
  -ReviewCaptureManifest docs/fixtures/layout/html-dsl/holdout-19-45/capture.json `
  -MappingDirectory docs/fixtures/layout/mapping `
  -Font docs/examples/resources/HarmonyOS_Sans_SC_Regular.ttf
pwsh -File Source/Verify-GeneratedComponentSDK.ps1
pwsh -File docs/examples/Qt/V2/Build.ps1 -Test
pwsh -File docs/examples/Qt/V2/Verify.ps1
python -B docs/validation/check_delivery.py
```

新页面先使用 `Source/HtmlDsl/pipeline/capture/capture_manifest.py create` 组装清单，再执行 `verify`。正式模式还必须提供独立 Pixso 布局证据 `layoutReference`；缺少清单或证据时，生成在适配前失败。

生成后可在 Qt Designer 打开 `build/preview/html-dsl/result-html.ui`。设计稿出现未映射组件时，回到 `DesignToCode_GenerateComponent` 增加设计契约、组件实现和 SDK 发布；token-sdk 正式链装配已发布组件；未实现目标库只允许 contract-only 预览。

## 布局结构能力验证

`v2_ui_compiler.exe` 是当前 HTML + DSL 路线的通用布局编译器；`docs/examples/Qt/V2` 是其运行与回归工作台，不代表只支持两个页面。当前布局分派已覆盖 `QHBoxLayout`、`QVBoxLayout`、`QGridLayout` 和 `QFormLayout`：前两者保持原有线性行为，Grid 使用行列与跨度，Form 使用行号和 Label/Field/Spanning 角色。完整字段、输入示例和验收边界见[布局转换器通用化规范](docs/designRules/layout/布局转换器通用化规范.md)。未知布局方向、缺失 Grid/Form 定位信息、绝对定位、未登记组件、缺失必需布局值和未支持节点类型会明确失败。

统一组件清单中的 `ComponentLab/Spacer` 与 `ComponentLab/LayoutContainer` 由本项目消费，不进入 QWidget Factory。`Spacer` 在横向或纵向 `QBoxLayout` 中生成 `QSpacerItem`，支持固定、最小和伸缩三种运行时策略；`LayoutContainer` 校验 Figma 的 `Columns=1/2/3` 与 `Density=Comfortable/Compact`，并由 `QGridLayout` 生成自动行列位置。两者都会先通过已发布组件 SDK 的 `designLibraryEntries()` 校验 canonical ID 和 `LayoutHelper` 类型。当前验证覆盖 LayoutConverter 和异常输入；上游设计归一化自动生成 `layoutHelper` 合同、ComponentLab 运行演示及端到端页面证据仍是后续门禁。

公共资产使用稳定语义 ID：图标为 `icon.*`，图片为 `image.*`，二维原理图为 `model.*`。`resourceMappings` 和顶层 `publicAssetMappings` 只登记语义 ID、所有权、状态和适配合同；实际 QRC 路径与资源存在性分别由已发布 SDK 的 `IconAssetCatalog`、`VisualAssetCatalog` 解析和校验。未知公共 ID 返回 `UNMAPPED_RESOURCE`，不会回退到本地同名文件；页面自有资源仍由页面包单独声明和发布。`ModelAsset` 当前仅代表二维原理图资源，不据名称推导 3D、命中检测或实时场景能力。

结构矩阵验证命令：

```powershell
$env:QT_ROOT = 'D:\QT\5.15.2\msvc2019_64'
python -B docs/validation/layout/test_shape_matrix.py
```

该测试调用同一个生成器，使用纵向根、横向根、嵌套结构以及 Grid/Form 夹具，检查 `.ui` 可解析、对象名唯一、行列/角色定位存在及重复生成字节一致。页面功能测试仍使用已有 V2 案例；二者分别回答“结构能否生成”和“特定演示流程是否正确”。

token-sdk 正式 `.ui` 使用 SDK 公开的 `commonWidget::*` 或 `designer_widgets::*` 类及原生布局/页面容器。`designer_widgets::SelectOption` 已在权威 SDK 发布；独立消费验证必须绑定该发布目录及其 manifest 哈希。本项目不保存第二套组件实现或 SDK 副本。

## 多组件库和一处前缀切换

配置入口为 docs/fixtures/layout/mapping/library-profiles.json，使用 -LibraryProfile 按名称选择；正式和预览入口均支持参数传递。默认 token-sdk 不改动，新 analysis-system 的独立 JSON 位于 component-libraries/analysis-system.json。

修改其中 classPrefix 会同步改变目标 QWidget 类名与自动头文件名，稳定 ID、设计绑定与 Qt 基类保持不变。54 个声明包括 49 个基础条目和 5 个 Analysis 条目；49 个 QWidget 目标与 4 个布局、1 个 Spacer 的处理边界不同。布局/Spacer 保持原生 XML，根页面不替换为目标组件。

analysis-system 仍为 contract-only/planned，仅支持映射测试和预览。Qt 基类测量、XML 或 uic 通过不代表真实库可编译运行，正式发布会拒绝该库。详见 [组件库切换规范](docs/designRules/library-profiles.md)。

解析器本身仍依赖原 SDK。导航栏 Token 消费问题按用户决定挂起；固定夹具的采集哈希需重新校验，不用 canonical 直连测试代替完整 HTML+DSL 验收。遵循工作区 AGENTS.md，不自主安装或升级环境。历史过程见 [归档索引](docs/archive/README.md)。
