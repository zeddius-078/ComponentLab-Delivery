# DesignToCode_GenerateLayout

本项目把 Pixso 页面转换为 Qt 页面结构。当前只维护 **HTML + DSL** 路线：HTML 提供浏览器实测的布局与文字尺寸，Pixso DSL 提供节点层级、组件身份、实例属性和 Token 引用。两类输入必须来自同一设计版本。

正式试点以 **light-compact + uic 四件套** 交付。平台维护工具和 SDK，业务侧开发者使用工具生成，再自行导入业务工程；工具不写业务仓库。执行标准、分工、阶段证据和未完成项统一维护在[唯一执行计划](docs/Gen_layout最小改造执行计划.md)。旧 QUiLoader 演示仅用于历史回归，不是正式页面交付合同。

| 目录 | 职责 |
| --- | --- |
| `Source/HtmlDsl` | 原始 HTML、Pixso DSL、合并规则、Qt XML 编译器和生成快照 |
| `Source/Qt` | ReferencePage、V1、V2 与节点案例的 Qt 运行器 |
| `Source/Validation` | 来源哈希、布局结构、SDK 引用和离线运行检查 |
| `build/sdk` | 从 GenerateComponent 导入的组件 SDK；本项目只消费 |
| `build/demo/page_examples` | 可运行的页面演示 |
| `build/reports` | 本轮构建和测试结果 |
| `build/delivery` | 正式模式的独立页面包；历史失败批次不能当成已验收版本 |
| [docs](docs/README.md) | 数据流、映射规则和开发入口 |

```text
同源 Pixso HTML + DSL
          |
          v
HTML 测量布局 + DSL 识别组件/状态/Token
          |
          v
.ui + .h + .cpp + .qss（页面资产存在时附 QRC）
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

## 历史预览回归

直接生成旧预览使用 `Source/HtmlDsl/Generate.ps1 -Preview`。下列 V2 兼容入口已明确转发该开关，不会误生成正式四件套。

```powershell
pwsh -File Source/Import-GeneratedComponentSDK.ps1 -Package '<GeneratedComponentSDK目录>'
pwsh -File Source/Qt/V2/Build.ps1 -Test
pwsh -File Source/Qt/V2/Verify.ps1
python -B Source/Validation/check_delivery.py
```

在 Qt Designer 查看 [V2 结果页](Source/HtmlDsl/generated/result-html.ui)。设计稿出现未映射组件时，回到 `DesignToCode_GenerateComponent` 增加设计契约、组件实现和 SDK 发布；本项目只装配已经发布的组件。

正式 `.ui` 使用 SDK 公开的 `commonWidget::*` 或 `designer_widgets::*` 类及原生布局/页面容器。`designer_widgets::SelectOption` 已在新 SDK 发布，旧预览可继续使用旧 SDK 和私有适配器。不要把旧 `build/sdk` 未升级的状态误认为新公开类已存在；独立消费验证必须指定实际包含该类的发布 SDK。本项目不保存第二套组件实现。
