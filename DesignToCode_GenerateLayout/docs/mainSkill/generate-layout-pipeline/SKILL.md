---
name: generate-layout-pipeline
description: 按输入类型选择独立的 HTML/DSL 或 Pixso 原生 Vue 导出到 Qt 页面路径，说明既有 SDK 映射、固定 Python 3.9 环境与分项验收；不开发组件或承诺任意 Vue 转换。
---

# Generate Layout Pipeline Skill

## 0. 先选择输入路径

| 实际输入 | 执行路线 | 必读说明 |
| --- | --- | --- |
| 同版 HTML、DSL、layoutReference、capture manifest | `Source/HtmlDsl` 第一路径 | 本 Skill 第 2–9 节和布局生成上手指南 |
| Pixso 原生 Vue ZIP / SFC、CSS、资源 + 同批 Pixso DSL | `Source/pixso_native` 第二路径 | [原生 DSM/D2C 到 Qt 第二路径](../../Pixso原生DSM-D2C到Qt第二路径说明.md) |

原生 Vue 不能直接交给 HTML/DSL 入口。第二路径不调用第一路径爬取器、编译器或结果，不能为了绕过缺失环境切换路线。工作目录分别管理，只消费同一已发布 SDK 与已有工具；不复制组件私有源码、不自行重建或升级 SDK。第二路径必须同时提供与 Vue 导出同批的 Pixso DSL；DSL 用于恢复节点名称、设计身份和布局关系，缺失或根节点不匹配时拒绝生成。

第二路径当前从 Source 运行，尚未纳入受控工具/离线包发布。按其说明显式指定 Node 和解析模块，不把 Vue 3 应用、Vite、Playwright 或 Chromium 当作静态转换的必要依赖，不安装环境来绕过问题。

本次第二路径配置采用 `export-first`：先按导出属性和 DSM→代码映射跑通，partial、未应用属性和视觉/内容差异留在旁置生成记录，不伪造完整合同。缺图标时仅按显式配置回退 `QStyle::SP_FileIcon`，逐项记录 `SYSTEM_ICON_FALLBACK`；损坏资源不能被回退掩盖。模块哈希、唯一身份、公开 API、SDK/Profile、路径安全和布局规则仍校验。

组件名称匹配采用 `family-identity-prefix` 规则：`ComponentLab/<组件集>/<组件身份>` 是组件映射的登记范围，身份之后的路径只作为属性或变体尾缀，不参与组件族名称比较。组件集、组件身份和 `componentKey` 仍需在身份事实源中唯一确认；该规则不把不同组件合并，也不以实例显示名称或变体字符串猜测组件族。

第二路径真实页面检查必须提供 `check_consumer.py --generation-record`；生成命令必须提供 `--dsl`；未提供时的 API 探针不能代替目标页面验收。`STAGED`、编译通过、运行通过、视觉通过、正式交付分别报告，不把样例或局部通过称为万能基准工具。

## 1. 任务边界

本 Skill 负责选择页面转换路线及解释验收边界。组件实现、Token、公共 QSS 和 SDK 由 `DesignToCode_GenerateComponent` 提供；页面生成器读取 SDK，不复制组件实现。以下第 2–9 节是第一路径合同，不将其 HTML/DSL 前置条件强加给第二路径。

正式页面输出：`.ui`、`.h`、`.cpp`、页面 `.qss` 和 `.qrc`。输出可供业务 Qt 工程继续编译和装配。

## 2. 执行前读取

按以下顺序读取：

1. 工作区 `AGENTS.md`：固定环境和协作约束。
2. 项目 `README.md`：目录与入口。
3. `docs/designRules/layout/布局转换器通用化规范.md`：布局字段和值域。
4. `docs/designRules/library-profiles.md`：组件库选择。
5. `docs/tutorial/布局生成上手指南.md`：当前命令。
6. 目标页面的 capture manifest、HTML、DSL、layoutReference、字体和资源清单；第二路径至少必须有同批 Vue ZIP、`get_node_dsl` JSON、映射和 SDK。

输入缺失时先记录缺失项，不能用固定样例代替目标页面。

### 2.1 输出执行设备基线

