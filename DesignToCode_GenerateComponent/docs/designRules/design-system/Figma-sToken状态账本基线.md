# Figma → sToken → 状态账本基线计划（2026-10-11）

> **状态（2026-10-17）**：本文件描述的"保留现有结构、增量优化"基线已被《Token 架构重建与设计—开发同步 V2》替代。V2 已落地：规范源为 `DesignSystem/source/*`（四层 primitive/profile/semantic/component + 产品域），旧 `stoken.json` 降级为冻结设计快照与单向兼容产物，旧 Commerly 链归档于 `DesignSystem/archive/commerly/`。验收报告见 `design-assets/DesignSystem/generated/reports/acceptance-report.md`。本文件与 `figma-core-rebuild-state.json` 保留为历史证据，不再作为当前权威描述。

## 结论

第一阶段基线按以下顺序建立：

```text
Figma 设计师资源结构
        ↓
设计决策与差异确认
        ↓
stoken.json 工程序列化
        ↓
Token 导出与生成校验
        ↓
状态账本当前对齐结论
```

Git 用于保存和发布可追溯版本，不是本阶段的设计治理目标。用户已确认另一台电脑没有待同步内容，因此跨电脑文件追查不再作为第一阶段任务。

## 1. 权威边界

| 对象 | 权威职责 | 当前入口 |
|---|---|---|
| Figma | 设计师资源结构、集合与模式、变量命名、Alias 关系、组件和 Variant 身份 | 文件 `w92BKW1EcutKmTXkwzNHbf` |
| `stoken.json` | 已确认设计决策的工程 Token 表达和生成输入 | `Source/ComponentKit/design-assets/DesignSystem/imports/stoken.json` |
| 生成物 | 验证工程解析、模式组合和稳定输出 | `generated/runtime/tokens_qt.json` 及生成报告 |
| 状态账本 | 记录 Figma、sToken、生成物之间的当前对齐结果和历史证据 | `figma-core-rebuild-state.json` |

Figma 先确定设计资源结构；同步并校验后，`stoken.json` 才成为该版本的工程运行事实源。状态账本不替代两端数据，只记录二者是否一致、如何验证以及仍有哪些缺口。

## 2. 第一阶段范围

### 2.1 Figma 设计师资源结构治理

需要完成：

1. 固定四层 DesignToken 结构：Primitive、Profile、Semantic、Component。
2. 固定 Profile 与 Theme 两条独立轴：Standard/Wide 与 Light/Dark。
3. 核对变量命名、分组、描述、Scope、Mode 和 Alias 方向。
4. 核对图标、资产、组件集、Variant 和设计师常用审查页面的组织方式。
5. 清理会让设计师误选的旧集合、旧模式、失效 Alias 和重复资源；历史证据保留在账本，不继续作为主入口。
6. 先处理共享依赖，再处理局部组件，避免为消除统计差异修改全局语义。

### 2.2 同步到 `stoken.json`

每一批已确认的 Figma 变更都要形成明确差异：

- 新增、删除或重命名的 Token；
- Mode 值或 Alias 目标变化；
- Profile 与 Theme 的适用轴；
- Component Token 的实际消费组件；
- Figma 原生变量 ID 和对应 sToken 路径。

只把已确认的设计决策写入 `stoken.json`。不得根据旧截图、历史统计或未验证的视觉推断补造 Token。

### 2.3 生成与校验

每批同步后至少验证：

1. `test_stoken_import.py`；
2. `export_tokens.py --check`；
3. 生成物逐字节稳定性和 SHA-256；
4. 四种 Profile × Theme 组合的解析结果；
5. Alias 断链、环、类型冲突和缺失模式；
6. 受影响组件的 Figma 绑定回读。

完整 Design System 生成和 Qt Runtime 属于后续工程门禁。当前缺失 `TokenSwitch` 的失败必须保留，不能被局部 Token 检查掩盖。

## 3. 状态账本对齐要求

账本必须同时提供两部分：

### 当前摘要

只保存当前有效结论：

