# 仓库协作交接：变更情况与阅读指引（2026-10-18）

> 本文件是交给协同 AI Agent 的阅读入口。先读「0. 速览」建立整体认知，再按任务需要进入对应小节。
> 权威规范与机器验证优先于任何历史文档；历史文档已就地标注"历史证据，不再作为当前权威描述"。

## 0. 速览（TL;DR）

- **主战场**：`DesignToCode_GenerateComponent`（组件 SDK + Token 体系）；消费者验证场：`ComponentLab`。
- **本次核心变更**：Token 体系完成 **V2-only 去遗留**——旧 stoken/Commerly 链全部删除，组件层改由 **Registry + 43 配方** 管线生成（1751 组件 token，0 诊断）；C++ 运行时全部迁移到 **canonical 路径**；SDK、SDKConsumerTest、ComponentLab 验证链全部绿色（ctest 17/17）。
- **所有变更尚未提交 git**：工作区即现状；`git status` 可看到全部改动（含 4 个 `RM` 重命名与若干新增文件）。
- **审计记录/变更说明已收束**到项目根 `DesignToCode_GenerateComponent/reports/`；Build 产物与 Source 源码已分离。
- **Token 版本更新重点**：见第 2 节。

## 1. 仓库地图

| 领域目录 | 职责 | 本次变更 |
|---|---|---|
| `DesignToCode_GenerateComponent/` | 组件 SDK、Token 体系、合同、编译器、发布 | **主战场**（V2-only 去遗留 + canonical 迁移） |
| `ComponentLab/` | 消费 SDK 的 Qt 应用（demo 图库 + 全量测试） | 适配 canonical 路径、字体/几何断言、ctest 17/17 |
| `DesignToCode_GenerateLayout/` | HTML DSL 布局生成 | 无本次变更（工作区存在更早会话的未提交改动，非本批） |
| `DesignToCode_GenerateComponent/Source/Qt/MappingWorkbench/` | 映射工作台（36 测试） | 无本次变更 |

关键根目录：
- `DesignToCode_GenerateComponent/Source/`：源码（设计源、管线脚本、工具、运行时 C++、CMake）。
- `DesignToCode_GenerateComponent/build/`：**全部构建产物**（`cache/GeneratedComponentSDK`、`cache/SDKConsumerTest`、`sdk/` 发布包、`reports/` 构建期机器报告）。
- `DesignToCode_GenerateComponent/reports/`：**审计记录与变更说明**（本文件、验收报告、决策记录、Figma 重绑定计划）。
- `DesignToCode_GenerateComponent/docs/`：规范与技能（`mainSkill/`、`designRules/`）。
- `ComponentLab/build/cache/main/`：ComponentLab 构建缓存（测试证据在 `evidence/tests/`）。

## 2. Token 版本更新（重点）

### 2.1 阅读顺序（按权威性）

1. **规范**：`docs/designRules/design-system/Component层Token构建规范.md`（1088 行）——组件层 Token 构建唯一权威。⚠️ 第 88 行附近存在**用户未提交的手工编辑**，任何自动化重写都不得覆盖该段。
2. **Schema/词典**：`Source/ComponentKit/design-assets/DesignSystem/schema/v2-token-schema.json` + `source/dictionary.json`（根/组/枚举/状态轴/部件属性词汇）。
3. **源数据**：见 2.2。
4. **管线**：见 2.3；**生成物**：见 2.4。
5. **历史文档**（仅供溯源，不权威）：`docs/designRules/design-system/Figma-sToken状态账本基线.md`、`figma-core-rebuild-state.json`（⚠️ 后者是 `generate_design_component_catalog.py` 的 LEDGER 输入，属源数据，不可删）。

### 2.2 源文件（四层 + Registry + Recipes）

| 文件 | 内容 |
|---|---|
| `Source/ComponentKit/design-assets/DesignSystem/source/primitive.json` | 原始色/尺度/字体（250） |
| `.../source/profile.json` | Standard/Wide 模式（72×2） |
| `.../source/semantic.json` | 语义角色，Light 完整 + Dark 稀疏覆盖 |
| `.../source/component-recipes/*.json` | **43 个组件配方**（本次新建；`{family}.json`，shared + overrides） |
| `.../source/component-registry.json` | 组件身份/状态矩阵/私有材质登记（本次扩展大量状态与 slot） |
| `.../source/product/application-layout.json` | 17 键产品域（Standard/Wide × px） |
| `.../config/legacy-map.json` | 旧键→新键决策记录（1055 条）+ 待决冲突 |
| `.../config/manifest.json` | schemaVersion 2.0、sourceHash、上下文定义 |
| `.../config/figma-audit-v2.json` | Figma 审计快照（`build_figma_map.py` 的**输入源数据**，勿动） |
| `.../config/figma-component-map-v2.json` | 701 条绑定地图（SDK/ComponentLab 消费） |
| `.../config/reference-mapping.json` | 参考值共享角色影响登记 |