本入口的受控执行设备是 `DS-V4F-BUILDUNK-NT-C256K-TEXT-AGENT-PROVIDERUNK`（DeepSeek V4 Flash 256K Non-Think），其完整能力画像在工作区 `docs/designRules/architecture/agent-execution-capability-profile.json`。该设备适合按既定合同执行采集、检查、结构化生成与小范围配置修改；不承担缺字段后的设计推断、组件合同设计、跨模块修复或未验证的流程分支选择。

调用任务必须冻结真实输入和输出：`fileKey/rootId` 或本地输入、路线、`GenerationMode`、SDK 根、Library Profile、mapping、字体、批次、页面名及验收命令。低能力执行端只可调用本 Skill 列出的现有入口；不能把 URL 换成样例、注入变量模式/几何、猜测组件身份或新建 HTML/DSL 解析器与 `.ui` 发射器。任一前置条件或门禁失败时记录错误并停止该分支；不得自动换 Profile/SDK/模式或把旧 `.ui` 作为本次结果。

### 2.2 低能力执行任务卡与受控工具门禁

低能力 Agent 不负责发现环境、选择替代工具或解释旧二进制行为。每次调用必须由任务发起方提供以下冻结字段；任一字段缺失即记录 `TASK_SPEC_REQUIRED` 并停止。

| 字段 | 必须提供的值 | Agent 不得推断的内容 |
| --- | --- | --- |
| `projectRoot` | `G:\ComponentLab-Delivery\DesignToCode_GenerateLayout` | 其他同名目录或示例项目 |
| `pythonExe` | 已存在的 Python 3.9 `python.exe` 完整路径 | 系统默认 Python 或较新版本 |
| `toolRoot` | `build\tools\layout` 或已批准的绝对工具目录 | 缓存 EXE、临时目录或源码旁二进制 |
| `sdkRoot` | 已发布 Release SDK 的绝对路径 | Debug SDK、私有源码或另一版本 |
| `generationMode` | `structure-only` 或 `strict` | 失败后自动降级或切换模式 |
| `rawDsl` / `captureManifest` | 同批、已冻结的输入绝对路径 | 样例、旧批次或改写后的 DSL |
| `mappingDirectory`、`font`、`libraryProfile` | 已冻结的绝对路径与 Profile 名 | 另一套 mapping/Profile 或缺失资源 |
| `outputDirectory` | `output\work\<批次>\<页面>` 下的新目录 | `build`、`delivery` 或已有结果目录 |
| `verificationCommand` | 本 Skill 指定的 `uic` / 检查命令 | 手写 XML 检查器或替代发射器 |

从 `Source/HtmlDsl/Generate.ps1` 启动时，入口先校验 `build/tools/layout/tool-manifest.json` 的文件哈希、发布时 `sourceFingerprint` 和当前 Source 指纹，以及受控执行设备画像。出现以下任一错误时必须停止，不生成 `.ui`、不改 DSL、不开新解析器：

| 错误 | 唯一处置 |
| --- | --- |
| `LAYOUT_TOOL_STALE_SOURCE` | 到固定开发环境执行 `Source/Workspace.ps1 -Action build -Target layout`，成功发布后从头执行原任务卡 |
| `LAYOUT_TOOL_SOURCE_PROVENANCE_MISSING` / `LAYOUT_TOOL_AGENT_BASELINE_MISSING` | 旧 Build 缺发布证明；只能重建并发布，不可手改 `build` 清单 |
| `LAYOUT_TOOL_INTEGRITY_FAILED` | 保留原 Build，报告哈希错误；不得复制、替换或绕过 EXE |
| `PYTHON39_REQUIRED` / `ENVIRONMENT_INCOMPLETE` | 只报告已安装路径缺口；不安装、不升级、不改 PATH |

结构生成的最小调用只允许使用这条既有入口：

```powershell
pwsh -File Source/HtmlDsl/Generate.ps1 `
  -GenerationMode structure-only `
  -RawDsl '<冻结的canonical.raw.json>' `
  -MappingDirectory '<冻结的mapping目录>' `
  -Font '<冻结的字体文件>' `
  -LibraryProfile '<冻结的Profile>' `
  -ToolRoot '<冻结的build/tools/layout>' `
  -WorkRoot 'G:\ComponentLab-Delivery\DesignToCode_GenerateLayout' `
  -OutputDirectory 'output/work/<批次>/<页面>'
```

