# 组件库切换与解耦契约（library-profiles.json）

## 2026-10-20：独立组件集与一处前缀切换

当前默认 `token-sdk` 保持不变；新增 `analysis-system` 是独立的**类名合同**，不是已实现组件库。选择由 `-LibraryProfile analysis-system`（PowerShell）或 `--library-profile analysis-system`（编译器）完成，生成器中没有硬编码这个组件集名称。

入口为 `docs/fixtures/layout/mapping/library-profiles.json`。新 profile 的 `componentLibrary` 指向同目录下的 `component-libraries/analysis-system.json`。以后新增另一套组件集，只需新增 JSON 并登记一个 profile 名称，原设计映射和组件实现生成器不用改。

新 JSON 声明 54 个稳定 ID：49 个基础声明（44 个 QWidget 控件、4 个布局、1 个 Spacer）及 5 个 Analysis 扩展/组合控件。每个 ID 仅对应一个目标类，不会把 `TableView` 与 `TableWidget` 合并。水平/垂直方向、QFrame 分隔线类型保留为属性，不制造重复身份。

### 最小命名变更

```json
{
  "classPrefix": "Z_",
  "headerPrefix": "AnalysisWidgets/",
  "components": [
    {"id": "QPushButton", "classSuffix": "PushButton", "qtBase": "QPushButton", "kind": "widget"}
  ]
}
```

生成规则为 `classPrefix + classSuffix`。只将 `classPrefix` 改为 `Acme_`，就会把 `Z_PushButton`、`Z_AnalysisDataTable` 等切换为 `Acme_PushButton`、`Acme_AnalysisDataTable`，并同步 `.ui` 的 customwidget 类名及自动头文件名。稳定 ID、设计绑定和 Qt 基类不变。`headerPrefix` 只控制头文件目录；如实际库目录也改变，再单独修改这一项。不得用前缀替换去修改 Qt API 拼写。

名称修正包括 `FormLayout`、`SpacerItem`、`CommandLinkButton`、`ColumnView`、`Frame`、`MdiArea`、`ComboBox`、`FontComboBox`、`TimeEdit`、`DateTimeEdit`、`LCDNumber`。组合菜单栏/工具栏采用 `AnalysisMenuBar` / `AnalysisToolBar`，不与 Qt 的 ToolBox 混淆。Analysis 扩展基类为规划值（`baseStatus=proposed`），不是对现有实现的认定。

### 职责与验收边界

- JSON 只定义类名、稳定 ID、Qt 基类、分类与头文件合同，不参与 GenerateComponent 的实现生成或 SDK 已实现组件登记。
- 构建期用 Qt 基类测量布局，输出使用目标类；这些测量不能证明目标库最终的外观、交互或像素一致性。
- 公开 Qt API 继续通过白名单适配，跳过的 SDK 专用调用记入 `skippedCalls`，不伪装为已实现能力。
- 未映射类、重复 ID/目标类、非法头文件与无效基类不会静默回退。
- 布局/Spacer 不是 QWidget：本轮 `.ui` 保留 Qt 标准布局和 spacer 元素，规划类名仅登记在 JSON，不宣称已实现自定义布局的 uic 接入。根页面继续是页面自己的 QWidget。
- `status=contract-only`、`implementationStatus=planned` 的组件集只允许预览/映射测试，不允许以正式可运行页面包发布。`.components.json` 保存组件集 ID、JSON SHA-256、测量基类与未实现状态。
- 当前布局解析器本身仍链接既有 SDK；本次解耦的是**目标库声明与目标组件实现生成**，不宣称已经去除解析器所有 SDK 构建依赖。

### 同一输入的对照测试

在项目根、已配置的原有 Qt 环境执行（不安装或更新任何环境）：

