# Pixso 原生 DSM/D2C 到 Qt 第二路径

## 1. 当前结论

第二路径已将 2026-10-22 批次附件的 `Frame223506.vue` 转换为 Qt 5.15.2 静态页面，通过真实页面的 uic、独立编译和运行检查，并形成 `output/delivery/pixso-native-20261022-01` 页面源码交付批次；上一批次 `pixso-native-20261021-01`（入口 `Frame226696.vue`）保留未覆盖。当前结论是**附件转换与 SDK 适配验证通过（限定范围）**，不是线上 DSM 到 Qt 的同批全链路验收，也不是任意 Vue 工程的通用转换承诺；该页面源码包不是免安装工具包。需要恢复 Pixso 可读名称时，第二路径必须携带同批 `get_node_dsl` 证据；仅有 Vue ZIP 时继续使用稳定哈希技术名。

2026-10-21 确认的策略为：以导出属性及 DSM 到代码侧的映射为准，先恢复身份、布局与关键内容，采用 SDK 初始外观。视觉差异、未应用属性及内容矛盾如实记录，不阻断本阶段；缺图标时允许配置系统默认图标并逐项记录。原始附件不修改，组件规则仍为 `partial`，不改称完整合同。

2026-10-22 新批次沿用同一策略与 `export-first`。本批次唯一新增的转换能力是页面级 `flex-basis: 0` 的零基准降级，以及 `min-width: 0`、`align-self: stretch` 的 no-op 判定；该修补只放宽已证明等价的声明，其余 CSS 仍按失败关闭处理，详见第 5 节。

| 项目 | 当前事实 |
| --- | --- |
| 来源目标 | Pixso 文件 `tuYNoeyBnlawZTMm2aRe2w`（调用方声明，未证明与线上同批），节点 `2_23506`；附件入口 `views/Frame223506.vue` |
| 映射 | `Source/pixso_native/component-mapping.json`，版本 `0.3.0`，策略 `export-first` |
| SDK | Release `20261021T152605Z-1cd59f126fb4-a5f76e39`，当前验证 Profile 为 `standard-light` |
| 页面 | 10 个普通布局容器；17 个公开实例：Navbar 1、Sidebar 1、Button 12、TableView 2、底部导航 1 |
| DSL 命名 | 强制提供同批 `--dsl`；名称清洗后与节点 ID、短哈希组合；缺失或无法关联时拒绝生成 |
| 已通过 | Qt 5.15.2 uic、独立 C++ 编译、1920×1080 离屏运行、52 项内容/状态读回、3 个页面 SVG 加载 |
| 图标回退 | 本批次按钮均带文字，没有 icon-only 按钮，因此没有 `SYSTEM_ICON_FALLBACK` 记录；3 个 SVG 属于底部导航主图标 |
| 页面交付 | `output/delivery/pixso-native-20261022-01/TopframeUI`；旁置清单、追踪记录、截图和使用说明 |
| 未验证能力 | 同批线上来源确认、完整视觉合同、真实第二 SDK 验证、隔离副本重复生成及第二路径受控工具发布 |

17 个实例不包含 SDK 内部控件。Navbar 内的搜索输入通过公开 `searchInput()` 设置占位文字，不重复生成为页面组件。配置列出其他 Profile 不等于已经逐项运行验证。

本轮附件为 `C:/Users/ZYC/Desktop/vue-Design Components Build.zip`，SHA-256 为 `88c839b54d0ab726d4ce8a77df6804aafda759b6c5ecbef50807ad769aee14b1`；上一批次同名单文件的 SHA-256 为 `96f40ae5fac37adf6578548129bf58d4fa0b4ff14996b1d50008c06d3eba93c6`，两者是不同导出，证据与结论不互相套用。受账号和保密条件限制，不要求提供新的企业版导出、上传公司资料或在公司环境使用 ChatGPT。转换入口直接读取本地附件、下游映射和公开 SDK，不以在线 DSM、MCP 或 ChatGPT 作为生成时依赖。

文档更新不替代验收，附件转换也不证明企业版 DSM 已验证。上游带配置导出须待具备条件后独立核验，原附件结果不能替代该项。视觉一致性按已确认策略暂不作为本阶段阻断项；第二套真实 SDK 和免安装工具发布也不得从本批页面验证中推导。