工具重建只允许在固定开发环境由工作区入口完成；运行已发布工具不要求 CMake、Visual Studio 或 Ninja：

```powershell
pwsh -File G:\ComponentLab-Delivery\Source\Workspace.ps1 -Action build -Target layout `
  -Python '<已有Python3.9.exe>' -QtRoot '<已有Qt目录>' `
  -VsRoot '<已有Visual Studio目录>' -Cmake '<已有cmake.exe>' `
  -NinjaRoot '<已有Ninja目录>'
```

## 3. 选择生成深度

| 深度 | 输入 | 输出 | 适用场景 |
| --- | --- | --- | --- |
| 结构预览 | 结构 DSL、初始样式、字体 | 结构 `.ui`、PNG、报告 | 先核对层级和位置 |
| 正式页面 | 同版 HTML + DSL、布局证据、映射、字体、SDK | 页面五件套 | 进入业务 Qt 工程 |

调用入口还必须选择 `GenerationMode`：

| 模式 | 入口参数 | 检查范围 | 输出与限制 |
| --- | --- | --- | --- |
| `strict` | 显式指定：`RunPageAudit.ps1 -GenerationMode strict` 或 `Generate.ps1 -GenerationMode strict` | 外观、属性、状态、Token、组件映射、布局和 SDK 合同 | 正式页面包；所有门禁保持不变 |
| `structure-only` | `RunPageAudit.ps1` 默认值；或 `Generate.ps1 -GenerationMode structure-only` | DSL 节点、几何布局、组件身份一对一映射、目标 Qt 类注册 | 仅输出 `.ui/.qss` 等工作副本；跳过设计外观、实例属性、状态和 Token setter，使用组件/Qt 默认外观，不得当作正式交付 |

`structure-only` 服务于快速切换组件库和早期布局验证。它不是“放宽 strict 的正式发布”，不会读取 HTML computed style 代替 DSL，也不会把未知组件伪造为已实现组件；映射缺失、身份歧义、类未注册仍然失败。组件库只要能提供目标类和最小身份映射即可，属性合同可在后续 `strict` 阶段补齐。

低能力 Agent 只能调用既有入口，不得改写 DSL、注入变量模式、猜测组件族或新建 `.ui` 发射器。结构模式不会求值 Token/变量，因此多模式变量集缺少 `variableModeBySetMap` 时将 `variableValidation` 记录为 `NOT_APPLICABLE`，而不是自行选择模式。缺失的数值 `gap` 或四向 `padding` 以确定性的 `0` 生成，并在布局 trace 的 `defaultedMetricFields` 中逐项记录。

结构模式有两种合法、互斥的容器策略：`stackMode` 为 `HORIZONTAL`、`VERTICAL`、`GRID` 或 `FORM` 时使用对应 Qt Layout；`stackMode=NONE` 或缺失时，只有 `FRAME` 的每个直接子节点都提供有限的父级相对 `left/top/width/height`，才使用 `absolute-geometry`（`QWidget` 嵌套与 `.ui` geometry）。后者保留原始 `childNode` 堆叠顺序，并写入 `trace.staticLayouts`，不补 Spacer、不按坐标重排、不猜 Auto Layout。若静态子节点缺几何则返回 `STATIC_GEOMETRY_REQUIRED`。Grid/Form 行列与跨度、组件身份以及类注册仍必须明确，不能猜测。普通非组件 `FRAME` 固定由 Qt 原生 `QWidget` 作为布局承载，不要求 contract-only 组件库额外登记该框架类；这不放行未知组件实例。`strict` 不启用 `absolute-geometry`，仍要求完整的 Auto Layout 合同。

### 3.1 显式实例展开

默认情况下，`structure-only` 将 `INSTANCE` 作为组件类叶节点处理。这保证组件解耦和最小映射稳定，但不会把组件母版内部的子层输出到 `.ui`。需要还原某个实例内部布局时，必须显式指定节点或组件名称：

```powershell
pwsh -File Source/HtmlDsl/Generate.ps1 `
  -GenerationMode structure-only `
  -ExpandInstance '2:28481' `
  -RawDsl '<包含该实例展开子树的 canonical.raw.json>' `
  -MappingDirectory '<组件库映射目录>' `
  -Font '<已有字体文件>' `
  -OutputDirectory 'output/work/structure-only/<批次>/<页面>'
