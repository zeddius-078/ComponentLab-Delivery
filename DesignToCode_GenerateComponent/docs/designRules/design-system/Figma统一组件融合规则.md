# Figma 统一组件融合规则

## 1. 目标与适用范围

本规则用于把稳定项目中的 48 项设计资产并入 `DesignToCode_GenerateComponent` 当前组件目录与 Figma 文件，形成一套统一组件集。48 项不是独立 Profile、兼容层或第二套组件库。

适用文件：`Design-Components-Build`（file key：`w92BKW1EcutKmTXkwzNHbf`）。

## 2. 硬约束

1. 公共组件身份严格一对一：一个 `ComponentLab/<CanonicalName>` 只对应一个工程公共身份；一个工程公共身份也只能对应一个 Figma 公共身份。
2. 内部 anatomy、variant、slot 可以一对多，但不得注册为第二个公共组件身份。
3. 不保留历史别名。拼写错误、Qt 类名式名称与旧项目名称通过一次破坏性迁移替换。
4. 仅视觉相似不能合并。用途、结构、公开属性、状态、事件和生命周期全部一致，才允许复用同一公共身份。
5. 部分重叠不得做“半映射”。无法证明等价时，先作为同一组件集中的独立候选项，状态标记为 `UNRESOLVED`，待证据齐全后再合并或保留。
6. Qt 类名记录在实现字段，不覆盖设计语义名。Figma 公共名采用 `ComponentLab/<PascalCaseName>`。
7. 资产采用 `<Type>Asset`，不与展示组件混用，例如 `ImageAsset` 与 `ImagePanel` 分开。

### 2.1 端到端追溯合同

统一链路为：

`01.primitive → 02.profile → 03.semantic → 04.component → ComponentLab/<CanonicalName> → 融合判定 → 工程公共身份`

一对一约束落在“公共身份”层，而不是 Figma 内部节点数量：

- 左侧唯一键：`ComponentLab/<CanonicalName>`；
- 右侧唯一键：已由源码和 Factory / Registry 证明存在的工程公共身份；
- Button、Select 等可以由多个内部 Component Set、anatomy、variant 和 slot 构成，但这些内部节点不得登记为第二个工程映射身份；
- 有工程对象时记录真实 `developmentComponentId`，不得用候选 Qt 类代替；
- 没有工程对象时固定记录 `ENGINEERING_GAP` 与 `developmentComponentId = null`，本轮不创建开发组件；
- Figma 变量绑定证明的是“可追溯到 Token”，不自动证明组件 Token 覆盖完整，也不证明 Qt 运行时已经消费该 Token。

机器可读事实源：[`component-traceability-manifest.json`](./component-traceability-manifest.json)。截至 2026-10-14，48/48 个融合输入身份均存在 Figma 节点并具有变量绑定；23 项具有源码可验证的工程身份，0 项为 `NAME_SYNC_REQUIRED`，25 项在当次组件工程审计中记为 `ENGINEERING_GAP`。其中 `Spacer`、`LayoutContainer`、`IconAsset`、`ImageAsset`、`ModelAsset` 不是 QWidget 组件，进入统一 Catalog 后应分别按 Layout Helper / Asset 的消费状态跟踪，不能继续用“是否注册组件类”判断完成度。

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

## 5. 48 项标准名与处理建议

