# Figma 统一组件融合规则

## 1. 目标与适用范围

本规则用于把稳定项目中的 48 项设计资产并入 `DesignToCode_GenerateComponent` 当前组件目录与 Figma 文件，形成一套统一组件集。48 项不是独立 Profile、兼容层或第二套组件库。

适用文件：`Design-Components-Build`（file key：`w92BKW1EcutKmTXkwzNHbf`）。

## 2. 硬约束

1. 公共组件身份严格一对一：一个 `ComponentLab/<CanonicalName>` 只对应一个工程公共身份；一个工程公共身份也只能对应一个 Figma 公共身份。
2. 内部 anatomy、variant、slot 可以一对多，但不得注册为第二个公共组件身份。
3. 公共目录不新增历史别名。现存兼容符号不能形成第二个公共身份；删除源码或二进制兼容接口属于独立变更，需要核对消费者，不由文档整理自动执行。
4. 仅视觉相似不能合并。用途、结构、公开属性、状态、事件和生命周期全部一致，才允许复用同一公共身份。
5. 部分重叠不得做“半映射”。无法证明等价时，先作为同一组件集中的独立候选项，状态标记为 `UNRESOLVED`，待证据齐全后再合并或保留。
6. Qt 类名记录在实现字段，不覆盖设计语义名。Figma 公共名采用 `ComponentLab/<PascalCaseName>`。
7. 资产采用 `<Type>Asset`，不与展示组件混用，例如 `ImageAsset` 与 `ImagePanel` 分开。

### 2.1 端到端追溯合同

统一链路为：

`Primitive / Profile / Theme / Semantic / Composite / Content → Component Recipe + Contract → 设计公共身份 → 融合判定 → 选定目标库的工程公共身份`

七层物理源与依赖以 v2-source-manifest.json 为准，Theme 不混入 Profile；Content 只负责文本与国际化。

一对一约束落在“公共身份”层，而不是 Figma 内部节点数量：

- 左侧唯一键：`ComponentLab/<CanonicalName>`；
- 右侧唯一键：已由源码和 Factory / Registry 证明存在的工程公共身份；
- Button、Select 等可以由多个内部 Component Set、anatomy、variant 和 slot 构成，但这些内部节点不得登记为第二个工程映射身份；
- 有工程对象时记录真实 `developmentComponentId`，不得用候选 Qt 类代替；
- 没有工程对象时固定记录 `ENGINEERING_GAP` 与 `developmentComponentId = null`，本轮不创建开发组件；
- Figma 变量绑定证明的是“可追溯到 Token”，不自动证明组件 Token 覆盖完整，也不证明 Qt 运行时已经消费该 Token。

追溯登记入口为 [`component-traceability-manifest.json`](./component-traceability-manifest.json)，设计模型以当前 Registry/Contract/Recipe/Binding 为准，工程状态还须核对当前源码、发布清单与消费测试。旧清单中的数量与状态不自动升级为当前验收结果。Spacer、LayoutContainer 与资产按各自类型验收，不用 QWidget Factory 判断其完成度。

## 3. Figma 文档结构与编号

页面名格式：`20.<Category>.<Item> · <CanonicalName> / <中文名>`。

| 编号 | 分类 | 说明 |
| --- | --- | --- |
| 20.1 | Navigation | 导航、流程定位与页面切换 |
| 20.2 | Display | 只读信息与状态展示 |
| 20.3 | Actions | 触发命令或业务动作 |
| 20.4 | Input | 文本、数值和直接录入 |
| 20.5 | Selection | 从候选值中选择 |
| 20.6 | Container | 容纳、组织与布局内容 |
| 20.7 | Data Views | 集合、层级与表格数据视图 |
| 20.8 | Feedback | 系统反馈、提示与加载状态 |
| 20.9 | Window & System | 窗口、对话框与系统选择器 |
| 20.10 | Assets | 图标、图片和模型等非控件资产 |

编号只表达信息架构位置，不作为代码 ID。组件迁移时优先保留原 Figma 节点 ID；新增页只登记真实组件或明确的 `UNRESOLVED` 合同，不制作伪组件占位视觉。

### 3.1 组件页标准结构

每个公共组件页只能保留一个顶层 `<CanonicalName> · Component Contract` 根框架。禁止继续使用“说明卡 + 组件集 + Review”三个并列顶层对象的临时结构。根框架统一包含：

1. Hero：组件名称、所属分类、编号和生命周期状态；
2. System Position：组件在设计系统中的类别、语义角色、相邻组件边界和证据状态；
3. Figma Asset：唯一公共身份、真实变体轴和组件集；
4. Interaction：设计可见状态与运行时行为的责任边界；
5. Review：主题、密度和场景审查实例；
6. Design ↔ Qt Contract：设计语义、Figma 属性、Qt 候选实现和事实源；
7. Implementation Contract：结构、状态、事件、可访问性、依赖和工程缺口；
8. Acceptance：分别记录 Figma 资产、工程映射和运行证据状态。