```

也可以使用 `-ExpandComponent 'ComponentLab/TableView'` 对该组件名称的所有实例展开。生产生成必须以普通 DSL 作为组件身份事实源：`mainStateGroup`、`mainComponent` 和 `componentKey` 用于定位 `pixComponentTreeDslNodes` 中的组件族与母版，母版的 `childNode` 才是展开结构来源。`get_node_dsl` 的 `isDetachInstance=true` 可以作为采集/对照证据，但不能单独作为生成输入；其中身份字段为空时返回 `DETACHED_DSL_REQUIRES_BASE_IDENTITY`，不会使用 `node.name` 或变体形式的 `componentName` 猜测组件族。输出保留外层实例的 `designNodeId`、组件身份和目标 Qt 类，子节点仅作为该实例的结构子树写入。

`-ExpandInstance` 与 `-ExpandComponent` 也可随 `strict` 正式入口传入；这只改变结构采集范围，不降低 strict 的属性、Token、SDK 和发布门禁。

展开是显式白名单能力，不改变默认 `structure-only` 行为。子树为空、母版缺失、外层已有布局或子节点无法转换时，生成器以事务方式丢弃暂存子树，保留原组件类节点，并在 trace 的 `warnings` 中记录 `INSTANCE_EXPANSION_MASTER_EMPTY`、`INSTANCE_EXPANSION_EMPTY`、`INSTANCE_EXPANSION_LAYOUT_EXISTS` 或 `INSTANCE_EXPANSION_FAILED`；这些 warning 不得被解释为内部结构已还原。组件外观、Token 和行为合同仍须由 `strict` 模式及组件 SDK 单独验收。

结构模式调用示例（`RawDsl` 必须是同一设计版本的完整 DSL）：

```powershell
pwsh -File Source/HtmlDsl/Generate.ps1 `
  -GenerationMode structure-only `
  -RawDsl '<同版canonical.raw.json>' `
  -MappingDirectory '<组件库映射目录>' `
  -Font '<已有字体文件>' `
  -OutputDirectory 'output/work/structure-only/<批次>/<页面>' `
  -LibraryProfile '<组件库Profile>'
```

结构模式仍用 Python 3.9 的外围入口和受控编译器；输出目录必须位于 `output/work`，不会写入 `output/delivery`。生成后执行包内 `uic`，但不宣称组件行为、视觉一致性或消费者编译已通过。

结构预览中的未知节点使用 QWidget 表达，适用于结构核对。正式页面要求组件身份和 SDK 合同完整。

## 4. 依赖与环境

第一路径使用已有 Python 3.9、Node.js、Playwright、批准的浏览器和已发布 SDK；CMake、MSVC、Ninja、Qt 5.15.2 用于开发构建及消费者验证，不要求使用端重建 SDK。路径通过当前进程参数或环境变量传入：`D2C_PYTHON_EXE`、`QT_ROOT`、`VS_ROOT`、`CMAKE_EXE`、`NINJA_ROOT`、`NODE_EXE`、`D2C_NODE_MODULES`、`GENERATED_COMPONENT_SDK_ROOT`。不安装、升级或修改全局环境。

从 GenerateLayout 根目录检查已有开发环境；脚本实际在上一级工作区 Source：

```powershell
pwsh -File ../Source/Workspace.ps1 -Action doctor -Target layout
```

环境检查失败时停止该阶段并记录原因。

## 5. 输入清单

capture manifest 绑定 HTML、DSL、设计文件标识、根节点、字体、显式资源、布局证据和 SHA-256。正式模式必须包含独立 `layoutReference`；该文件来自设计节点布局属性读取。

### 5.1 MCP 采集（第一路径核心断点）

第一路径没有"只给链接就能生成"的旁路：必须先用 Pixso 采集 MCP 拿到**同批** DSL、布局证据、HTML 与资源。这是链路的第一个硬断点，缺失时停止并报告，不得用样例、DSL 字段或浏览器测量伪造输入。

前提：Pixso 桌面端已打开目标文件（设计文件由客户端当前打开的文档决定）；采集服务默认 `http://127.0.0.1:3667/mcp`，工具面须包含 `get_node_dsl`、`design_to_code`、`eval_script`。端口不同时用 `--endpoint` 指定；服务不可用即停止。从 GenerateLayout 根目录执行：