## 2. 隔离边界与四个核心产物

第一路径位于 `Source/HtmlDsl`，输入为 HTML＋DSL＋layoutReference。第二路径位于 `Source/pixso_native`，输入为原生 Vue 导出件；不互调爬取器、适配器、编译器或历史结果，不在失败后静默切换。

```text
Vue SFC / 导入 / props / slots + CSS / 模式 / 本地资源
  → 静态语法与实例解析
  → 组件身份绑定 + 布局规则转换
  → 已发布 SDK 公开类和 API
  → 静态 .ui / .h / .cpp / .qss / .qrc
```

| 产物 | 维护位置 | 职责 |
| --- | --- | --- |
| 组件映射表 | `Source/pixso_native/component-mapping.json` | 来源身份、Vue 导入/标签、SDK 版本/Profile、Qt 类、属性、槽位和降级策略 |
| 组件转换引擎 | `Source/pixso_native/component_engine.py` | 类型化 C++ 调用与 customwidgets，不复制 SDK 私有实现 |
| 布局映射表 | `Source/pixso_native/layout-mapping.json` | 横纵排列及对齐规则 |
| 布局转换引擎 | `Source/pixso_native/layout_engine.py` | Qt 布局与静态控件树；由 `styled_layout.py` 接入样式树 |

入口是 `generate_page.py`；`component_binding.py` 解析实例槽位，`assets.py` 收集资源，`page_output.py` 输出页面。上游 `dsm/component_parsers.json` 不等于下游 Qt 映射，当前是待实际导出验证的名称/导入配置。`read_component_config_data` 保存的是设计组件定义清单，不是已安装的 DSM 解析配置；不能据此宣称线上 DSM 配置已经发布或回读。

## 3. 依赖和目录

| 阶段 | 必需依赖 | 不需要的依赖 |
| --- | --- | --- |
| 静态转换 | Python 3.9、Node、已有解析模块、完整 Vue/CSS/资源、映射、匹配的已发布 SDK 与公开校验工具 | Vue 3 应用运行时、Vite、Playwright、Chromium、第一路径编译器 |
| 编译/运行验证 | Qt 5.15.2 MSVC x64、匹配 MSVC、CMake、Ninja、同一 Release SDK | SDK 源码重建、Node 或浏览器参与 Qt 页面运行 |
| 生成页面运行 | 消费者工程、SDK、匹配 Qt 运行材料 | Vue、解析工具、MCP、生成记录 JSON |

本机解析模块为 `@babel/parser 7.29.8`、`postcss 8.5.28`、`@vue/compiler-sfc 2.7.16` 的 **parseComponent 文件块分割接口**。不调用其模板/脚本编译或应用运行接口；模板和有限表达式由项目静态适配器解释，不能称为 Vue 3 编译验证。

本机 Node 位于 `build/runtime/node/node.exe`，解析模块位于 `G:/ComponentLab-Vue2/Source/node_modules`。后者尚未纳入第二路径受控交付，不能承诺只复制 GenerateLayout 就能换机运行。所有路径显式传入，缺失先报告，不联网补包、不安装或升级环境。`--dsl` 是强制输入，只能指向与 Vue 导出同批的 `get_node_dsl` JSON；根节点不匹配、JSON 无法解析或没有节点成功关联时，生成器拒绝写出页面。

Source 保存维护源码，docs 保存现行说明/固定输入/验证代码，build 是受控资产。第二路径的解压、生成与验证写 `output/work/pixso-native/<新批次>`，拒绝覆盖、目录联接和越界。本次页面在暂存区完成清单、独立编译及运行校验后移入新的 `output/delivery` 批次，再逐文件复核哈希。尚无第二路径自动晋级入口；本次受控交接不等于已发布通用打包工具，也未套用第一路径验收器。

输出路径不仅检查目录片段：项目的 Source、docs、build 内即使出现 `output/work/pixso-native` 后缀也拒绝写入；迁移/隔离项目按相同目录合同检查。Windows 尾点、尾空格及备用数据流形式的歧义路径在写盘前拒绝，不能借路径别名绕过保护。