组件的“位置”不是页面编号本身，而是 `Category + Semantic Role + Boundary + Dependency + Lifecycle` 的组合合同。若一个组件会触发导航，但自身不组织导航结构，应按其主要能力归入 Actions，并在合同中记录导航语义；例如 `Link` 归入 Actions，`Navigation`、`Sidebar`、`Tabs` 归入 Navigation。`Tooltip` 归入 Feedback，因为它是短暂上下文反馈，而不是静态 Display。

## 4. 融合判定

| 判定 | 动作 |
| --- | --- |
| 完全等价 | 复用现有公共身份，标准名覆盖旧名 |
| 同义但边界可扩展 | 扩展一个标准组件合同后合并，仍保持 1:1 |
| 只部分重叠 | 保留两个清晰命名的公共身份，不建立部分映射 |
| Figma 缺失 | 在统一分类中新增组件页与公共身份 |
| 工程缺失 | Figma 项保留 `DESIGN_READY / ENGINEERING_GAP`，不得写成已实现 |
| 证据不足 | 标记 `UNRESOLVED`，禁止自动合并 |

## 5. 命名与身份的当前依据

旧 48 项手工命名对照和阶段合并数量已归档，不再作为现行公共名称或已完成状态表。当前名称查询 Component Registry、组件目录源与 Runtime Mapping；有冲突时记录差异并核对，不从旧表自动覆盖现行源。

- 设计公共名采用 ComponentLab/<PascalCaseName>；Qt 类保留准确 API 拼写，置于实现字段。
- 不把 Qt 基类当成已实现的项目组件。模型/项驱动、命令/导航、内容/视觉与资产/控件的边界分别保留。
- Tag 应区分不可删除与可删除合同；Chip 的选择能力不与 Tag 合并。
- ModelAsset 仅指 2D 模式下各类原理图界面组成的基础大型可视化资产，不代表 3D 模型或 QAbstractItemModel；缩放、命中和实时状态叠加属于上层实现合同。
- 多库映射通过命名配置选择上下文。一个选定库内仍是一对一，不让一个设计公共身份同时解析成多个目标公共类。
- analysis-system 的 planned 类只用于类名合同预览，不填充为本统一目录的 VERIFIED 工程实现。

## 6. 迁移与验收

迁移顺序：冻结清单 → 修正页面结构和编号 → 复用/改名等价组件 → 新增缺失组件 → 更新工程目录 → 验证 Figma 与工程双向唯一性。

每个公共身份至少记录：Canonical Name、Figma Node ID、工程 Registry ID、Qt/自定义实现类型、属性与状态合同、生命周期状态、证据版本。任何重复公共身份、旧拼写残留、一个身份映射多个工程对象、或把 `UNRESOLVED` 写成 `APPROVED`，均判定失败。

工程映射验收只允许三种状态：

| 状态 | 含义 | 本轮动作 |
| --- | --- | --- |
| `VERIFIED` | 工程公共类和 Factory / Registry 路径均存在 | 只登记，不改开发组件 |
| `NAME_SYNC_REQUIRED` | 唯一工程对象存在，但公共设计身份尚未同步 | 明确差异，不在本轮改代码 |
| `ENGINEERING_GAP` | 没有可验证的工程公共对象 | 保持空值，不用 Qt 候选类冒充实现 |

## 7. 原资产不可取得时的本地重建规则

1. 当前 ComponentLab 的 Token、字体、密度、圆角和既有组件结构仍是视觉事实源。
2. GitHub 上成熟设计系统仅用于核对 anatomy、状态、键盘行为、事件和可访问性边界。首选 Ant Design、Fluent UI 与 PatternFly 的官方仓库或规范。
3. Pinterest 只作为视觉趋势与构图参考，不作为组件 API、状态或实现证据。
4. 禁止复制外部品牌视觉、专有素材或整套组件外观；所有结果必须重新绑定 ComponentLab Token。
5. 重建顺序按依赖推进：基础显示与反馈 → 输入与操作 → 选择与导航 → 复合容器与数据交互。
6. 每批必须同时完成标准 Component Contract 页、系统位置、公共身份、变体轴、组件属性、Token 绑定、审阅实例和截图检查，未完成工程映射时明确标记 Engineering Gap。

历史重建批次、节点操作与数量结论见 [原始说明归档](../../archive/2026-10-20-docs-refresh/docs/designRules/design-system/Figma统一组件融合规则.md)。本规范保留治理规则，不据历史批次宣称当前 Figma、开发组件和运行验收均已完成。
