# Figma 核心设计系统审计基线

> 文件：`Design-Components-Build`（`w92BKW1EcutKmTXkwzNHbf`）  
> 审计日期：2026-10-07  
> 目标：把当前设计文件升级为 `DesignToCode_GenerateComponent` 与 `ComponentLab` 的 UCD 规范事实源。

> **历史基线**：本文件的数量、缺口和优先级结论限定于 2026-10-07 审计时点。后续进展以[现有状态账本](figma-core-rebuild-state.json)、Figma 原节点及仓库当前代码分别核验；不能将本基线的 19 页或“仅三个试点”描述用于当前覆盖率，也不能以设计截图替代 Qt 运行证据。

## 1. 结论

当前 Figma 已有高质量 Ant Design 参考稿和 Button、Checkbox、Select 三组 DSM 试点，但缺少统一的信息架构、治理规则、Figma→Qt 身份合同与发布门禁。因此它目前是“参考素材 + 组件试点”，尚不是可以直接作为工程权威依据的设计系统。

本轮改造采用三层事实结构：

1. **规范层**：设计原则、基础视觉、组件分类、状态与可访问性规则。
2. **合同层**：Figma 身份、Qt 类型、SDK 类型、属性/状态/事件、Token/QSS 与证据的映射。
3. **参考层**：Ant Design、华为设计系统与历史 PPT，只提供设计依据，不直接拥有项目事实优先级。

## 2. 已确认事实

| 领域 | 已确认事实 | 状态 |
|---|---|---|
| Qt 基线 | Qt 5.15.2；86 个登记类型，其中 59 个具体可视类型 | CONFIRMED |
| Qt 属性 | 400 个 `Q_PROPERTY` 已核验，错误数为 0 | CONFIRMED |
| SDK 命名 | `Q<Class>` 映射为 `commonWidget::Z_<Class>` | CONFIRMED |
| 逻辑组件 | 当前令牌/组件库中有 15 个逻辑组件 | CONFIRMED |
| 运行时主题 | light/dark × comfortable/compact，共 4 个模式 | CONFIRMED |
| 页面绑定 | 当前工程侧记录 619 条 UI 布局绑定 | CONFIRMED |
| Figma 全局 | 19 个页面、785 个变量、87 个组件集、21 个独立组件 | CONFIRMED |
| Figma 试点 | Button、Checkbox、Select 已有较完整变体与变量绑定 | CONFIRMED |
| Figma 工程绑定 | 三个试点尚未完成当前 Qt/SDK 身份审批 | UNRESOLVED |

## 3. 事实源优先级

发生冲突时按以下顺序处理：

1. 当前 Figma 核心设计系统中标记为 `APPROVED` 的规范与组件身份。
2. `Source/DesignEvidence/packages/DesignTest-2-145/canonical/index.json` 的可追溯证据。
3. `Source/ComponentKit/component-definitions/native-components.json` 与组件合同。
4. `design-assets/tokens/*.json`、profiles、widgets 与 UI layout bindings。
5. Ant Design、华为设计系统和历史 PPT，仅作为外部参考。

任何未完成身份审批的条目必须标记为 `PROPOSED` 或 `UNRESOLVED`，不得以视觉相似度推断为已实现。

## 4. 建议的 Figma 页签目录

| 页签 | 应包含的内容 | 首轮状态 |
|---|---|---|
| `00 · Core Design System` | 使命、消费者、事实源、生命周期、责任边界 | 新建 |
| `01 · Start Here` | 页签地图、阅读路径、状态图例、贡献规则 | 新建 |
| `10 · 通用设计基础` | 理念、视觉、布局、动效、语言、全球化、隐私、可访问性 | 新建 |
| `11 · Tokens & Runtime Profiles` | base/semantic/component 三层 Token、四模式、QSS 输出规则 | 新建 |
| `20 · 控件总览` | 华为七类控件 + Qt 专属扩展、覆盖矩阵、成熟度 | 新建/复用空页 |
| `21 · Component Contract` | anatomy、slot、property、state、event、asset、token、QA 合同 | 新建 |
| `30 · Figma → Qt Contract` | Figma 身份 → canonical contract → SDK/Qt/QSS 的单向映射 | 新建 |
| `31 · Release & Evidence` | 评审门禁、证据状态、发布/回滚、版本策略 | 新建 |
| `20.3.01 · 操作类 / Button` | Button 规范、变体、行为与工程映射 | 现有页升级 |
| `20.5.01 · 选择类 / Checkbox` | Checkbox 规范、变体、行为与工程映射 | 现有页升级 |
| `20.5.02 · 选择类 / Select` | Select 规范、变体、行为与工程映射 | 现有页升级；Qt 身份待决 |
| `90.* · Reference` | AntD 源稿、反馈组件源稿、Layout 源稿、历史 PPT | 现有页归档 |

## 5. 控件分类

沿用华为“控件”下的导航、展示、操作、输入、选择、容器七类，同时为桌面 Qt 补充数据视图、反馈、窗口与系统三类。分类解决设计检索，Qt 类型表解决工程实现，两者通过组件合同关联，不把类继承树直接复制成设计分类。

## 6. 首轮发布边界

- Button、Checkbox、Select：保留现有设计资产，进入身份与工程合同校准。
- 其余 Qt 可视组件：在覆盖矩阵中登记，先给出 `REFERENCE / PROPOSED / UNRESOLVED` 状态，不伪造已完成的设计资产。
- AntD 与 PPT：统一移入参考层，保留可追溯来源但不再作为隐式权威。
- C++/Qt 不在 Figma Code Connect 当前受支持语言列表中；本项目使用 Figma 页面内合同 + 仓库 JSON/Markdown 映射，Code Connect 保持待支持状态。

## 7. 关键未决问题

1. Select 的最终工程身份应落在 `QComboBox → Z_ComboBox`、专用 `Z_Select`，还是分层组合合同。
2. Dropdown/Menu 与 Select 的语义边界及复用关系。
3. 现有提案组件合同的设计审批人、工程审批人与版本号。
4. 59 个可视 Qt 类型中哪些进入公共设计资产，哪些仅保留为工程能力。

## 8. 2026-10-07 实施结果

当前 Figma 已形成 39 页的受治理目录：

- 4 个目录分隔页。
- 8 个核心治理页：Cover、Start Here、Foundations、Tokens、Coverage、Component Contract、Figma→Qt Contract、Release & Evidence。
- 9 个控件分类页：导航、展示、操作、输入、选择、容器、数据视图、反馈、窗口与系统。
- 3 个已提升的 DSM 试点页：Button、Checkbox、Select。
- 15 个参考页：AntD 组件、Layout、Feedback 与历史 PPT，全部位于 `90.*`。

核心页与分类页统一为 1440px Auto Layout，并绑定既有 `QtLab / Runtime` 语义颜色。Button 的 5 个组件集、Checkbox 的 2 个组件集、Select 的 19 个组件集均已写入合同状态说明。所有新增页面均通过缺失字体、横向自适应布局和变量绑定检查。

这次升级完成的是“权威目录、治理合同、覆盖分类与现有试点接管”。除 Button、Checkbox、Select 外，其余组件仍需按 `31 · Release & Evidence` 的五道门禁逐个转正；参考页不计入已实现覆盖率。