## 4. 映射与降级规则

通过文件身份、组件族、Vue `import/tag` 和已核对模块哈希绑定，不能仅按同名、颜色或尺寸匹配。设计 ID 用于查表和追踪，不进入生成器的页面特判；`mappingKey` 是本地键，不冒充 Pixso 原生 key。一个设计组件族对应一个目标组件身份，内部集合通过父组件公开 API 装配。

`sdkRelease`、`profiles`、`pageHost` 由配置控制。当前根类为 `designer_widgets::ThemeHost`，在 setupUi 前注入 Profile。切库应更换映射与真实 SDK，不是只替换前缀；已有自包含配置切换测试，不代表第二套真实 SDK 通过。

| 情况 | strict（缺省） | export-first（本次显式采用） |
| --- | --- | --- |
| partial 规则 | 拒绝 | 允许已配置 API，保留未完成项 |
| 未映射属性/不支持枚举 | 拒绝 | 保存原值和未应用原因，不猜默认值 |
| 内容矛盾、未消费内容、边界外观差异 | 拒绝 | 记录差异并采用 SDK 外观，不覆盖公共 Token/QSS |
| 无可见图标 | 无隐式通用替代 | 仅按显式 fallback 配置使用系统图标并记录 |
| 模块哈希不符、身份不唯一、SDK/Profile 不匹配 | 拒绝 | 同样拒绝 |
| 非法 API、损坏资源、越界、不支持布局 | 拒绝 | 同样拒绝 |

`export-first` 不是关闭全部门禁。未知组件不自动猜 SDK 类；第一路径结构预览的 QWidget 降级规则不能当作第二路径已实现的能力。

槽位按实例作用域解析，不通过全局 DOM ID 或文字顺序猜语义：

- `textBindings` / `textListBindings`：唯一文本与显式有序列表，拒绝重复消费。
- `collections`：父组件内部条目、标签、选中索引和图标，不重复展开为页面级控件。
- `tableBinding`：显式表头和行列槽位生成只读 `QStandardItemModel`，不虚构编辑业务。
- `publicChildren`：只调用声明过的公开访问器；Navbar 使用 `searchInput()->setPlaceholder()`，不查找私有子控件。
- `iconBindings`：优先使用合法源资源；缺失时按配置采用 `SP_FileIcon`。损坏资源不按缺失掩盖。图标先赋值，accessibleName 在 icon-only 之前设置；它不作为可见按钮文字。

本批次关键内容包括 Navbar 的 `Alex Morgan` 和 `Search workspace`、Sidebar 六个路由标签（`dashboard` 为当前路由）、两个 3×3 表格、底部主/次导航及索引，均由 52 项读回逐项核对。上一批次底部第二项的 `&nbsp;` 是源插值字符串的字面值，本批次底部主项为 `首页`/`aaa`/`设置`，未出现该字面值。设计实例属性、可见性标记和表格标题/示例文本的重复声明保留未应用记录，不生成已知会抛错的 SDK 调用。上一批次的系统图标不是恢复出的设计资产；本批次没有 icon-only 按钮，因此没有系统图标回退记录。

## 5. 静态语义和布局范围

支持有限 v-if、声明默认值、标量表达式、具名插槽及父级词法作用域，不执行附件业务脚本。动态插槽、循环、事件和不可静态确定的表达式未完整支持。

支持显式 flex 行列、至少三层嵌套、整数像素 gap/四边 padding、有限主轴 spacer 和交叉轴对齐；零间距显式写入。普通 border-box 容器支持受约束的固定尺寸、100% 填充、单个 flex-grow:1 和矩形 overflow:hidden。width:auto 不直接解释为 Hug；calc、换行/反向 flex、复杂网格、一般百分比、滚动与复杂叠层未完整支持。

2026-10-22 批次新增三条页面级声明的处理，均为保持行为的最小放宽，不放宽失败关闭门禁：