```powershell
pwsh -File Source/HtmlDsl/pipeline/Generate-Pages.ps1 `
  -Pages @{ review = 'docs/fixtures/layout/design-inputs/review-page/canonical.raw.json' } `
  -OutputDirectory build/preview/analysis-system `
  -MappingDirectory docs/fixtures/layout/mapping `
  -Font docs/examples/resources/HarmonyOS_Sans_SC_Regular.ttf `
  -LibraryProfile analysis-system

python -B -m unittest discover -s docs/validation/layout -p test_component_libraries.py -v
```

改用 `-LibraryProfile token-sdk` 和另一输出目录即可对照原系统。以上是现有 canonical 测试输入，不绕过或宣称修复 HTML 采集清单的问题。新增测试覆盖同一输入双库、默认行为、纯 JSON 注册第三套库、一处前缀切换、全部 49 个 QWidget 声明的 uic 解析及拒绝错误合同。uic 成功不等于目标库编译或运行成功。

本轮实测：新增 9 项测试和原有 5 项切换测试通过；49 个 QWidget 声明在 `Z_` / `Acme_` 两种前缀下均通过 uic。`Source/Build.ps1 -Test` 的布局转换套件通过，组件转换套件在既有导航栏 `component.navigation-bar.base.state.default.slot.container.height` 缺失处失败，该问题仍按用户决定挂起。不能把专项通过写成全项目通过。

## 固定安装环境

遵循工作区 `AGENTS.md`：不得自主安装、更新或替换 Python、Qt、编译器与依赖。版本不兼容优先做最小源码修补；需要变更环境必须询问用户。Python 构建脚本以 3.9 兼容为目标，只有真实 3.9 运行证据才能称为该版本实测通过。

## 背景

GenerateLayout 生成的页面引用具体组件类（`commonWidget::Z_*`）并通过 GenerateComponent SDK 的
`ComponentFactory::apply` 与组合 API 初始化。开发侧无法一次性切换到新组件库，因此需要：

1. GenerateLayout 与目标组件库解耦；
2. 支持**批量更改组件命名**切换目标库；
3. 支持**渐进式适配升级**（先切个别组件，其余保持原库）。

## 三层结构

| 层 | 文件 | 职责 | 谁维护 |
| --- | --- | --- | --- |
| 设计契约 | `component-mapping.json` | 设计身份 → 默认 SDK 类与属性/状态/槽映射；权威且稳定 | GenerateComponent 发布流程 |
| 库配置 | `library-profiles.json` | 类命名规则、API 策略、目标库角色 | 开发侧按需扩展 |
| 生成器 | `v2_ui_compiler` | 按所选 profile 解析最终类名与调用策略 | GenerateLayout |

设计契约层**不因切换目标库而修改**；切换只发生在库配置层。

## 内置 Profile

- `token-sdk`（默认）：当前行为，零变化。生成的 `.ui` 用 `commonWidget::Z_*` / `designer_widgets::*`，
  初始化用 `factory.apply` + 组合 API。
- `qt-native`：**批量改名** `commonWidget::Z_X → QX`（语义组件用 `componentOverrides` 映射到正确的
  公开 Qt 类，如 `ComponentLab/Input → QLineEdit`、`ComponentLab/Select → QComboBox`）；只保留
  `publicQtApis` 白名单内的调用并改为**直接方法调用**，SDK 专属 setter 与组合 API 被跳过且记录在
  清单 `skippedCalls` 中；没有公开 Qt 等价物的类（如 `Z_MetricCard`）**失败关闭**
  （`UNSUPPORTED_QT_NATIVE_CLASS`），开发侧需提供覆盖或移除该组件。SelectOption 适配器在 qt-native
  下报 `SELECT_OPTION_REQUIRES_TOKEN_SDK`。
- `custom-library`（示例）：`classRenames` 批量改名 `commonWidget::Z_* → MyOrg::*`，
  `componentOverrides` 只覆盖个别组件（示例覆盖 Button），演示渐进式迁移。构建期仍用 SDK 控件实测几何，
  交付类名与 `factory.apply` 调用契约交给目标库的同名适配器实现。

## 设计 Profile 到 SDK 上下文

设计稿中的 `Profile` 名称属于设计侧变量模式；SDK 运行时只接受
`standard-light`、`standard-dark`、`wide-light`、`wide-dark` 四个已发布上下文。
两套名称不能直接混用，映射合同中的 `profileAliases` 负责显式转换：

| 设计侧名称 | SDK 运行上下文 | 设计语义 |
| --- | --- | --- |
| `light-comfortable` | `standard-light` | 亮色、舒适密度 |
| `light-compact` | `wide-light` | 亮色、紧凑密度 |

生成器会扫描当前页面实例的 `Profile`：

1. 读取页面内所有实例的设计侧 Profile；
2. 要求页面只有一个有效 Profile；
3. 通过 `component-mapping.json.profileAliases` 得到 SDK 上下文；
4. 使用 SDK 上下文安装 Token、创建组件和校验变量；
5. 在 `.components.json` 与正式生成记录中同时写入 `designProfile`、`profile` 和 `profileAlias`。

别名只允许指向 `runtimeProfiles` 中已经发布的上下文。缺少别名、目标上下文不存在或页面混用多个 Profile 时，生成器返回明确错误并停止，不把设计名称直接写入 Token 运行时。

## 切换方式

```powershell
# 正式生成时指定库配置（默认 token-sdk）
pwsh -File Source/HtmlDsl/Generate.ps1 -CaptureManifest '<capture.json>' -MappingDirectory '<映射目录>' `
  -Font '<字体>' -Batch '<批次>' -Page '<页面>' -LibraryProfile qt-native
```

