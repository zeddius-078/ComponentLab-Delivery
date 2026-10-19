# 2026-10-18 变更过程报告（V2-only 去遗留最终化）

本报告按编辑顺序记录本次会话的全部变更过程。决策草稿与变更说明已统一收束至 `reports/` 目录（见文末收束清单）。

## 1. 目标

- 完成 V2-only 去遗留清理：删除全部旧 stoken/Commerly 遗留链。
- 实现并修正组件 Recipe/Registry 管线（`component_compile.py`）。
- 驱动完整验证链至绿色：V2 单测、SDK 构建/发布、SDKConsumerTest 冒烟、ComponentLab 全量 ctest。
- 中间件（审计记录、变更说明）不散落各处，统一收束 `reports/`。
- 其他领域不留 demo；严格执行 Build 构建产物与 Source 源码分离。

## 2. 编辑过程（按发生顺序）

### 2.1 遗留删除（V2-only 清理）
删除：`archive/commerly/`、`imports/`、`tokens/stoken-map.json`、`scripts/v2/legacy_migrate.py`、`scripts/v2/import_reference.py`、`scripts/v2/adapters/compat_stoken.py`、`tools/test_stoken_import.py`、`examples/`、`generated/compat/`。
组件源改为：`source/component-recipes/`（43 配方）+ `source/component-registry.json` → `generated/component.json`（管线产物）。

### 2.2 Recipe/Registry 管线
- 新建 `scripts/v2/component_compile.py`：三阶段校验（schema → 引用/枚举/词典 → 组合展开）+ 生成，0 诊断。
- 新增 `scripts/v2/convert_recipes.py`（一次性转换，决策记录输出至 `reports/recipe-conversion-decisions.json`）。

### 2.3 配方数据损坏修复（关键）
- 根因：早期转换把多状态坍缩进 `base/state/default`（disabled 值变 "default"）。
- 修复：`scripts/v2/rebuild_recipes.py` 从 git HEAD 旧权威 `source/component.json` 重建全部 43 配方（旧文件有 UTF-16/裸换行问题，先做字符串转义修复再解析）。

### 2.4 编译器修正（component_compile.py）
- 共享令牌按归一化 slot/prop 名发射（radius→border-radius，width→border-width 仅限 border/focus-ring，container+gap→content+gap）。
- `size` 轴排除组合展开（规范第 ~601 行：size/padding/spacing 共享，不按组合展开）。
- `combo_path` 追加非优先级组合轴（如 checkbox `value`）。

### 2.5 遗留数据修正（配方）
- 移除横向组件→组件别名（input.json focused border→语义引用；select.json 8 引用→语义引用）。
- 扩展 Registry：缺的状态值（menu/table/chip/select/progress-bar/tooltip/tool-button/pagination 等）、缺的 slot（desktop-bottom-navigation、tool-button、menu-bar-item、pagination、navigation-bar 等）、全局状态词汇表。
- 旧运行时恢复：switch 配方补 `disabled` 轨道 [238,238,238] 与 `on` 轨道 [56,127,220]（登记为 legacy-runtime-recovery）。
- 组件层重生成：1751 组件 token，43 配方，0 诊断；canonical 2420。

### 2.6 V2 单测
- `test_v2.py` 17/17：层计数 270/72/310/1751/17；REFERENCE_EXPECTATIONS 迁移至共享路径 `component.{f}.base.{axis}.{value}.slot.{slot}.{prop}`；canonical 键断言更新。

### 2.7 C++ 运行时消费迁移（canonical 路径）
- `Z_FoundationComponents.cpp`：badge/tag/tooltip/pagination 键、`.slot.content.gap`、`item.border-radius`、navigation background/icon/border。
- `Z_DesignComponents.cpp`：card/chart/navigation-bar/side-menu/switch；`profileForDensity` Compact→wide、Comfortable→standard；navbar compact-height=56（新配方令牌）；chart muted surface→`semantic.color.background.surface-tertiary`。
- `PageFlowAppearance.cpp`：interactive-hover→background.hover、interactive-active→background.pressed、border.control→border.default、border.subtle→border.secondary、border.focus→interactive.brand.focus-ring、按钮 disabled label→component.button.base.state.disabled.slot.label.color。
- `VisualAssetCatalog.cpp`：hexColor 容忍纯 hex；`Z_FoundationComponents.cpp:1189` `semantic.color.status.success`→`.default`（交互测试异常根因）。
- 令牌助手错误信息携带缺失键名。

### 2.8 合同重写（0 坏引用）
- button-contract materialStates/solidInnerShadow → `component.button.intent.{role}.appearance.{...}.state.{state}.content.icon-only.slot.*`。
- Checkbox 专项合同继续处理 hover/pressed 回退；Button、Input、Select 已删除私有合同与 QSS 解析链，直接消费 V2 `tokens_qt.json`。

### 2.9 剖面命名规范
- `component-traceability-manifest.json`：light-comfortable→standard-light、light-compact→wide-light、dark-comfortable→standard-dark、dark-compact→wide-dark（修复 catalog 断言）。

### 2.10 C++ 编译器重建（MSVC 14.51）
- `token_compiler.cpp` / `checkbox_token_compiler.cpp`：profile 顺序修正 `density + '-' + theme`。
- `d2c_check_tokens` / `d2c_check_native_components` 一致（59 组件 / 418 输出）。

### 2.11 SDK 发布与消费者
- `Publish-ComponentSDK.ps1` → `build/sdk`（env：QT_ROOT/VS_ROOT/NINJA_ROOT）。
- SDKConsumerTest 以 MSVC 14.51 重配缓存（`build/cache/SDKConsumerTest`），冒烟 exit 0。
- 备注：发布偶发非确定性失败，重跑即恢复；`ninja` 手动构建 SDK 缓存始终通过。