- `flex-basis: 0`（或 `0px`）按 CSS 语义替换主轴尺寸，条目吸收主轴剩余空间；只接受零基准，且要求 `flex-grow: 1` 与 flex 父级，否则分别以 `UNSUPPORTED_FLEX_BASIS`、`FLEX_BASIS_ZERO_REQUIRES_FLEX_GROW`、`FLEX_BASIS_PARENT_REQUIRED` 拒绝。
- `min-width: 0` 判定为 no-op（Qt 最小宽度本就是 0），只记录不写入页面；`min-width` 的其他取值仍按 `UNLOWERED_CSS` 拒绝。
- `align-self: stretch` 仅在交叉轴尺寸为确定长度时判定为 no-op；交叉轴尺寸不确定或父级方向未知时拒绝。

这三条声明的处理结果逐条写入生成记录的 `layoutDeclarations`，no-op 项另发 `PAGE_LAYOUT_NOOP` 警告。本批次共 3 条 no-op（内容区 2 条、内容区内层 1 条），1 条 `flex-basis` 记为 applied。旧版转换器对本批次附件会报 `UNLOWERED_CSS: min-width`，对上一批次附件的同类声明同样失败，属于旧版能力缺失而非本批次导出特例。

页面 QSS 当前只转换普通容器背景色，使用精确 objectName 选择器。布局交给 QLayout，组件外观、字体和 Token 来自 SDK；其余视觉声明逐项记录，不称为完整 CSS 转译。

1920×1080 是显式验证视口，不是浏览器实测。模式脚本的重复 ID、未激活目标和时序差异保留在证据中；没有浏览器运行证据时不宣称与 Vue 初态完全等价。

## 6. 生成命令

以下在 GenerateLayout 根目录执行。尖括号替换为实际路径或未使用批次；生成器文件系统参数要求绝对路径。命令不安装依赖。

执行前须确认三组输入：原始导出及完整引用文件；已有 Python 3.9、Node 和解析模块；与映射匹配的已发布 SDK 及校验工具。`--main` 和 `--modes` 是输入根目录内的实际入口/模式脚本，不是可任意填写的占位值。仅含组件 SFC 的 MCP 返回包不等于完整应用 ZIP，不能从另一批附件补入这些文件后声称同批。以下命令只复现已核对的附件。

```powershell
$projectRoot = (Get-Location).Path
$run = Join-Path $projectRoot 'output/work/pixso-native/<新批次>'
$node = Join-Path $projectRoot 'build/runtime/node/node.exe'
$modules = '<已有解析模块 node_modules 的绝对路径>'
$sdk = '<已发布 SDK 的绝对路径>'
$verifier = '<工作区绝对路径>/Source/SDKContract.py'
$mapping = Join-Path $projectRoot 'Source/pixso_native/component-mapping.json'
$page = Join-Path $run 'TopframeUI'

py -3.9 -B -c "import sys; assert sys.version_info[:2] == (3, 9), sys.version; print(sys.version)"
if ($LASTEXITCODE -ne 0) { throw '需要已有 Python 3.9；停止，不安装环境' }
foreach ($requiredPath in @($node, $modules, $sdk, $verifier, $mapping)) {
  if (-not (Test-Path -LiteralPath $requiredPath)) { throw ('缺少已有输入或工具：' + $requiredPath) }
}
py -3.9 -B $verifier verify --sdk $sdk --configuration Release
if ($LASTEXITCODE -ne 0) { throw '已选 SDK 校验失败；不重建或切换其他 SDK' }

py -3.9 -B Source/pixso_native/acquire.py unpack `
  --input '<原始 Vue ZIP 的绝对路径>' --output (Join-Path $run 'attachment')
if ($LASTEXITCODE -ne 0) { throw '附件解压失败' }

# 本次附件的真实目录；其他导出应先核对 input-manifest.json。
$inputRoot = Join-Path $run 'attachment/raw/vue3/src'
py -3.9 -B Source/pixso_native/generate_page.py `
  --node $node --modules $modules --input $inputRoot --output $page `
  --mapping $mapping --sdk $sdk --sdk-verifier $verifier `
  --entry views/Frame223506.vue --main main.ts --modes utils/variables.js `
  --instance root --node-id 2_23506 --page-name TopframeUI `
  --profile standard-light --source-file-key tuYNoeyBnlawZTMm2aRe2w `
  --viewport 1920 1080 --viewport-source 'validation viewport; not browser measurement' `
  --dsl (Join-Path $run 'dsl/canonical.raw.json')
if ($LASTEXITCODE -ne 0) { throw '第二路径生成失败；读取该命令的 JSON 错误' }
```