```powershell
# ① DSL（逐字节保存为 canonical.raw.json，同时保留 01..04 请求/响应记录）
py -3.9 -B Source/HtmlDsl/pipeline/capture/capture_mcp.py --tool get_node_dsl `
  --arguments '{"guid":"<根节点>","simplify":false,"isDetachInstance":false}' `
  --output output/work/captures/<批次>/dsl

# ② HTML/代码入口（design_to_code 响应原样保存）
py -3.9 -B Source/HtmlDsl/pipeline/capture/capture_mcp.py --tool design_to_code `
  --arguments '{"guids":["<根节点>"]}' `
  --output output/work/captures/<批次>/d2c

# ③ 布局证据（只读探针 + DSL FRAME 匹配；禁止用浏览器测量反推）
py -3.9 -B Source/HtmlDsl/pipeline/capture/layout_reference.py `
  --guid '<根节点>' --file-key '<Pixso文件ID>' `
  --dsl output/work/captures/<批次>/dsl/canonical.raw.json `
  --output output/work/captures/<批次>/layout

# ④ 落地 HTML 与显式资源（有外链资源时追加 --resource-list）
py -3.9 -B Source/HtmlDsl/pipeline/capture/import_d2c.py `
  --response output/work/captures/<批次>/d2c/04-call.response.body `
  --dsl output/work/captures/<批次>/dsl/canonical.raw.json `
  --layout-reference output/work/captures/<批次>/layout/layout-reference.json `
  --output output/work/captures/<批次>/capture

# ⑤ 核对 DSL 与 04 响应逐字节一致
py -3.9 -B Source/HtmlDsl/pipeline/capture/verify_capture.py `
  output/work/captures/<批次>/dsl
```

`layout_reference.py` 内部调用既有只读探针 `capture_structure.js`（`pixso.fileKey`/`pixso.getNodeById` 锁定文件与根节点，读取 `layoutMode`、`itemSpacing`、四向 `padding`），把 `design-structure/v1` 快照与 DSL `FRAME` 集合按 GUID 匹配后写入 `layout-reference.json`，原始 01..04 记录同目录保存。停止条件：探针文件 ID 不一致、DSL 根不匹配、任一 `FRAME` 在快照中缺失、五项指标非有限数值时停止该页面，不用默认零补齐；这些路径由 `docs/validation/formal/test_layout_reference.py` 的本地 stub 覆盖。真实 Pixso 闭环仍需采集 MCP 可用后按上述顺序执行，未验证前不得宣称闭环通过。

转换口径：对 DSL 中每个 `FRAME` 按 GUID 匹配探针节点；`gap ← itemSpacing`，`left/top/right/bottom ← paddingLeft/Top/Right/Bottom`（均为设计像素，后四项是内边距，不是节点坐标）。仅接受 Pixso 明确返回的有限数值，显式 `0` 有效；缺失、重复 GUID、身份不一致或不支持的布局模型均停止。保留原始 MCP 响应，确保布局证据独立于浏览器测量。manifest 校验只证明格式、覆盖与哈希，适配阶段再核对 HTML 测量值；两阶段都不能替代来源真实性核验。

### 5.2 清单创建与验证

```powershell
py -3.9 -B Source/HtmlDsl/pipeline/capture/capture_manifest.py create `
  --output '<capture目录>/capture.json' `
  --html '<capture目录>/page.html' `
  --dsl '<capture目录>/canonical.raw.json' `
  --root-id '<根节点ID>' --file-key '<Pixso文件ID>' `
  --font-family '<字体族>' `
  --layout-reference '<布局证据.json>'
py -3.9 -B Source/HtmlDsl/pipeline/capture/capture_manifest.py verify `
  --manifest '<capture目录>/capture.json' --mode formal