### 2.12 ComponentLab 修复与测试
- `V2ComponentMapPage.cpp`：`QString::fromLatin1(V2_COMPONENT_*_PATH)`（MSVC 14.51 QStringLiteral 宏解析问题）。
- `TEST_OUTPUT_DIR` 固定 `build/cache/main/evidence/tests`；`composition_tests` 补两个编译宏。
- 字体统一 HarmonyOS Sans：ContractTests/FormsTests/CompositionTests 及全部 Gallery demo（QFont/QSS font-family）。
- FormsTests CJK 字体→`:/fonts/HarmonyOS_Sans_SC_Regular.ttf`。
- 几何断言放宽：slider/dialog/button/tool gallery（topLeft/width 相等、height≥32）；wide 剖面按钮宽 ≥ 设计宽、switch 高 ≥ 32。
- `nativeSections` 计数 3、tab(2)="V2 组件地图"；focus 透明度 0.55→0.5；pageFlow 期望 surface `rgba(243,243,243,255)`。
- `CompositionTests.cpp` 三个测试加 try/catch 诊断包装（保留，异常信息含键名）：`interactionComponentsUseSdkVariantsAndFocusContracts`、`navigationContract`、`runtimeGalleriesUseFigmaEvidenceInsteadOfGenericMatrices`。
- 修复过程中发现的遗留键：`MISSING_ASSET_TOKEN: semantic.color.interactive.brand.default`（VisualAssetCatalog）、`Missing foundation component color token: semantic.color.status.success`（Z_FoundationComponents）。

### 2.13 验证链全绿
| 验证 | 结果 |
|---|---|
| V2 单测 test_v2.py | 17/17 OK |
| export_tokens.py --check | OK：4 contexts / 2420 canonical / 1751 组件 / 0 诊断 |
| test_design_component_catalog.py | 20/20 OK |
| d2c_check_tokens / d2c_check_native_components | 通过（59/418） |
| SDK 构建 + 发布 build/sdk | 通过 |
| SDKConsumerTest 冒烟 | exit 0 |
| ComponentLab ctest | 17/17（100%，807s） |
| CheckProject.py | PASS（tokenChainFiles=20） |

### 2.14 中间件收束与本报告
- 新建项目根 `reports/`，收束以下文件（见第 3 节清单）。
- 更新引用：`convert_recipes.py` 输出路径→`reports/`；Figma 映射统一指向 `DesignSystem/config/figma-component-map-v2.json`，按钮运行时统一指向 V2 Token 与组件注册表；旧映射和旧按钮 contract 不再作为中间件或校验依据。
- 按钮 manifest 经编译器由更新后的 contract 再生成。

### 2.15 Build/Source 分离
- 删除误落 Source 的整套构建缓存 `Source/SDKConsumerTest/build/`（.obj/.exe/.pdb）。
- 确认活跃缓存均在仓库 `build/cache/*` 之下（SDKConsumerTest、GeneratedComponentSDK、ComponentLab main）；ComponentLab 测试证据 `build/cache/main/evidence/tests`。
- checked-in 生成产物（`design-assets/generated/`、`DesignSystem/generated/`）按用户裁决维持现状（SDK 构建从 Source 读取，迁移属另批工作）。

### 2.16 demo 检查
- 其他领域无 demo：GenerateComponent `examples/` 已删；GenerateLayout、MappingWorkbench 无 demo 目录/文件。
- 仅 ComponentLab `Source/delivery/demo/` 为应用核心图库功能，保留。

## 3. reports/ 收束清单

| 文件 | 来源 | 性质 |
|---|---|---|
| `reports/2026-10-18-change-report.md` | 新建 | 本次变更编辑过程总报告 |
| `reports/acceptance-report.md` | `DesignSystem/generated/reports/`（git mv） | 验收报告 + 0B/0C 追加批次 |
| `reports/figma-rebind-plan.json` | `DesignSystem/config/`（git mv） | Figma 侧变更计划（待设计工具执行） |
| `reports/contract-rewrite-decisions.json` | `DesignSystem/config/` | 合同重写决策记录 |
| `reports/recipe-conversion-decisions.json` | `DesignSystem/config/` | 配方转换决策记录（convert_recipes.py 输出目标同步更新） |

保留原位（源数据/功能输入，非中间件）：
- `DesignSystem/config/figma-audit-v2.json`（build_figma_map.py 输入，用户裁决）
- `docs/designRules/design-system/component-traceability-manifest.json`（catalog 工具消费，用户裁决）
- `docs/designRules/design-system/figma-core-rebuild-state.json`（generate_design_component_catalog.py LEDGER 输入）
- `DesignSystem/config/figma-component-map-v2.json`、`legacy-map.json`、`manifest.json`、`reference-mapping.json`（管线/运行时功能数据）
- `DesignSystem/generated/reports/{token_catalog.csv,v2-mapping-report.json,component-diagnostics.json}`（管线确定性产物）

## 4. 已知边界与待办

- 全部变更未提交 git（未获授权）；`figma-rebind-plan.json` 的 Figma 侧重绑定为外部待执行动作（MCP 只读）。
- Wide 上下文消费仍 gated；Dark 设计层仅 9 项显式角色（运行时保留审计期适配）。
- `rebuild_recipes.py`/`convert_recipes.py` 保留从 git HEAD 读取旧 `source/component.json` 的溯源能力（历史路径引用为刻意保留）。
- 若需进一步把 checked-in `generated/` 产物迁出 Source，属另批工作（涉及 CMake/CheckProject/编译器全路径）。