`--source-file-key` 只是调用者声明，不证明同批。`--node-id 2_23506` 来自导出树，不混同 URL 中的 `2:23506`。原 ZIP 哈希和解压成员写入 input-manifest.json；新采集应另存，不能和旧附件改写为同批。`--dsl` 为必填，指向与本次导出同批的 `get_node_dsl` 原始 JSON（`dsl/canonical.raw.json` 为该原始返回体），缺失时生成器直接失败。

```text
TopframeUI/
├─ form/TopframeUI.ui
├─ include/TopframeUI.h
├─ src/TopframeUI.cpp
└─ view/res/
   ├─ qss/TopframeUI.qss
   ├─ TopframeUI.qrc
   └─ assets/<内容哈希>.svg
```

生成器必须使用 Pixso 的可读图层/组件名称，命令中必须提供：

```powershell
--dsl '<同批 get_node_dsl 返回 JSON 的绝对路径>'
```

例如，底部导航会生成类似 `px_IconPlacement_Top_2_27027_<hash>` 的 Qt `objectName`。节点 ID 与哈希后缀仍用于唯一性和追踪。没有 DSL、名称缺失或无法关联时不猜测，也不再保留哈希回退，直接失败并要求修复输入批次。

`.ui` 是静态控件树，`.h` 提供公开访问器，`.cpp` 直接调用公开 API，不重放 Vue/DSL/JSON。业务动作由消费者连接，不自动补造信号行为、Tab 顺序或 buddy 合同。

旁置 `TopframeUI.generation.json` 保存源实例、objectName、槽位、未应用属性、资源和产物哈希。`STAGED` 只证明写入及哈希通过，`acceptance: not-run` 不因消费者执行而静默改写；RUNNING、FAILED 或损坏记录都不是成功。

## 7. 验证命令

真实页面必须传 `--generation-record`；不传时是独立 API 探针，不能把其附加控件算入目标页面验收。

```powershell
$qt = '<已有 Qt 5.15.2 msvc2019_64 绝对路径>'
py -3.9 -B docs/validation/pixso_native/check_consumer.py `
  --page $page --page-name TopframeUI --generation-record ($page + '.generation.json') `
  --sdk $sdk --qt $qt --cmake '<已有 cmake.exe 绝对路径>' `
  --ninja '<已有 ninja.exe 绝对路径>' --vsdev '<已有 VsDevCmd.bat 绝对路径>' `
  --work (Join-Path $run 'consumer')
if ($LASTEXITCODE -ne 0) { throw '检查失败；读取 consumer/result.json 和对应日志' }

# 当前进程中指定已有 Qt，供继承 API 的头文件检查使用。
$env:QT_ROOT = $qt
py -3.9 -B -m unittest discover -s docs/validation/pixso_native -v
if ($LASTEXITCODE -ne 0) { throw '第二路径 Python 回归失败' }
py -3.9 -B Source/CheckProject.py
if ($LASTEXITCODE -ne 0) { throw '项目结构检查失败' }
```

Node 测试独立运行，不包含在 Python 的 126 项计数内：

```powershell
foreach ($test in @('static_expression','template_syntax','vue_graph','css_cascade',
                     'style_graph','content_evidence','collection_evidence')) {
  & $node (Join-Path $projectRoot ('docs/validation/pixso_native/test_' + $test + '.cjs')) $modules
  if ($LASTEXITCODE -ne 0) { throw ('静态解析测试失败：' + $test) }
}
```

消费者仅在子进程中加载已有 VS/Qt 环境。CMake 的 moc `-f<Page>.h` 选项处理 Qt 5 中文相对 include 编码，不修改工具安装。结果、截图和日志留在指定工作目录，生成页面运行不依赖这些材料。

验收读取本次 `consumer/result.json` 及其绑定的生成记录、页面和 SDK 哈希，不从测试计数或旧目录推断通过。第一路径回归须另外运行，分组入口见[验证代码说明](validation/README.md)；原生路径测试通过不会代替第一路径测试。