```

路径按清单自身目录解析。原始响应按字节保存，导入器只下载清单中显式声明的资源。

## 6. 组件与布局映射

组件映射使用所选 Profile。稳定设计身份对应 SDK manifest 中的 canonical ID、Qt 类和公开属性。组件初始化生成器只调用合同中的 API。

布局分派如下：

```text
HORIZONTAL -> QHBoxLayout
VERTICAL   -> QVBoxLayout
GRID       -> QGridLayout
FORM       -> QFormLayout
```

Grid 需要行、列和跨度；Form 需要行号和角色。定位信息缺失时报告错误，不能由 left/top 推断。

## 7. 页面生成顺序

```text
capture verify
  -> browser adapt
  -> component mapping
  -> layout conversion
  -> .ui/.h/.cpp/.qss/.qrc
  -> uic
  -> consumer build
  -> runtime / visual review
```

SDK 页面构造顺序为 `setupUi`、页面 QRC 注册、ComponentFactory 应用运行时属性、页面 QSS 加载。公共组件样式由 SDK 管理，页面 QSS 只表达页面专属样式。

正式入口：

```powershell
pwsh -File Source/HtmlDsl/Generate.ps1 `
  -CaptureManifest '<capture.json>' `
  -MappingDirectory '<映射目录>' `
  -Font '<字体文件>' `
  -Batch '<批次>' -Page '<页面名>'
```

正式结果进入 `output/delivery/<批次>/<页面>`，临时数据进入 `output/work`，日志进入 `output/logs`。已有页面目录拒绝覆盖。

### 7.1 固定顺序自动执行

低能力 Agent 优先使用同一入口，不要自行拆分或改写步骤：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Source/HtmlDsl/RunPageAudit.ps1 `
  -Url 'https://pixso.cn/app/design/<fileKey>?item-id=<rootId>'
```

该入口默认按固定顺序执行环境与 MCP 检查、DSL、D2C、布局证据、页面落地、资源扫描、最终导入、manifest 校验、字体解析、adapt、structure-only 与 `uic`，随后把 strict 和正式验收记为 `NOT_RUN`。仅在显式传入 `-GenerationMode strict` 时，才追加 strict 编译、strict `uic` 与正式验收。普通步骤失败记录后继续；缺少前置输入时下游标记 `SKIP`；工具完整性或路径安全失败时停止受影响分支。结果在 `output/work/audit/<批次>/result.json`，简要报告在同目录 `REPORT.md`，原始日志在 `output/logs/audit/<批次>/run.log`。默认完成即返回 0；需要 CI 失败码时加 `-FailOnError`。structure-only 成功不等于 strict 正式交付成功。

## 8. 验证

```powershell
py -3.9 -B docs/validation/check_delivery.py --formal-batch '<批次>'
pwsh -File docs/validation/formal/Verify-RelocatedConsumer.ps1 `
  -Batch '<批次>' -SdkRoot '<SDK目录>'
```

验证至少分为：输入清单、浏览器适配、生成文件、uic、消费者编译、运行状态和视觉比对。每一项单独记录，不能从上一项推导下一项。

## 9. 失败处理

| 错误 | 处理 |
| --- | --- |
| 哈希不匹配 | 重新获取同版输入并更新清单 |
| 缺少布局字段 | `strict`：回到设计节点数据补充定位信息并失败；`structure-only`：数值 gap/四向 padding 可确定性降级为 `0` 并在 trace 记录；若 `stackMode` 为 `NONE` 或缺失，可仅在每个直接子节点具备有限 `left/top/width/height` 时使用 `absolute-geometry`，否则返回 `STATIC_GEOMETRY_REQUIRED` |
| `FRAME_CONTENT_LAYER_UNRESOLVED` | 仅表示未知 DOM 包装层未定位；记录 warning，当前 FRAME 只做叶测量并保留 DSL 子树几何，不伪造 DOM 子布局 |
| `HTML_CHILD_SET_MISMATCH` | 真实 DOM 与 DSL 子节点数量、唯一性或成员集合不一致；保持 ERROR，阻断适配 |
| 未登记组件 | 记录为组件生产端待办；布局任务不自动开发组件或登记虚假实现 |
| SDK manifest 不匹配 | 核对显式选择与清单，使用匹配的已发布 SDK；重建须另有授权，外部 SDK 只读 |
| uic 失败 | 修正 `.ui` 序列化，不修改报告掩盖错误 |
| 消费者编译失败 | 检查头文件、库、QRC 和 CMake 链接 |

结构预览可用于确定节点和布局，不能替代正式组件映射。`synthetic-example` 只表示手写技术样例，不能作为 Pixso 实测证据。
