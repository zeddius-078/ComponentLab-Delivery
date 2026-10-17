# ComponentLab Token V2 重建验收报告

生成日期：2026-10-17 · 规范：`Token 架构重建与设计—开发同步：Agent 执行提示词 V2`
设计快照：`DesignSystem/imports/stoken.json` sha256 `276aaf099919b960906be6474cd0fbdf252a50c5e0e685c40a9829ff7d1df145`（Figma 文件 `w92BKW1EcutKmTXkwzNHbf`）

## 0. 追加批次：radio / menu / tree / table 参考值落地（2026-10-17）

按新任务要求，以参考文件（结构有问题、参数值正确）的**真实值**落地四个组件，先 Figma 后开发：

- 参考文件保存在项目内与 stoken 同级：`DesignSystem/imports/reference/{radio,menu,tree,table}.json`（冻结证据）。
- 开发侧：`scripts/v2/import_reference.py` 将参考值导入 V2 规范源——
  - 新增 19 个参考色板 primitive（`primitive.color.reference.*`，L 索引 p-* 步）＋ 1 个间距步（`primitive.spacing.5`）；
  - 新增 38 个 semantic 角色（background.secondary/tertiary、border.primary/tertiary、interactive.brand.soft、status.alarm/error/warning 六态族、size.height/padding 尺度、border-width.hairline/thick、caption 行高）；
  - 8 项共享角色 fix-value（text.disabled #BBBBBB、text.unit #4E4E4E、background.hover/pressed/disabled、brand.hover/pressed、heading4 行高 24）——参考文件重定义共享语义角色，影响范围已在 `config/reference-mapping.json` 登记；
  - 重建 `component.radio`（20 键）并新增 `component.menu`（29）、`component.tree`（12）、`component.table`（57）。
- **逐项校验**：`tests/verify_reference_values.py` 将参考文件全部 117 个属性绑定解析到终值并与 V2 组件解析值比较——**全部完全一致**（含 radio sm/md 同为 16、menu 渐变 0/100 位置、表格 5xl 尾随空格等参考文件原样特征，按值落地）。
- Figma 侧：`config/figma-rebind-plan.json` 新增 `referenceComponentRebind`，列出四组件页面现有绑定（旧值）→ 参考目标值（如 menu-hover #f3f3f3→#F5F5F5、table header-bg #00000005→#EEEEEE）；MCP 只读，设计工具重绑定为待执行外部动作。
- 待补：tree 参考文件注明的“多 icon 多实例结构”为设计侧待补项，已登记，未虚构。

## 1. 实际改了什么

### 新规范源（唯一日常写入入口）
| 文件 | 内容 | Token 数 |
|---|---|---|
| `DesignSystem/source/primitive.json` | 原始色值、尺度、字体、字重；DTCG 风格类型 + 显式单位 | 250 |
| `DesignSystem/source/profile.json` | Standard/Wide 模式（spacing/typography scale、viewport、grid、safe-area） | 72×2 |
| `DesignSystem/source/semantic.json` | Light + 稀疏 Dark 覆盖（9 项）；排版按角色聚合；metric 拆分为 border-width/radius/interaction-size/visualization-size/space 五条主干 | 272 + 9 dark |
| `DesignSystem/source/component.json` | 组件/变体/状态/部件/属性；横向组件别名全部改为指向 semantic/primitive 终值；input 尺寸枚举 small/medium/large；背景色属性统一 background-color；Figma 独有的 4 个 progress 状态键补入 | 434 |
| `DesignSystem/source/product/application-layout.json` | 17 个 application-layout 键移入产品范围包（含 Standard/Wide 模式、单位 px） | 17×2 |
| `DesignSystem/source/dictionary.json` | 固定词典：根、组、枚举、状态轴、部件/属性词汇、opacity/color 策略 | — |
| `DesignSystem/source/component-registry.json` | 组件身份、状态矩阵、私有材质登记（含 X_pushbutton 与源码 Z_PushButton 的差异登记） | — |
| `DesignSystem/schema/v2-token-schema.json` | 源文件 JSON Schema（类型、单位、命名、模式覆盖约束） | — |
| `DesignSystem/config/legacy-map.json` | 1055 条旧键→新键决策记录 + 8 条待决冲突 + 164 条组件私有材质登记 | — |
| `DesignSystem/config/manifest.json` | schemaVersion 2.0、sourceHash、上下文定义、设计快照、兼容退出条件 | — |
| `DesignSystem/config/figma-rebind-plan.json` | Figma 侧候选差异（重绑定规则、旧运行时变量处理、回滚） | — |