### 2.3 管线（scripts/v2/）

| 文件 | 职责 |
|---|---|
| `model.py` | 加载 + schema/词典/枚举校验 |
| `resolver.py` | 上下文解析、环检测、类型相容 |
| `values.py` | 值解析工具 |
| `compile.py` | 编排：adapters → 生成物注册（token_catalog.csv、v2-mapping-report.json、css/dtcg/runtime） |
| `component_compile.py` | **组件层三阶段管线**：校验（schema→引用/枚举/词典→组合展开）→ `generated/component.json` + `component-combinations.json` + `reports/component-diagnostics.json`；组合展开排除 size 轴、combo_path 追加非优先级轴、共享令牌按归一化 slot/prop 名发射 |
| `build_figma_map.py` | figma-audit-v2.json → figma-component-map-v2.json |
| `convert_recipes.py` / `rebuild_recipes.py` | 一次性转换/重建工具（**保留 git HEAD 读取旧 `source/component.json` 的溯源能力**，历史路径引用为刻意保留，勿当悬空引用清理） |
| `migrate_contracts.py` | 合同旧引用→V2 路径（幂等） |
| `adapters/qt_runtime.py` | tokens_qt.json 兼容信封 + canonical 上下文（standard/wide × light/dark）+ adapter.qt 计算值（32px 交互下限、12/14px 字号下限、iconContainer、hitTarget） |
| `adapters/{css,dtcg,catalog}.py` | CSS 变量 / DTCG 导出 / 全量 CSV 账本 |
| `tests/test_v2.py` | 17 个单测（层计数 270/72/310/1751/17；canonical 键冻结断言） |

### 2.4 生成物（`.../design-assets/DesignSystem/generated/` 与 `design-assets/generated/`）

- `DesignSystem/generated/component.json`：**组件层产物（1751 token）**；`component-combinations.json`：组合展开清单。
- `DesignSystem/generated/reports/`：`token_catalog.csv`、`v2-mapping-report.json`、`component-diagnostics.json`（管线确定性产物，`--check` 比对）。
- `DesignSystem/generated/runtime/tokens_qt.json`：运行时信封（4 运行时 profile + canonical 上下文，共 2420 canonical）。
- `DesignSystem/generated/css/tokens.css`、`dtcg/*.json`：导出面。
- `DesignSystem/generated/runtime/tokens_qt.json`：Button、Input、Select 与 Motion 的唯一 Qt 运行时 Token 输出；不再存在 button/forms 私有 catalog/QSS。
- `design-assets/generated/{checkbox,native}/*`：当前仍保留的专项/原生生成物，**四个 profile 目录名 canonical：`standard-light/standard-dark/wide-light/wide-dark`**（排序 density+'-'+theme）。

### 2.5 合同（手写，0 坏引用）

| 文件 | 关键内容 |
|---|---|
| `.../tokens/checkbox-contract.json` | hover/pressed 回退 default |
| `.../DesignSystem/config/figma-component-map-v2.json` | 当前 Figma 组件到 V2 Token 的正式映射 |

### 2.6 C++ 编译器（MSVC 14.51）

- `design-assets/tools/checkbox_token_compiler.cpp` 为当前唯一组件私有编译器；Button、Input、Select 已直接读取 V2 运行时 Token。
- ⚠️ **profile 顺序约定：`density + '-' + theme`**（standard-light 等），任何新代码都不得用旧顺序（theme-density / comfortable-compact 名已废弃）。
- 校验目标：`d2c_check_tokens`、`d2c_check_native_components`（59 组件 / 418 输出）。

### 2.7 运行时消费（已全部迁移 canonical 路径）

| 文件 | 迁移要点 |
|---|---|
| `runtime/component/Z_FoundationComponents.cpp` | badge/tag/tooltip/pagination 键、`.slot.content.gap`、`item.border-radius`、navigation 键；`:1189` `semantic.color.status.success`→`.default` |
| `runtime/component/Z_DesignComponents.cpp` | card `base.surface.card.slot.*`、chart、navigation-bar、side-menu、switch；`profileForDensity`（Compact→wide/Comfortable→standard）；navbar compact-height=56 |
| `runtime/component/PageFlowAppearance.cpp` | interactive-hover→background.hover、active→pressed、border.control→default、subtle→secondary、focus→interactive.brand.focus-ring |
| `runtime/component/VisualAssetCatalog.cpp` | hexColor 容忍纯 hex |
| 错误信息 | color/metric 异常携带缺失键名（`Missing ... token: <key>`） |