## 8. 证据、限制与交接

2026-10-22 批次的本机证据位于 `output/work/pixso-native/20261021-23506-01`：`attachment/`（原始 ZIP 解压与 input-manifest.json）、`dsl/canonical.raw.json`（同批 `get_node_dsl` 原始返回体）、`TopframeUI/`（暂存页面）与旁置 `TopframeUI.generation.json`；`consumer/` 为暂存页面的 uic、独立编译、1920×1080 离屏运行与 52 项内容/状态读回，`consumer-delivery/` 为页面移入交付目录后的同一套检查，两处 `result.json` 字节完全相同（`b988399257d66f7d…`）。交付目录 `output/delivery/pixso-native-20261022-01` 共 12 个文件，其中页面运行所需文件为 8 个，其余为说明、清单、追踪记录和截图；完整历史日志、Vue 输入与生成器不进入页面包。本批次未执行隔离副本重复生成、重复生成字节比对、跨盘与干净 Windows 离线运行，上一批次的对应结论不套用。

以下三段为 2026-10-21 上一批次（`pixso-native-20261021-01`）的历史证据，保留用于对照，不作为本批次结论。

本机锚点位于 `output/work/pixso-native/20261021-native-01`：`export-first-page-04` 及旁置生成记录、`consumer-final-01/result.json` 和 `page.png`。工作证据可以清理；不存在时应重新执行，不能从文档数字判定新版本通过。

上一批次的交付验证位于 `output/work/pixso-native/20261021-handoff`：仅复制第二路径源码及验证代码的 `isolated-project` 从原始 ZIP 重新生成，`consumer-isolated` 完成独立消费；`consumer-staged-delivery` 对交付暂存页面再次编译运行。8 个页面文件与此前同输入结果逐字节一致。交付目录共 12 个文件，其中页面运行所需文件为 8 个，其余为说明、清单、追踪记录和截图；完整历史日志、Vue 输入与生成器不进入页面包。

按最终确认的唯一附件重新执行的结果位于 `output/work/pixso-native/20261021-attachment-final`：144 个成员重新解压，当前源码重新生成页面，Qt 5.15.2 uic、独立编译与 76 项内容/状态读回通过；8 个页面文件与既有交付逐字节一致，未覆盖交付批次。该次生成不调用 MCP、在线 DSM、ChatGPT 或第一路径。

输出保护修补后的隔离复验位于 `output/work/pixso-native/guard-20261021/isolated/output/work/pixso-native/run`：副本不含第一路径源码，重新解压、生成、uic、独立编译及 76 项读回通过；8 个页面文件仍与交付一致。原交付清单保留其实际生成时的源码指纹，不修改旧清单伪装为修补后的源版本。

| 验收项 | 当前状态 | 范围 |
| --- | --- | --- |
| 文件字节与已发布 SDK 完整性 | 通过 | 附件/保留原生导出分别校验，页面资源与 SDK 清单按哈希检查 |
| 线上同批来源 / 新 DSM 配置发布 | 未执行 | 当前目标截图与 DSL 已读取；带配置导出返回企业账号限制，未发布配置；线上读取不能追认为原附件同批 |
| 已映射 API 与当前 Profile | 通过 | 当前 SDK、standard-light、已配置槽位/初态；六族映射仍为 partial，不代表完整合同 |
| 完整组件视觉合同 | 未执行 | 未应用属性和外观差异保留记录，不把 export-first 候选提升为完整等价 |
| Qt 5.15.2 uic | 通过 | 真实 TopframeUI.ui |
| 独立编译 | 通过 | MSVC x64 Release，公开 SDK |
| 运行/资源 | 通过 | 本批次限于 17 个实例、52 项读回、页面 QSS、3 个 SVG；本批次无 icon-only 按钮，没有系统图标回退记录 |
| 视觉一致性 | 未执行 | 已批准不阻断此阶段，不宣称像素还原 |
| Python 回归 | 通过 | Python 3.9.13，126 项（含 4 项 DSM 配置关联检查、5 项输出边界检查及本批次新增 7 项布局规则检查）；缺 QT_ROOT 时头文件检查会跳过，须如实报告 |
| Node 静态解析回归 | 通过 | 文档所列七组测试共 107 项，不包含在 Python 测试计数内 |
| 重复生成/中文空格路径 | 未执行（本批次） | 本批次只生成一次，未做重复生成字节比对；上一批次在本机 8 个文件哈希相同并通过消费者编译运行，不是跨盘或干净 Windows 验收 |
| 第一路径布局回归 | 通过 | 限定范围：最近 61 项中 60 通过、1 项目录联接测试跳过；不等于浏览器采集全链路 |
| 第一路径 formal 合同回归 | 通过 | 46 项通过；NTFS 迁移夹具只对编译器子进程显式指定受控 DLL/插件路径，未改第一路径内核 |
| 项目顶层回归 | 通过 | 11 项固定输入、隔离及路径边界检查；不是对子目录测试的递归总计 |
| 两路径静态及副本运行隔离 | 未执行（本批次） | 本批次未做隔离副本复验；上一批次副本不含第一路径源码，重新解压、生成和消费通过，且显式共享既有工具/公开 SDK，不是操作系统级访问隔离 |
| 页面源码交付 | 通过 | 本批次新交付目录暂存、独立消费、完整清单及移动后哈希复核通过；没有覆盖已有页面 |
| 第二 SDK / 独立离线工具包 | 未执行 | 配置接缝已测；无第二套真实 SDK，解析依赖尚未受控交付 |