### 新编译链（可重复生成）
`DesignSystem/scripts/v2/`：model（加载+校验）→ resolver（上下文解析、环检测、类型相容规则）→ adapters：
- `qt_runtime`：tokens_qt.json 兼容信封（旧运行时 profile 名保持，canonical 上下文 standard/wide × light/dark 一并输出；旧 base.stoken.*/旧运行时角色为单向兼容视图，带退出条件；adapter.qt.* 为 V2 登记的计算值：32px 交互下限、12/14px 字号下限、iconContainer、hitTarget 等）
- `css`：CSS 自定义属性 + 标识符碰撞检测
- `dtcg`：DTCG 2025.10 导出（项目扩展类型带 $extensions 标记）
- `catalog`：全量 CSV 账本（每上下文解析值 + 模式敏感性）
- `compat_stoken`：旧 stoken.json 格式的单向兼容再生成
- `legacy_migrate.py`：旧快照 → V2 源的一次性迁移工具（溯源证据）
- `migrate_contracts.py`：合同/映射文件旧引用 → V2 路径（幂等）

入口：`design-assets/tools/export_tokens.py`（CLI 不变，CMake 调用不变）。

### 归档
旧 Commerly 链（primitives.json/profiles.json/widgets.json/component_library.json/ui_layout_bindings.json/qt_qss_capabilities.json/generate.py/ui_layout_contract.py/stoken_import.py）移入 `DesignSystem/archive/commerly/`，仅作冻结兼容层；`tokens/stoken-map.json` 标记 superseded-by-v2。

## 2. 采用/放弃的决策

- **采用**：四根 primitive/profile/semantic/component；border/primary 并入 border/default；语义排版按角色聚合；横向组件别名扁平化为 semantic/primitive 终值（价值不变）；application-layout 进产品域；显式单位（px/%）；opacity 源单位 %、计算系数 0–1；grid column-count/output-scale 为无单位 number（类型修正，值不变）；组件尺寸枚举 small/medium/large；32px/12px/14px 运行下限登记为 adapter.qt 计算值（业务政策保留，不写回设计源）。
- **放弃**：旧 base/semantic/component.basic 三层根；metric/space 与 spacing 同义主干；旧 stoken.json 手写格式（变为冻结快照 + V2 生成产物）；旧 comfortable/compact 密度轴与 Standard/Wide 的等价假设（分别登记，不再互认）。
- **保留**：X_pushbutton 登记为“用户提供类名”；实际源码类为 `commonWidget::Z_PushButton`（`runtime/component/Z_PushButton.h`），未改名。Commerly 冻结键（innerShadow 材质等）在合同中以 isolate 状态保留，带退出条件。

## 3. 与审计基线的等价性验证

- 旧 tokens_qt.json（git HEAD）与 V2 生成产物逐键比较（4 个运行时 profile，颜色归一化、浮点容差 1e-5）：**0 值不一致、0 键丢失**；新增 4232 个 canonical/adapter 键。
- 仅 4 个登记偏差：dark 下 `base.stoken.input.base.default.border.color` / `base.stoken.select.base.default.border.color` 由旧 light 值（gray-35）变为设计 dark 值（gray-55）——border/primary 合并到有 dark 定义的 border/default 的直接后果，运行时 UI 路径（runtime 角色）不受影响。
- 冲突路径（如 `semantic.color.background.surface`）在运行时 profile 上保留审计值（runtime-value-wins），canonical 上下文保持设计值；全部记录在 `generated/reports/v2-mapping-report.json`。
- C++ 编译器重建后产物与审计链逐字节比较（归一化哈希行）：button QSS ×4、tokens.h、button manifest、checkbox/forms catalogs、native-components.qss 全部**值一致**；差异仅为输入哈希与版本字段。

## 4. 已执行验证（命令与结果）