遗留键清零清单：`semantic.color.status.success(.default)`、`background.surface-muted(→surface-tertiary)`、`interactive-hover/active`、`border.control/subtle/focus`、`background.app(→page)`。

### 2.8 SDK 资源（`build/sdk/`，由 `Source/Publish-ComponentSDK.ps1` 发布）

`resources/mappings/` 含：`figma-component-map-v2.json`、`figma-audit-v2.json`、`component-registry.json`、`reference-mapping.json`、`token-manifest.json`、`design-component-catalog.json`、`component-source-map.json`、`qt-component-map.json` 等。
ComponentLab 编译宏：`V2_COMPONENT_MAP_PATH` / `V2_COMPONENT_REGISTRY_PATH` 指向 SDK 内 V2 映射。

### 2.9 命名与展开规则要点

- 路径形：`component.{family}.base.{axis}.{value}.slot.{slot}.{prop}`；组合路径追加非优先级轴（如 checkbox `value`）。
- 优先级：**size → intent → appearance → state → content**。
- **size/padding/spacing 共享，不按组合展开**（规范 ~601 行）。
- `propertyAliases`：`radius→border-radius`、`width→border-width`（仅 slot `border`/`focus-ring`）、`container+gap→content+gap`。
- 运行时剖面 canonical 名：`standard-light`、`standard-dark`、`wide-light`、`wide-dark`。

## 3. 本次变更摘要

1. **去遗留删除**：`archive/commerly/`、`imports/`、`tokens/stoken-map.json`、`legacy_migrate.py`、`compat_stoken.py`、`import_reference.py`、`examples/`、`generated/compat/`、`test_stoken_import.py`（**不得重建**）。
2. **Recipe/Registry 管线**：43 配方 + 扩展 Registry + `component_compile.py`；曾发现配方多状态坍缩损坏，已用 `rebuild_recipes.py` 从 git HEAD 旧权威源重建。
3. **运行时 canonical 迁移** + **合同重写**（0 坏引用）。
4. **剖面改名**（component-traceability-manifest.json）+ **C++ 编译器 profile 顺序修复** + MSVC 14.51 全量重建。
5. **SDK 发布 + SDKConsumerTest 冒烟 + ComponentLab ctest 17/17 全绿**（字体 HarmonyOS Sans、focus 0.5、wide 几何 ≥ 设计界等适配）。
6. **中间件收束**：`reports/`（acceptance-report.md、figma-rebind-plan.json、*-decisions.json、本指引）；**Build/Source 分离**：删除误落 `Source/SDKConsumerTest/build/` 的缓存。

## 4. 现状更新点（给协同 Agent 的行动提示）

- **git 未提交**：一切以工作区为准；`RM` 重命名（reports 两文件）已暂存。
- **报告入口**：`DesignToCode_GenerateComponent/reports/2026-10-18-change-report.md`（编辑过程全记录）+ `reports/acceptance-report.md`（0B/0C 节为最新批次）。
- **Build/Source 分离约定**：所有构建进 `build/cache/*`；Source 内只留源码 + checked-in 生成物（`generated/` 迁出属另批工作）。
- **demo 约定**：其他领域不留 demo；ComponentLab `Source/delivery/demo/` 是应用核心功能。
- **历史文档**：`docs/designRules/design-system/` 下状态账本类文件已标注历史证据；`docs/README.md` 第 18 行指向 `reports/`。

## 5. 验证命令（接手后先跑一遍确认现状）

环境（PowerShell 5.1，**无 vcvars**，每条命令自设环境）：
- MSVC：`D:\VS\VC\Tools\MSVC\14.51.36231`；Qt：`D:\QT\5.15.2\msvc2019_64`；Ninja：`D:\QT\Tools\Ninja`；Windows Kit：`C:\Program Files (x86)\Windows Kits\10`（26100）。
- 需手动设置 `INCLUDE`（msvc+ATLMFC+ucrt/shared/um/winrt/cppwinrt）、`LIB`（msvc+ATLMFC+ucrt/um）、`PATH`（Ninja+msvc bin\Hostx64\x64+Qt bin+kit bin）。

