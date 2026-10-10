# Figma DSM → Qt 基础资产交接（2026-10-10）

本文件供在另一台电脑接续工作。详细逐步证据以同目录 `figma-core-rebuild-state.json` 为准；其中较早的“待回读/旧绑定”条目是历史快照，优先看最新顶层记录，不要把历史停点当成当前状态。Figma 文件：<https://www.figma.com/design/w92BKW1EcutKmTXkwzNHbf/Design-Components-Build>。

## 权威边界与本轮原则

- 目标是 Qt 5.15.2 / Qt Widgets / C++ 的**Figma 基础设计合同**，不是宣称 SDK 已实现或运行验证，也不改 gen-layout 算法。保留已有组件集、变体、实例和真实节点 ID，增量修正。
- 用户手写 `Source/ComponentKit/design-assets/DesignSystem/imports/stoken.json` 是工程 Token 源。Figma 四个 DesignToken 集合限定为 `DSM · 01.primitive`、`DSM · 02.profile`、`DSM · 03.semantic`、`DSM · 04.component`。Profile 只有 Standard（1920）/Wide（2560）两模式，当前 Qt 导出默认 Standard；Theme 只有 Semantic Light/Dark，不由旧 QtLab Runtime 多模式代管。非 DesignToken 的 Reference 集合暂可保留作历史/工程引用。
- Native Component、Design System Component、Composite Component、Engineering Contract 分开；Figma 设计完成和 Qt Runtime Verified 分开，不自动提升 Approved。

## 2026-10-11 第一阶段基线决策

第一阶段改为以“Figma 设计师资源结构 → `stoken.json` → 生成校验 → 状态账本对齐”为主线。Figma 负责设计师资源结构和设计身份；经确认的设计决策同步到 `stoken.json` 后，`stoken.json` 作为该版本的工程生成输入；状态账本记录两端是否一致、验证证据和未完成边界。

用户已确认另一台电脑没有待同步内容，因此跨电脑文件追查不再是当前任务。Git 只作为版本发布和回退机制。详细范围、门禁和执行顺序见 `Figma_sToken_状态账本基线计划_2026-10-11.md`。

## 已落地且独立核实

1. **Profile 结构**：实时 Figma `VariableCollectionId:1400:3` 已是一个集合、89 个变量、Standard `1400:1` / Wide `1797:1`，默认 Standard。89×2 个 Figma Primitive 别名与手写 sToken 逐项一致，差异 0。用户截图的 `desktop-standard` / `desktop-wide` / `tablet-touch` 共 234 变量是旧状态；不要重新创建或移动变量组。Semantic 当前 Light/Dark，共 254 变量；Primitive 250，Component 397（最近已核实的集合快照）。
2. **ToolButton**：原组件集 `443:222`、Key `2e146b4d34575a25a30a6fb2d52442f3f57a1284`、34 变体保留。原集 224 处 DSM 变量画笔、旧 Reference 画笔 0、无变量绑定 SOLID 画笔 0；25/25 现有 Component 角色直接被原集消费（详见账本 ToolButton 条目）。原评审框 `443:223` 的八卡已在原 ID 上清除 QtLab Runtime 旧显式模式，分别保留 DSM Profile Standard/Wide 与 Semantic Light/Dark。独立回读确认每卡仍继承一件原 ToolButton 变体实例；2200×830 静态图八卡可辨，无明显裁切重叠。QAction、popupMode、checkable/checked、键盘/焦点、高 DPI、Wide Qt 运行仍未验证。
3. **Checkbox 与共享图标**：原 Checkbox 集 `235:2710` 保留 30 变体，21/21 既有 Component 角色有原集直接消费者。Check 图标集 `989:57`、Indeterminate `1011:70` 各保留 5 变体，各自五种色调的源画笔都绑定 DSM Semantic，两个源集旧 Reference 画笔均为 0。Checkbox 原集的继承图标画笔为 DSM invert 32 处、disabled 8 处，旧 Reference 画笔 0；Checkbox 页 `234:2717` 内 47 个相关 Checkbox/Check/Indeterminate INSTANCE（含嵌套）均未发现旧 `VariableID:132:*` 画笔。该结论不覆盖其他页或其他旧集合。四卡评审框 `237:2770` 已明确 Standard 与 Light/Dark；卡片旧 Runtime 模式、旧 Reference 背景/文字绑定清除；静态画面检查通过。
4. **Semantic 增量**：手写 sToken 与 Figma 既有 03.semantic 集增加 `color/text/muted`，Light gray-65、Dark gray-25；没有新建第五层。Tokens 总览原 TEXT `321:4568` 已改为 254 并独立回读与集合一致。