| 验证 | 命令 | 结果 |
|---|---|---|
| V2 编译（写） | `python -B design-assets/tools/export_tokens.py` | 通过：4 上下文、1045 规范 token |
| V2 编译（漂移检查） | `python -B design-assets/tools/export_tokens.py --check` | 通过 |
| V2 单元测试 | `python -B -m unittest discover -s DesignSystem/scripts/v2/tests` | 16/16 OK |
| 旧快照结构测试 | `python -B tools/test_stoken_import.py` | 9/9 OK |
| Button 编译器（重建，MSVC 2022 + Qt 5.15.2 msvc2019_64） | `d2c_button_token_compiler ... --check` | CHECK OK |
| Checkbox 编译器 | `d2c_checkbox_token_compiler ... --check` | consistent |
| Forms 编译器 | `d2c_forms_token_compiler ... --check` | 通过 |
| 原生组件生成 | `python -B tools/generate_native_components.py --check` | 通过（59 组件/418 输出） |
| 设计组件目录 | `python -B tools/generate_design_component_catalog.py` + `test_design_component_catalog.py` | 20/20 OK |
| 参考值逐项校验 | `python -B scripts/v2/tests/verify_reference_values.py` | 117/117 与参考文件完全一致 |
| 映射工作台变量传输 | `mapping/design/prepare.py --output ...` | 68 变量/4 profile 生成 |
| 映射工作台测试 | `test_design_normalization.py` / `test_independence.py` / `test_pixso_button.py` / `test_qt_native_catalog.py` | 21/21 + 4/4 + OK + OK |
| SDK 静态库集成构建 | `cmake --build ... --target generated_component_sdk`（MSVC 2022 + Qt 5.15.2；构建内自动跑 V2 导出 + 三个编译器 + AUTORCC 嵌入） | 成功（GeneratedComponentSDK.lib） |
| 项目整体检查 | `python -B Source/CheckProject.py` | PASS（tokenChainFiles=19） |

环境限制：Qt GUI 截图回归与 SDK 端到端运行验证未在本会话执行（无显示环境；SDK 重新发布属于外部动作）。`Source/CheckProject.py` 的 SDK 目录断言需在重新发布后通过（见第 6 节）。

## 5. 未支持范围与阻塞（如实登记）

1. **Figma 设计工具侧重绑定**：桌面 MCP 只读；变量集合重建与节点重绑定需设计工具写权限。已交付 `DesignSystem/config/figma-rebind-plan.json`（候选差异 + 规则 + 旧变量族处理），状态 `pending-design-tool-action`。
2. **Dark 不可发布**：设计 Dark 仅 9 项显式角色；条件解析仍会得到浅色主文字/白色 page。运行时 profile 保留审计期 dark 适配（登记为运行时适配层，退出条件：完整设计 Dark 或消费者迁移）。
3. **visualization-size 冲突（8 键 × 2 模式）**：文件与 Figma 值并存登记，文件值为当前运行时基线；Wide 上下文消费者保持 gated。见 `config/legacy-map.json` `pendingConflicts`。
4. **progress-bar 的 4 个设计状态**（paused/error/in-progress/completed）：设计定义，Qt 运行时未验证（ProgressAppearance 仍为硬编码色板）——登记为 design-defined-runtime-unverified。
5. **运行时 C++ 组件读取**：Z_DesignComponents/Z_FoundationComponents/PageFlowAppearance/VisualAssetCatalog 等继续读取运行时适配角色键（值不变）；迁移到 canonical 路径是带退出条件的后续批次。
6. **Appearance 家族硬编码色板**（SliderAppearance 等）：既有缺口，不在本轮试点范围，未在本轮伪称迁移。
7. **SDK 重新发布**：`Source/Publish-ComponentSDK.ps1` 由用户运行；本会话未推送远程、未覆盖未提交修改。

## 6. 旧结构如何退出

- 旧 Commerly 生成器与源：归档只读；新写入一律进入 V2 源。
- 旧 stoken.json：冻结设计快照（哈希登记）+ `generated/compat/stoken.json` 为 V2 单向再生成产物；不再是手写入口。
- tokens_qt.json 中的 legacy 键（base.stoken.*、旧运行时角色）：单向兼容视图，退出条件写入 manifest：运行时 C++ 组件迁移到 canonical 路径、合同与编译器仅消费 V2 路径、legacy 密度轴替换或移除。责任人：ComponentKit。
- `stoken-map.json`：标记 superseded，保留为归档导入器证据。

## 7. 门禁结论

- Schema/词典/解析规则：唯一且可执行（schema + dictionary + compiler 校验）。
- 逻辑键唯一、导出名碰撞检测、缺失引用/环/类型不兼容：0（编译器强制，测试覆盖）。
- 旧键处理记录：1055/1055；待决冲突 8 项独立列出，未假通过。
- 共同语义/design-only/runtime-only/产品域清单：见 legacy-map 与 component-registry；可视化冲突与 Dark 阻塞项未通过正式切换门禁，其余部分通过。
- 本会话不构成产品运行验证声明；C++ 产物与审计链的值级等价已机器验证。