| 验证 | 命令（工作目录） | 期望 |
|---|---|---|
| V2 单测 | `python -B -X utf8 -m unittest discover -s ...\scripts\v2\tests` | 17/17 OK |
| Token 漂移检查 | `python -B -X utf8 ...\tools\export_tokens.py --check` | 4 contexts / 2420 / 1751 / 0 诊断 |
| 设计目录测试 | `python -B -X utf8 ...\tools\test_design_component_catalog.py` | 20/20 OK |
| SDK 检查 | `ninja d2c_check_tokens d2c_check_native_components`（`build/cache/GeneratedComponentSDK`） | 一致（59/418） |
| 项目检查 | `python -B -X utf8 Source\CheckProject.py` | PASS（tokenChainFiles=20） |
| SDK 发布 | `Source\Publish-ComponentSDK.ps1`（env: QT_ROOT/VS_ROOT/NINJA_ROOT） | 发布到 `build/sdk`；偶发非确定性失败重跑即恢复 |
| 消费者冒烟 | `ninja` 后运行 `sdk_consumer_smoke.exe`（`build/cache/SDKConsumerTest`） | exit 0 |
| ComponentLab | `ninja` 后 `ctest`（`ComponentLab/build/cache/main`；DEMO_PROFILE/`QT_QPA_PLATFORM=offscreen`） | 17/17（约 807s） |

## 7. Source SDK 分支（新增源码交付路径，2026-10-18）

按《GenerateComponent 增量改造》任务书落地，**Binary SDK 消费面冻结、零回归**：

| 项 | 内容 |
|---|---|
| 开关 | `Source/ComponentKit/CMakeLists.txt` 新增 `option(COMPONENTLAB_SOURCE_SDK ...)` **默认 OFF**；开启后提供 `package_componentlab_source` 自定义目标（不进 ALL、不进发布） |
| 实现 | `Source/ComponentKit/source-sdk/`（stage_source_sdk.cmake 打包脚本 + consumer-template.cmake 消费模板 + package-README.md）——**可独立删除/回退** |
| 产物 | `build/resource/ComponentLabSourceSDK/`：include/MyDesignSystem（转发头，指向 src/ 唯一物理副本）、src/（与二进制 target 完全相同的 98 个 .cpp 源码）、runtime/component-resources.qrc + 182 个 qrc 引用资源（原相对布局，路径契约不变）、design-assets 生成物（tokens_qt.json/checkbox/native QSS）、licenses、CMakeLists.txt、README.md、manifest.json（逐文件 sha256 + token sourceHash 溯源） |
| 一致性 | 源列表镜像 generated_component_sdk target（排除 NativeWidgetWrappers.cpp，同二进制）；staging 脚本校验交付 CMakeLists 所列源全部存在；开启 D2C_BUILD_TOKEN_TOOLS 时依赖 d2c_check_tokens/d2c_check_native_components |
| 消费方式 | `add_subdirectory(<包> ComponentLabSourceSDK)` → `target_link_libraries(app PRIVATE GeneratedComponentSDK::Components)`；独立 `cmake -S <包>` 亦可 |
| 已验证 | 专用缓存 build/cache/SourceSDKStage staging 成功；外部消费工程 build/cache/SourceSDKConsumerSmoke 编译+offscreen 运行 exit 0（组件创建 + tokens 资源）；Binary SDK 发布/SDKConsumerTest 冒烟/CheckProject 全部不回归 |
| 注意 | 与二进制 SDK **不要同时链接**（同符号）；消费方需先 `component_theme::install(app, "standard-light")`（触发 Q_INIT_RESOURCE），否则组件读资源失败 |

## 8. 交接注意事项（禁区清单）

1. **`G:\ComponentLab-Delivery\DesignToCode_0000` 不存在**（用户已清理）；一切路径以 `DesignToCode_GenerateComponent` 为准。
2. 不得覆盖 `Component层Token构建规范.md` 第 88 行附近的用户未提交编辑。
3. 不得重建已删除的遗留链（archive/imports/stoken-map 等）；`rebuild_recipes.py` 中 git HEAD 旧路径引用是刻意保留的溯源入口。
4. `figma-audit-v2.json` 是 `build_figma_map.py` 输入源数据；`figma-core-rebuild-state.json` 是 catalog LEDGER 输入；`component-traceability-manifest.json` 被 catalog 工具+测试消费——三者均保留原位。
5. `http://127.0.0.1:3845/mcp` 尚无已配置的 MCP 工具（信息性）。
6. 发布脚本偶发"SDK build failed"：重跑 `Publish-ComponentSDK.ps1` 即恢复；不要在其前串联杂散命令。
7. 历史文档（`docs/designRules/design-system/` 状态账本类、`reports/acceptance-report.md` 1-7 节）描述的是**变更前**状态，结论以第 0-2 节与机器验证为准。
8. 所有变更未提交 git；提交需用户授权。