本批次告警为：组件映射差异 17、采用 SDK 外观 17、页面视觉差异 10、未消费组件内容 4、布局 no-op 3。上一批次另有源内容矛盾 12、系统图标回退 12、页面视觉差异 6，不适用于本批次。类别可重叠，不能相加作为组件数量。

交接以该批次 `README.md` 和 `manifest.json` 为入口，完整差异保留在旁置 `TopframeUI.generation.json`。再次生成仍须核对当前源码、映射、同一 SDK 和本次消费者结果，使用新批次；不递归扫描旧报告获取发布资格，不覆盖业务已加工页面，不将本源码交付包称为免安装包。

回退仅涉及第二路径源码与相关说明/验证代码，不重写第一路径、原始附件、SDK 或安装环境。源模块或 SDK 变更后应重新核对映射，不机械更新预期哈希来消除错误。

## 9. 上游 DSM 配置与本次原生复核

维护文件为 `Source/pixso_native/dsm/component_parsers.json`。它不自动载入画布、不自动修改 DSM，也不参与 `generate_page.py` 的 Qt 类选择；Qt 选择仍由 `component-mapping.json` 决定。两者通过相同的导出标签和真实 Vue 模块路径关联，避免把 DSM 与 Qt 合同混成一份配置。

| 设计组件族 | 定义 ID | DSM 候选规则键 / Vue 标签 | Vue 模块 |
| --- | --- | --- | --- |
| ComponentLab/Navbar | 380:113 | ComponentLabNavbar | components/Componentlabnavbar.vue |
| ComponentLab/Button | 2258:15827 | ComponentLabButton | components/Componentlabbutton.vue |
| Legacy/Input/Plain | 339:111 | LegacyInputPlain | components/Legacyinputplain.vue |
| Legacy/TableView | 717:21 | LegacyTableView | components/Legacytableview.vue |
| ComponentLab/DesktopBottomNavigation | 1312:96 | ComponentLabDesktopBottomNavigation | components/Componentlabdesktopbottomnavigation.vue |
| ComponentLab/Sidebar | 392:216 | ComponentLabSidebar | components/Componentlabsidebar.vue |

规则键取自实际组件定义清单的 `key`，标签及 default import 路径在附件和本次普通原生导出中均存在。这证明关联有来源，但不证明 DSM 实际匹配已成功。配置只声明 `name`、对应 `__imports__` 和 `props.showTrueValue=true`；不猜测 props.filter 的覆盖行为，不添加未经验证的 text/icon/traverse/object 规则。省略 traverse 不能解释为全部子内容已保留。公开文本、图标、子项槽位及默认值过滤仍需真实 D2C⁺ 输出核对。

