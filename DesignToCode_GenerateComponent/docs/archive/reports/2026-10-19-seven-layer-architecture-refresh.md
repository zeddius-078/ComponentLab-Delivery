# 2026-10-19 七层 Token 架构与组件源模型刷新报告

## 1. 变更级别

本次不是普通修订，而是一次重大架构刷新。它继承并 supersede `reports/archive/2026-10-18/2026-10-18-change-report.md` 中的 V2 迁移结果，但不覆盖原报告。旧数据继续保留为历史证据，不再作为当前 Token、Figma 或 Runtime 的事实源。

当前基线由以下文件共同构成：

```text
Registry + Component Contract + Component Recipe
        + Composite Registry/Packages + Content Catalog
        ↓
V2 Compiler
        ↓
Figma Binding Manifest / Runtime expanded output
```

## 2. 七层架构基线

当前逻辑层固定为：

```text
01 Primitive → 02 Profile → 03 Theme → 04 Semantic
             → 05 Composite → 06 Component → 07 Content
```

### 2.1 各层职责

| 层级 | 当前职责 | 当前状态 |
|---|---|---|
| Primitive | 原子颜色、尺寸、动效原子值 | active |
| Profile | 平台、分辨率、密度、Interaction Size | active |
| Theme | Light/Dark 表面与主题角色 | active-first-slice |
| Semantic | 跨组件颜色、边框、间距和状态语义 | active |
| Composite | Typography、Motion 等复合视觉特征 | package-backed |
| Component | 组件 Registry、Contract、Recipe 和稀疏覆盖 | active-source |
| Content | 英文默认、中文化及多语言实例文本 | active-empty-catalog |

七层适配不是给每个组件复制七套变量，而是通过 `source/v2-source-manifest.json#/componentLayerBindings` 登记组件参与关系。当前已登记：Button、Input、Select、Table、Checkbox、Radio、Menu、Tool Button。

## 3. 组件模型刷新

### 3.1 已完成的新源模型

- Button：Registry → Contract → Recipe → Figma Binding 已按新模型重建；旧 Figma 变体只作为迁移证据。
- Input：Mode、Appearance、State、Size 与 Runtime Action Slot 分离。
- Select：Selection、Searchable、Appearance、State、Open、Size 分离，并保留非法组合约束。
- Table：Table/Column/Cell/Editor/Runtime 数据边界分离，多级表头和生命周期 Slot 已进入 Contract。
- Checkbox：Value、State、Content、Size 分离。
- Radio：Value、State、Content、Size 分离。
- Menu：Menu Surface、Menu Atom、Runtime Action Model 分离；Loading 保留在 Runtime 生命周期语义中。
- Tool Button：独立于 Button，采用 Presentation、Value、State、Menu 四轴；旧 `checked-hover`、`hover-pressed`、`open` 等复合状态不再作为活动 State。

### 3.2 历史资料处理

旧的组件恢复数据继续保存在：

```text
docs/archive/design-system-history/history/pre-v2-recovery-20261018/
```

这些文件只用于追溯、比对和迁移诊断，不参与当前编译。当前活动源只读取 `source/` 下的 Registry、Contract、Recipe、Composite 和 Content 文件。

## 4. 数量审计基线

### 4.1 分层源统计

| 层级 | 数量 |
|---|---:|
| Primitive | 303 |
| Profile | 72 |
| Theme 基础角色 | 9 |
| Theme Dark 覆盖 | 9 |
| Semantic | 310 |
| Composite 治理记录 | 36 |
| Component 稀疏源路径 | 636 |
| Content 条目 | 0 |

Component 稀疏源明细：

| 组件 | 稀疏源路径 |
|---|---:|
| Button | 111 |
| Input | 76 |
| Select | 65 |
| Table | 105 |
| Checkbox | 15 |
| Radio | 18 |
| Menu | 24 |
| Tool Button | 22 |
| 已确认组件合计 | 436 |
| 全部 Recipe 合计 | 636 |

### 4.2 Runtime 展开统计

- Runtime 展开路径：7146
- 不同引用值：169
- 重复引用出现：6977
- 当前可解析的不同基础值约：86

Runtime 重复路径是组件轴、State、Slot 和 Property 的合法寻址结果，不等于设计侧存在同等数量的独立设计决策。详细统计见：

```text
DesignSystem/generated/reports/token-layer-count-report.json
```

### 4.3 Figma 历史证据

- `DSM/Component` 历史实测变量：2229
- Button 历史实测变量：887

这些数量是旧设计物化结果，不能作为当前 V2 源模型、组件变体或 Token 绑定基线。

## 5. 当前已知审计项

以下问题已记录，但尚未在本次历史刷新中静默修正：

1. Button Registry 的 `matrix.logicalCombinationCount` 当前写为 2700，按当前轴值计算应为 2160；这是 Registry 数据错误，待单独修正。
2. Menu Contract 已规定 `loading` 属于 Runtime Action Lifecycle，但 Registry 仍把它列在 Menu State 轴；应在下一轮 Menu 修订中移除该视觉轴值。
3. `generated/reports/design-materialization-budget.json` 仍记录旧设计源计数 639，当前编译计数为 636，形成一条非阻断性 Warning；预算报告需重新生成。
4. Gradient、Shadow、Blur Composite 包已建立结构，但当前仍没有 active governed records。
5. Content Catalog 已建立英文默认和中文 Locale 政策，但尚无具体条目。

## 6. 验证结果

```text
Component V2 generated
phase=ci
design-source=636
runtime-expanded=7146
recipes=43
diagnostics=1
```

- 阻断性 Error：0
- V2 测试：39/39 通过
- 剩余诊断：既有设计预算报告滞后 Warning

## 7. 当前事实源规则

- 当前活动源：`source/`
- 历史证据：`history/` 与日期化变更报告
- 生成产物：`generated/`
- Figma：只消费 Figma Binding Manifest，当前未执行写入
- Runtime：消费编译后的合法展开结果，不反向定义设计源

本报告由 `source/v2-source-manifest.json#/architectureBaseline/current` 指向，作为当前七层 Token 架构的历史基线。
