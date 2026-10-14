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

结构矩阵验证命令：

```powershell
$env:QT_ROOT = 'D:\QT\5.15.2\msvc2019_64'
python -B Source/Validation/layout/test_shape_matrix.py
```

该测试调用同一个生成器，使用纵向根、横向根、嵌套结构以及 Grid/Form 夹具，检查 `.ui` 可解析、对象名唯一、行列/角色定位存在及重复生成字节一致。页面功能测试仍使用已有 V2 案例；二者分别回答“结构能否生成”和“特定演示流程是否正确”。

正式 `.ui` 使用 SDK 公开的 `commonWidget::*` 或 `designer_widgets::*` 类及原生布局/页面容器。`designer_widgets::SelectOption` 已在权威 SDK 发布；独立消费验证必须绑定该发布目录及其 manifest 哈希。本项目不保存第二套组件实现或 SDK 副本。