生成记录（`generation-record.json`）与组件清单（`.components.json`）会记录
`libraryProfile` 与 `library-profiles.json` 的 SHA256，保证切换可追溯。

## 新增自定义库（渐进式适配）

在 `library-profiles.json` 的 `profiles` 中新增一项：

```json
{
  "libraryKind": "custom",
  "classRenames": [{"pattern": "^commonWidget::Z_(.+)$", "replacement": "MyOrg::\\1"}],
  "componentOverrides": {"ComponentLab/Button": {"qtClass": "MyOrg::PushButton"}},
  "apiPolicy": "sdk-composition",
  "widgetSource": "sdk-factory",
  "headerPolicy": "custom-header-map",
  "headerMap": [{"classPattern": "^MyOrg::", "extends": "QWidget", "header": "MyOrg/Components.h"}]
}
```

关键约束：

- `apiPolicy=sdk-composition` 要求目标库提供与 SDK 同名 API 的 `apply` 适配器（渐进式适配的接缝）；
- `apiPolicy=public-qt` 要求目标类只使用公开 Qt API，生成器发出直接方法调用；
- `headerMap` 决定 `.ui` 的 customwidget `extends/header` 声明；
- 没有等价物时不静默降级：qt-native 直接失败并给出类名，让开发侧明确决策。

## 按组件名替换映射（批量改名 + 单组件覆盖）

不需要修改设计契约 `component-mapping.json`。按组件名替换映射只有两种操作，都在
`library-profiles.json` 内完成：

1. **批量改名**：`classRenames` 按顺序对每个组件的 `qtClass` 做正则替换。
   例如把所有 `commonWidget::Z_*` 一次性改名为 `MyOrg::*`：

   ```json
   "classRenames": [
     {"pattern": "^commonWidget::Z_(.+)$", "replacement": "MyOrg::\\1"}
   ]
   ```

2. **按组件名单点替换**：`componentOverrides` 以 `designComponentName`（与
   `component-mapping.json` 的 `designComponentName` 一致）为键，只覆盖该组件的类名：

   ```json
   "componentOverrides": {
     "ComponentLab/Button": {"qtClass": "MyOrg::PushButton"},
     "ComponentLab/Input": {"qtClass": "MyOrg::LineEdit"}
   }
   ```