- Figma 文件、集合、模式和计数；
- `stoken.json` SHA-256 和各层计数；
- 生成物 SHA-256、解析 Token 数和验证命令；
- 本批涉及的 Figma Variable ID、Component Set ID 和节点 ID；
- Figma ↔ sToken 名称和 Alias 一致性结果；
- 当前阻塞、未验证项和证据边界。

### 历史记录

追加保存每次读写、旧计数和被替代结论。历史记录不得再通过“now”“current”等措辞冒充当前状态。

状态账本的当前摘要只有在以下条件全部满足后才能写为 `ALIGNED`：

1. Figma 结构已独立回读；
2. Figma 与 `stoken.json` 名称集合一致；
3. Alias 目标和模式值一致；
4. Token 导出与 `--check` 通过；
5. 账本中的计数、哈希和验证时间与实际文件一致。

任何一项未完成时必须使用 `PARTIAL` 或 `UNVERIFIED`。

## 4. 当前基线事实

截至 2026-10-11，本地源文件可确认：

| 层级 | 当前源结构 |
|---|---:|
| Primitive | 250 |
| Profile | Standard 89、Wide 89 |
| Semantic | Light 基础 254、Dark 8 项差异覆盖 |
| Component | 397 |

`stoken.json` SHA-256：

`ac38103d124ae0ebbdbcaf75370025724ce7d710f9a4d19ee497a053989f0069`

当前 `tokens_qt.json` SHA-256：

`d87561987bb669e4229e9524ab3b8d9c978e99857d61bb8f9bbcfc5d23746b1c`

2026-10-10 交接记录报告四种组合各解析 1994 Token。账本原 `claimStatus.currentVerified` 中 1981、388 等汇总是较早检查点，本计划将其从当前结论中移除。397 个 Component Token 与 Figma 的全量名称和 Alias 逐项一致性仍需在下一次实时审计中重跑，因此当前总体状态为 `PARTIAL`，不能提前写为 `ALIGNED`。

2026-10-11 已在当前电脑重新执行：sToken 导入测试 8/8 通过；`export_tokens.py --check` 通过，四种组合各 1994 Token，生成物 SHA-256 保持 `d87561987bb669e4229e9524ab3b8d9c978e99857d61bb8f9bbcfc5d23746b1c`；完整 `scripts/generate.py --check` 仍准确失败在缺失 `basic.switch / app.components.token_switch`。该失败继续作为明确阻塞项。

## 5. 执行顺序

### 阶段 A：建立实时差异表

- 读取 Figma 四集合和设计师资产结构；
- 读取当前 `stoken.json`；
- 生成名称、模式、值、Alias 和 ID 差异；
- 将历史 Reference 与当前 DSM 主资源分开。

完成标志：每个差异都有“保留、修改、归档或待决策”的处理结论。

### 阶段 B：优化 Figma 设计师资源

- 先处理命名、分组、模式、共享变量和图标资产；
- 再处理组件集、Variant 与审查页面；
- 每次变更保留原生 ID 或记录身份迁移；
- 对共享资源执行消费者审计。

完成标志：设计师能从稳定入口找到正确资源，不需要理解历史采集或旧集合。

### 阶段 C：同步工程 Token

- 只同步阶段 B 已确认的变化；
- 保持四层结构和 Profile/Theme 独立；
- 更新测试和生成物；
- 不在 Component 层复制无关全量变量。

完成标志：导出稳定、Alias 无断链、四种组合结果符合设计决策。

### 阶段 D：完成账本对齐

- 更新当前摘要；
- 写入 Figma 与本地源哈希、计数和验证时间；
- 将旧摘要降为历史检查点；
- 明确 Runtime、Approved 和人工评审仍处于何种状态。

完成标志：账本能准确回答“Figma 与 sToken 当前是否一致，以及证据在哪里”。

## 6. GitHub 发布策略

GitHub 每个版本保存：

- 源码和合同；
- Figma 状态账本；
- `stoken.json` 和稳定生成物；
- 必要的机器验证摘要；
- 本计划及后续对齐记录。

构建缓存、可再生中间文件、压缩包和本机 IDE 状态不进入版本库。Git 提交只表示可追溯检查点，不自动表示 Figma、Runtime 或人工批准完成。