配置结构依据附件 Skill 及[Pixso 组件解析器合同](https://pixso.cn/developer/zh/d2c/component-parsers.html)。当前 MCP 暴露的是 `design_to_code({clientFrameworks:"vue", guids:[...], componentParsers:{...}})`；没有独立 `codeOptions` 参数，故本次不构造 codeOptions 文件，也不宣称修改了线上配置存储。

```powershell
# 在 GenerateLayout 根目录执行；采用已提供 DSM Skill 的原校验器。
py -3.9 -B '<已解压 DSM Skill 绝对路径>/scripts/validate_mapping.py' `
  Source/pixso_native/dsm/component_parsers.json
py -3.9 -B -m unittest discover -s docs/validation/pixso_native -p test_dsm_config.py -v
```

本次六条规则通过附件结构校验器；4 项项目检查验证一对一标签、实际模块路径、最小字段和名称归一化无碰撞。这些检查不模拟 Pixso。未调用独立 `validate_code_options.py`，原因是本次没有 codeOptions 对象；不是该项已通过。

2026-10-21 的普通导出保存在 `output/work/pixso-native/20261021-dsm-01`，权限复核保存在 `output/work/pixso-native/20261021-dsm-02`。只读确认的活动文件为 `uY1I2ZbcaYEK6HP92KWubw`，目标 `2:26696` 为 `Z/topframeUI`，实际父页面 `320:2720`；不能将历史链接的 `380:2` 当作当前父页面。

- 普通原生 Vue 导出：31 个代码文件、93 份资源全部落地，原始与资源本地化副本分开保存。
- DSL 命名复验位于 `output/work/pixso-native/dsl-named-20261021`：21 个节点成功关联名称；底部导航生成 `px_IconPlacement_Top_2_27027_d1bae4d479...`，表格生成 `px_Size_Small_State_Default_2_26947_c27cca358...`。该批次只验证命名，不替换既有交付页。
- 与原附件比较：14 个代码文件字节相同、15 个仅经资源哈希证明的 URL 文本差异；`Frame226696.vue` 有其他差异，新 `Vector249813.vue` 在原附件中不存在。93 份资源字节相同。这不是同批证明，旧交付不会随新设计自动变为通过。
- 最新复核中，截图与未拆实例的 DSL 已成功读取，根节点尺寸为 1920×1080；这些是当前设计参考，不是原附件生成时的证据，也不作为本轮 Qt 转换的补入数据。
- 带非空配置的 D2C⁺ 探针返回 `D2C+ is only available for enterprise accounts.`，确认当前账号不能执行该导出。未取得配置输出，没有写入画布或线上 DSM，不重试绕过账号限制。
- 新普通 D2C 返回包仍未提供 main.ts、路由和模式脚本；缺失文件未补造，也未拼入原附件再宣称同批。当前可重现 Qt 交付依然以原附件为输入。

2026-10-22 批次的 DSM 侧证据限于结构与关联层面：同一份 `dsm/component_parsers.json` 的 6 条规则在本批次导出件的标签与实际模块路径上重新核对通过（`test_dsm_config.py` 4 项通过），组件配置证据落在 `output/work/pixso-native/20261021-23506-01/probe-dsm-config/canonical.raw.json`。本批次同批 `get_node_dsl` 原始返回体由项目内 Recorder 脚本访问本地 MCP 端点取得（内置 Pixso MCP 会话在本机返回 `-32000 Bad Request: No valid session ID provided`，未使用该通道），保存为 `dsl/canonical.raw.json`，根节点 `2_23506` 与导出树一致，命名按节点 ID 关联。画布侧复核（活动文件、父页面、目标节点 `2:23506`）本批次未重做，`--source-file-key tuYNoeyBnlawZTMm2aRe2w` 仍只是调用方声明；D2C⁺ 探针受账号限制的结论沿用上一批次，本批次未再发起。

本轮按唯一附件完成本地转换，不继续索取无法提供的企业资料。若未来获准验证 D2C⁺，须重新核对活动文件与目标节点，取得实际使用的配置及完整导出，再按文件/资源哈希、真实匹配、属性/插槽、Qt 生成的顺序验收。原文件及已有交付保持不变；当前配置只能作为集成测试基线，不能作为全文件生产替换配置。