覆盖优先级：`componentOverrides`（按名）先于 `classRenames`（批量）应用；两者都未命中时保留
设计契约的默认 `qtClass`。切换顺序即渐进式适配顺序：先用 `classRenames` 切换同名映射的大类，
再逐个为语义不同的组件补 `componentOverrides`，最后为没有等价物的组件在目标库补齐实现或从
页面移除。

选择 profile 后生成器的执行结果（类名、跳过调用）都记录在 `.components.json` 与
`generation-record.json` 中，`libraryProfile` 与配置文件 SHA256 保证切换可追溯。

## 点位视图验证（同一设计 x 两套映射）

项目 demo（`build/demo/page_examples/V2`）内置同一设计（Pixso 27:450 复核页）的两个点位页面：

| 前缀 | 组件库 | 运行方式 |
| --- | --- | --- |
| `review` | Z Design System（token-sdk，`commonWidget::Z_*` + 工厂初始化） | `Run-HTML-DSL.cmd --page review` |
| `review-qt` | Qt 原生（qt-native，`Q*` 内置类 + XML 属性初始化） | `Run-HTML-DSL.cmd --page review-qt` |

生成方式（复用现有编译器，不引入新脚本）：

```powershell
# Z Design System 点位（默认 profile，与预览管线一致）
v2_ui_compiler.exe --raw docs/fixtures/layout/design-inputs/review-page/canonical.raw.json `
  --mapping docs/fixtures/layout/mapping --extensions docs/fixtures/layout/mapping/extensions.json `
  --font <data>/HarmonyOS_Sans_SC_Regular.ttf --output <data>/review --strict-layout

# Qt 原生点位
v2_ui_compiler.exe ... --output <data>/review-qt --strict-layout --library-profile qt-native
```

qt-native 点位的公开 Qt 组件的初始状态（text／enabled／checked／placeholderText／readOnly／
currentIndex／value／format／title／gridVisible）由生成器直接序列化进 `.ui` XML 属性，运行期
`QUiLoader` 即可还原，不需要 ComponentFactory；运行期加载器（`UiPage`）对清单中的公开 Qt
内置类放行，对带命名空间但未注册的类仍失败关闭（`UNREGISTERED_UI_CLASS`）。

## 已记录的管线限制（2026-10）

1. **预览管线不接受库配置**：`Generate.ps1 -Preview` 走 `Generate-Pages.ps1`，其编译命令没有
   `-LibraryProfile` 参数（正式路径 `Generate.ps1` 已有）。qt-native 点位只能按上节方式直接调用
   编译器生成，或在后续把参数透传补进 `Generate-Pages.ps1`。
2. **预览采集清单哈希失配（先于本功能存在）**：`docs/fixtures/layout/html-dsl/result-page/capture.json`
   的 `sourceHashes.htmlSha256` 与磁盘上的 `page.html` 实际 SHA256 不一致，`capture_manifest.py
   verify` 报 `CAPTURE_HASH_MISMATCH: htmlSha256`，因此完整预览/正式管线当前无法跑通；本次点位
   页面用 canonical 设计输入直接驱动编译器绕过该检查。
3. **SelectOption 适配器没有公开 Qt 等价物**：qt-native 下包含 SelectOption 的页面按设计失败关闭
   （`SELECT_OPTION_REQUIRES_TOKEN_SDK`），需要开发侧决定替代组件或自定义覆盖。
4. **无公开 Qt 等价物的组件类失败关闭**：如 `commonWidget::Z_MetricCard` 改名后得到
   `QMetricCard` 不在公开 Qt 白名单表内，报 `UNSUPPORTED_QT_NATIVE_CLASS`（含类名），不静默降级。
5. **qt-native 的几何测量基于公开 Qt 控件**：设计稿使用 token 化外观，`Q*` 控件测量出的
   sizeHint 与源稿几何可能不同；布局结构一致，像素级视觉对等不在 qt-native 契约内。