## 最新发现但未修改的边界

- `color/icon/primary`（Figma `VariableID:1403:19`）在手写 sToken 与 Figma 的 Light/Dark 都是 gray-90。04.component 只有两个直接别名消费者：导航 hover 图标 `1404:14510`、ScrollBar dragging 拇指 `1404:14551`。不能用旧 Check 图标的 text-primary Dark gray-5 直接推断这两处应全局改色；导航实际背景/暗色样本仍待审计。
- ScrollBar 原集 `471:477`（8 变体）的两个 Dragging 变体 `471:423` / `471:459` **尚未直接消费**上述 DSM dragging-thumb 角色：Groove/Sub/Add page、Handle 与 Sub/Add line 仍是历史 `471:*` 绑定。四个嵌套 Chevron 的 fill 已是 DSM text-primary，但 stroke 仍为旧 Reference text-primary `132:2813`：`I1007:46;1006:85`、`I1007:49;1006:101`、`I1007:70;999:69`、`I1007:73;989:60`。四个实例分别源于原 ChevronLeft/Right/Up/Down 组件集 `1006:98`、`1006:114`、`999:82`、`989:73`。先查共享源与其他消费者，再决定源迁移或实例覆盖；不要仅为消除数量直接改全局 Semantic。
- 全量设计系统生成检查仍因缺少 `basic.switch / app.components.token_switch` 退出 1。Switch 是设计系统扩展组件，TokenSwitch 的实际实现未完成；不可将其说成 Qt 原生 Switch 或测试通过。

## 本机刚执行的核验

- `python -m unittest test_stoken_import.py`：8 tests，OK。
- 在 `Source/ComponentKit/design-assets/tools` 执行 `python export_tokens.py --check`：4 canonical profiles × 1994 tokens，SHA256 `d87561987bb669e4229e9524ab3b8d9c978e99857d61bb8f9bbcfc5d23746b1c`，退出 0。
- 在 `Source/ComponentKit/design-assets/DesignSystem` 执行 `python scripts/generate.py --check`：退出 1，真实错误为 `Component module is missing: basic.switch / app.components.token_switch`。
- 这三项是 Token 源/工程检查，不是 Qt Widgets 运行画面测试。账本 JSON 已可被 PowerShell `ConvertFrom-Json` 解析。

## 第一阶段从这里继续

1. 先读取 Figma 实时四集合、图标/资产和组件资源结构，与当前 `stoken.json` 建立名称、模式、Alias、值和原生 ID 差异表；不要依据旧截图或较早账本条目推断现状。
2. 先治理设计师入口、集合分组、模式、共享图标和变量，再处理局部组件。四个 Chevron、ScrollBar 与 `color/icon/primary` 是首批共享依赖审计对象。
3. 仅将确认后的 Figma 设计决策同步到 `stoken.json`，再执行导入测试、Token 导出、四种组合解析和稳定哈希校验。
4. 最后更新账本当前摘要；Figma 名称/模式/Alias、sToken、生成物计数与哈希全部一致后才写 `ALIGNED`。Qt Runtime、Approved 和人工评审继续独立。

## Git 发布边界

项目将在当前基线建立 GitHub 版本，用于保存源码、合同、账本、`stoken.json`、稳定生成物和必要验证摘要。构建缓存、压缩包及本机临时文件不进入仓库。Git 提交只表示一个可追溯检查点，不自动证明 Figma 与 sToken 已对齐，也不代表 Qt Runtime 或人工批准完成。

复制后可用 SHA-256 核对本次交接快照（路径相对本项目目录；若交接后又编辑文件，哈希自然会改变）：

```text
AC38103D124AE0EBBDBCAF75370025724CE7D710F9A4D19EE497A053989F0069  Source/ComponentKit/design-assets/DesignSystem/imports/stoken.json
1E31A5C60D9FD84390B1D0CFABC5249E4204DE1C9E07A1DD604C9C6D25D31D18  docs/DesignSystem/figma-core-rebuild-state.json
4129B8571FE240B2C8BCB23C121C1758A27AF2ADD168E885FD40354E38852538  Source/ComponentKit/design-assets/tools/test_stoken_import.py
```