| # | 原名称 | 标准名 | 当前动作 |
| ---: | --- | --- | --- |
| 01 | Navigation | Navbar | 合并现有 Navbar |
| 02 | SideNavigatior | Sidebar | 合并现有 Sidebar；公共组件集统一为 `ComponentLab/Sidebar` |
| 03 | TabWidge | TabWidget | 合并现有 TabWidget |
| 04 | TitleBan | TitleBar | 新增；归入 Window & System |
| 05 | Link | Link | 新增；不与 CommandLinkButton 合并 |
| 06 | Timeline | Timeline | 新增 |
| 07 | Text | Label | 合并现有 Label |
| 08 | Dividel | Divider | 合并现有 Divider |
| 09 | Progressbar | ProgressBar | 合并现有 ProgressBar |
| 10 | Scroll | ScrollBar | 合并现有 ScrollBar；不与 ScrollArea 合并 |
| 11 | Badge | Badge | 新增 |
| 12 | Message | Message | 合并现有 Message |
| 13 | Lamp | StatusIndicator | 新增 |
| 14 | ToolTip | Tooltip | 新增 |
| 15 | Tag | Tag | 新增；不与可选择 Chip 合并；已建立 `Dismissible=True/False` 正式变体，删除事件与焦点回退仍待运行时验证 |
| 16 | Spacing | Spacer | 新增布局辅助身份 |
| 17 | Stepper | Stepper | 新增；表示流程步骤，不表示数值输入 |
| 18 | DataDisplay | Statistic | 合并现有 Statistic；范围限定为一个带标注的格式化数值，不包含卡片、图表或数据集 |
| 19 | PushButton | Button | 合并现有 Button |
| 20 | DropDownButton | ToolButton | 合并现有 ToolButton；菜单能力由 `popupMode`、`QMenu` 与 `defaultAction` 承担 |
| 21 | DropDownField | Select | 合并现有 Select；不自动并入 ComboBox |
| 22 | PageHeadel | PageHeader | 新增 |
| 23 | Fitel | Filter | 新增 |
| 24 | Pagination | Pagination | 新增 |
| 25 | Transter | Transfer | 新增 |
| 26 | Upload | Upload | 新增 |
| 27 | MoreButton | OverflowMenuButton | 新增；不得用含糊的 MoreButton |
| 28 | LineEdit | LineEdit | 合并现有 LineEdit |
| 29 | Search | SearchInput | 新增 |
| 30 | Numberlnput | NumberInput | 新增语义组件；内部可适配 SpinBox/DoubleSpinBox |
| 31 | Keyboard | NumericKeypad | 新增 |
| 32 | Switch | Switch | 合并现有 Switch |
| 33 | RadioButton | RadioButton | 合并现有 RadioButton |
| 34 | CheckBox | Checkbox | 合并现有 Checkbox |
| 35 | Slidel | Slider | 合并现有 Slider |
| 36 | TimePicker | TimePicker | 合并现有 TimePicker |
| 37 | ColorPicker | ColorPicker | 新增；不与 ColorDialog 合并 |
| 38 | SegmentationButton | SegmentedControl | 新增 |
| 39 | TableView | TableView | 合并现有 TableView |
| 40 | Anchor | AnchorNavigation | 新增 |
| 41 | Collapse | Collapse | 合并现有 Collapse |
| 42 | TreeWidget | TreeWidget | 合并现有 TreeWidget |
| 43 | Window | Dialog | 按“弹窗”语义合并现有 Dialog |
| 44 | Drawel | Drawer | 现有组件覆盖 Overlay / Push；规范身份、SDK/Factory 键已同步为 `ComponentLab/Drawer` / `commonWidget::Z_Drawer`，旧类名仅作兼容入口 |
| 45 | LayoutFrame | LayoutContainer | 新增；不强制映射到单一 QWidget |
| 46 | ICONResource | IconAsset | 新增资产身份 |
| 47 | Image | ImageAsset | 新增资产身份；不与 ImagePanel 合并 |
| 48 | Model | ModelAsset | 新增 2D 原理图基础资产身份；仅包含 2D 模式下的各类原理图界面，供大型可视化组合使用 |

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

第一批本地重建范围：`Badge`、`StatusIndicator`、`Link`、`Tag`、`Tooltip`。这些组件依赖少且语义边界明确；其中 `Badge` 与 `Tooltip` 复用已有 Component Token，其他组件复用现有 Semantic/Profile Token。

截至 2026-10-14，48 项清单已完成融合判定：`23 MERGE + 25 BUILT + 0 REVIEW`。25 个 `ADD` 项已全部在同一 Figma 组件集中完成本地重建，所有重叠项复用或改名现有公共身份，没有复制成第二套组件。新增页均采用一个顶层 `<CanonicalName> · Component Contract`、一个 `ComponentLab/<CanonicalName>` 正式组件集，并按 `20.<Category>.<Item>` 排序。Figma 完成不等于工程完成：Qt 适配、Registry 映射、四 Profile 截图和运行时证据仍分别保持 `PENDING / OPEN`。

本轮闭环四项：`SideNavigatior → Sidebar`、`DataDisplay → Statistic`、`DropDownButton → ToolButton`、`Drawel → Drawer`。其中 Sidebar、Statistic、ToolButton 保留原节点 ID 并补全能力边界；原 `ComponentLab/OverlayDrawer` 在 Figma 中原位改名为 `ComponentLab/Drawer`，节点 ID 仍为 `569:632`。工程目录和 Factory 现以 `commonWidget::Z_Drawer` 为规范入口；`commonWidget::Z_OverlayDrawer` 仅保留源码、二进制符号和旧 Factory 字符串兼容，不形成第二个公共身份。

本轮新增资产：`TitleBar`、`Timeline`、`Spacer`、`Stepper`、`PageHeader`、`Filter`、`Pagination`、`Transfer`、`Upload`、`OverflowMenuButton`、`SearchInput`、`NumberInput`、`NumericKeypad`、`ColorPicker`、`SegmentedControl`、`AnchorNavigation`、`LayoutContainer`、`IconAsset`、`ImageAsset`、`ModelAsset`。`ModelAsset` 的范围已经明确：仅包含 2D 模式下的各类原理图界面，是大型可视化的基础资产层；它不代表 3D 模型、`QAbstractItemModel` 或完整交互画布。上层大型可视化负责场景组合、视口、缩放、命中检测和实时状态叠加，Qt 候选方向为 `QGraphicsScene/QGraphicsView` 或自定义 `QPainter` 渲染适配器，最终工程对象仍需从稳定项目核对。
