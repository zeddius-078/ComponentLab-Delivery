# ComponentLab V2 组件系统化改造方案提示词

## 0. 使用方式

本文件是一份面向 Figma 设计侧、Design Token 侧、Vue/Qt 开发侧和组件生成器的统一执行提示词。

执行目标不是在旧组件上继续补丁式扩展，而是以 V2 新规范为唯一目标，完成组件架构、Component Contract、Figma Component、Figma Variable、Token Recipe、Runtime API 和生成验证的协同重建。

本轮目标组件：

```text
Button
Checkbox
Radio
Table
Select
Input
```

后续将按照同一套架构和验证路径，继续改造剩余组件。

目标 Figma 文件：

```text
https://www.figma.com/design/w92BKW1EcutKmTXkwzNHbf/Design-Components-Build
```

本提示词不授权直接修改 Figma。执行 Figma 写入前，必须先完成只读审计、结构映射和方案验证；获得明确执行授权后，才允许创建或修改 Component Set、Variable Collection 和绑定关系。

不得向以下目录写入临时方案、课件或未经批准的中间产物：

```text
G:\ComponentLab-Delivery\build
```

## 1. 总体架构原则

### 1.1 V2 是必要纠错和重大架构更新

旧版 sToken 和旧组件结构只作为历史迁移基线，不得继续作为 V2 的结构依据。

旧资料处理方式：

```text
旧组件源文件
    ↓
docs/archive/design-system-history/history/pre-v2-recovery-20261018/
    ↓
只保留视觉值迁移参考
    ↓
按 V2 Contract 和 Recipe 完全重建
```

不得把旧版 `state`、`item/content/icon` 等模糊层级、完整笛卡尔积或历史命名直接搬入 V2。

### 1.2 单一权威来源

工程侧权威来源由以下文件共同组成：

```text
source/component-registry.json
source/component-contracts/*.json
source/component-recipes/*.json
source/figma-bindings/*.json
source/v2-source-manifest.json
```

职责边界：

```text
Component Contract
    → 组件结构、能力、运行时语义、合法配置

Component Registry
    → 注册组件轴、Slot、Overlay 维度、解析顺序和生成边界

Component Recipe
    → 视觉 Token 到 Slot/Property 的映射

Figma Binding Manifest
    → Figma Component、Slot、Variable 与 Canonical Token 的对应关系

Generated Outputs
    → 自动生成产物，不得直接编辑
```

### 1.3 Token 与 Runtime 的边界

Token 负责：

- 颜色、尺寸、间距、边框、圆角、排印、阴影、渐变、模糊和动效参数；
- 组件 Slot 在某种视觉状态下的表现值；
- Overlay 激活后的视觉结果。

Runtime API / Component Contract 负责：

- 数据绑定、权限、事件、行为和状态转换；
- 选择集合、焦点记录、编辑草稿、校验和提交；
- 分页、虚拟滚动、刷新、冻结和数据源；
- Action 的目标范围、可见性、可用性和执行状态；
- Overlay 什么时候激活以及作用于哪些对象。

重要规则：

```text
Runtime 决定“什么时候发生、作用于谁”
Token 决定“发生后视觉上是什么样”
```

不得把行为关系、业务条件、字段绑定、查询规则或状态转换写进单个 Token。

### 1.4 共享属性按属性类型复用

优先复用公共 Token：

```text
Size
Interaction Size
Spacing
Padding
Typography Composite
Shadow Composite
Gradient Composite
Blur Composite
Motion Composite
```

组件 Recipe 只建立组件所需的 Slot 映射，不复制 Primitive 或公共 Shared 数值。

如果某属性在多个组件中具有同一含义，统一命名，不同时使用两个表示相同语义的 `base`、`share` 或 `shared` 名称。

### 1.5 禁止无约束笛卡尔积

组件必须先定义：

1. 合法轴；
2. 组件结构；
3. Slot；
4. 合法组合；
5. Overlay 解析顺序；
6. Runtime API 能力；

之后才生成 Token。

不得把所有状态、内容、尺寸、外观、意图和行为轴全部相乘生成组合。可复用属性使用 Base/Shared 绑定，差异使用稀疏 Overlay 或明确的组合级覆盖。

## 2. 统一迁移流程

每个组件必须按照以下闭环执行：

```text
1. 读取旧 Recipe、Registry、Figma 组件和运行时映射
2. 识别历史问题和可迁移视觉值
3. 建立 V2 Component Contract
4. 建立有限视觉轴、Slot 和 Overlay
5. 建立 Figma Component 结构
6. 建立 Figma Variable 与 Binding Manifest
7. 建立 Runtime API 边界
8. 编译 Token 和 Component 产物
9. 执行结构、引用、组合和版本校验
10. 输出错误条目、定位路径和修复建议
11. 通过视觉/交互/运行时验收后再进入下一组件
```

每次只完成一个可验证切片：

```text
对齐一项
    → 修改对应源文件
    → 更新规范和 Manifest
    → 生成产物
    → CI 校验
    → 测试
    → 输出结果
    → 进入下一项
```

## 3. Figma 统一规则

### 3.1 每个组件只保留一个公开设计身份

每个组件建立一个新的 V2 公共 Component Set。旧的 TableView、DataTable、TableWidget 等历史身份只能进入 Legacy 归档，不得继续作为三个并列公共组件。

推荐结构：

```text
ComponentLab/{ComponentName}
```

### 3.2 Variant 只表达稳定视觉轴

Variant 用于表达设计师可以稳定选择、并且具有明确视觉差异的轴。

以下内容默认不做 Figma Variant：

- Runtime 数据源；
- 权限；
- 事件；
- 查询；
- 分页；
- 虚拟滚动；
- 冻结；
- 编辑提交；
- 业务字段类型；
- 任意深度的表头树；
- 所有 Action 组合；
- 所有状态的笛卡尔积。

### 3.3 Figma Variable 命名

Figma 变量集合：

```text
DSM/Component
```

Table 变量前缀：

```text
table/
```

Canonical Token 前缀：

```text
component.table.
```

示例：

```text
Figma Variable:
table/overlay/row-interaction/hover/slot/row/background-color

Canonical Token:
component.table.overlay.row-interaction.hover.slot.row.background-color
```

Figma Variable 不保存 Runtime 触发关系，只保存绑定到视觉属性的值。

## 4. Button V2 改造规则

Button 以新规范完全重建，旧 Button 只归档。

### 4.1 Intent

```text
primary
secondary
operation
danger
dark
```

`dark` 仅表达深色/反色表面环境，不表示新的业务意图。

`default` 不作为 Button Intent。`default` 只在确实表达初始交互状态时使用。

### 4.2 Appearance

```text
filled
outlined
dashed
filled-inner-shadow
text
link
```

所有 Intent 默认支持上述 Appearance；合法组合由组件 Contract 登记，不直接生成无约束笛卡尔积。

### 4.3 Content

```text
icon-only
icon-left-text
text-icon-right
text-only
icon-left-text-icon-right
```

`icon-only` 具有独立结构形状，通常为方形；尺寸轴可以复用公共 Size，但结构通过独立 Content/Slot 表达。

### 4.4 状态和 Loading

- 交互状态与 Appearance、Intent、Content 独立建模；
- Loading 由业务状态控制，组件提供接口；
- Loading 时整体视觉表现与按下状态一致；
- Loading 时 Content 整体替换为动态 Spinner；
- Spinner 的尺寸、颜色、线宽、弧长/起止角等视觉参数由 Spinner Token 管理；
- 不把 Spinner 动效结构展开成 Button 状态轴。

### 4.5 Button 共享 Token

优先复用：

```text
Size
Interaction Size
Padding
Spacing
Typography Composite
Spinner Composite
```

Button Recipe 只映射 Intent、Appearance、State、Content 到视觉 Slot。

## 5. Checkbox V2 改造规则

Checkbox 必须区分两种公开组件内容结构：

```text
control-only
control-with-label
```

### 5.1 轴和状态

- `value` 与 `state` 独立；
- `value`：`unchecked / checked / indeterminate`；
- `state`：`default / disabled / focused / hover / pressed`；
- `pressed` 只表达视觉按下，不独立保存业务值；
- `disabled` 保留当前 value，同时锁定交互；
- `indeterminate` 的转移由 Runtime/业务逻辑管理；
- 焦点环只在键盘交互需要时显示；
- 指示器与 Label 共享同一个交互表面。

### 5.2 尺寸

```text
compact → 20px
small   → 24px
medium  → 28px
large   → 32px
```

Interaction Size 可引用大一号交互尺寸，并按 4px 步进管理。

Checkbox 不因表格行选择、表头全选和业务布尔字段而复制视觉 Token；这些场景复用 Checkbox 视觉，但通过不同 Component Contract 区分语义。

## 6. Radio V2 改造规则

Radio 采用与 Checkbox 一致的模型原则，但不复制 Checkbox 的业务语义：

- `value` 与 `state` 独立；
- `control-only` 与 `control-with-label` 结构保持一致；
- 支持 `compact / small / medium / large`；
- 键盘焦点与按下视觉独立；
- `disabled` 锁定交互但不改变当前值；
- 单个 Radio 只表达自身值；
- 选项互斥、选中迁移和 Group 校验由 Radio Group/Runtime 管理。

## 7. Input V2 改造规则

Input 不复用 Button 的 Intent/Appearance 序列。Input 是独立的输入组件家族，不包含 Form Label、Help Text 或 Error Message 容器。

### 7.1 Input 类型

至少覆盖：

```text
plain
password
search
dropdown-search
file-browser
```

其中：

- `plain`：普通文本输入；
- `password`：只切换密码显示/隐藏；
- `search`：搜索输入；
- `dropdown-search`：从 Input 角度定义的复合输入，下拉浮层与输入搜索组合；
- `file-browser`：输入框 + 文件浏览触发器 + 清除能力的复合输入变体。

### 7.2 单行、多行和只读

```text
single-row
multi-row
```

- `multi-row` 只表达多行能力；
- 具体高度、可输入行数由开发 API 控制；
- 多行输入不提供原生 Clear Button，但保留外部清除 API；
- 只读与禁用视觉合并；
- Loading 优先级高于其他可用性状态；
- Loading 时右侧显示 Spinner，输入锁定，整体视觉与禁用一致；
- Placeholder 复用当前 State 的文本颜色，不建立专属 Placeholder Color Token；
- Focus 不启用 Focus Ring；任何情况下都不启用焦点环；
- Hover 边框为黑色；Focus 边框使用高亮色；Error 优先于普通交互状态；
- Password/Search 的功能图标属于固定结构 Slot，不新增无关状态轴。

### 7.3 Input 复合 Token

Typography、Gradient、Shadow、Blur、Motion 等复合 Token 通过公共 Composite 包管理，Input 只引用和绑定，不复制字体或特殊效果定义。

## 8. Select V2 改造规则

Select 需区分父组件、Popup、Option Item 和内容 Tag：

```text
Select
├── Trigger
├── Content
│   ├── Select All / Unselect All
│   └── Option Item
│       ├── Checkbox
│       └── Label
└── Tags
```

### 8.1 Single / Multiple

- `single` 仅表示单项选择；
- `multiple` 允许多个 Tag；
- Single 与 Multiple 的父组件状态集合可以不同；
- Option Item 的状态不能直接复制 Select 父组件状态；
- 多选 Option Item 复用 Checkbox 视觉，但 Option Item 保持自己的交互语义；
- Option Item 点击文本也可以执行选择；
- 禁用项不能选中，但可以聚焦并显示焦点环；
- Popup 打开期间选项状态原则上保持稳定，关闭时可触发后端校验。

### 8.2 Select All

```text
unchecked
indeterminate
checked
disabled
```

- 首行固定并用分隔线与 Option Item 分开；
- 全选/取消全选作用于当前全部搜索结果；
- 不可选择项排除在统计范围之外；
- 所有选项异常时仍可进入取消状态，但取消后不能重新激活；
- Clear 与 Popup 状态独立；
- Popup 关闭后清除临时搜索文本，Multiple 的搜索场景按已确认规则保留。

### 8.3 Tag 与搜索区域

Multiple Select 的 Content 区域分为：

```text
Tag Display Area
Search Trigger Area
Clear / Dropdown Trigger Area
```

支持：

- `single-row` Tag 排布；
- `multi-row` Tag 排布；
- 单行 Tag 溢出渐消；
- 多行 Tag 超出后显示省略号；
- 空 Tag 时显示 Placeholder；
- 搜索只改变 Popup 结果，不改变父 Select 的结构和已选值。

## 9. Table V2 改造规则

Table 是复合数据表格，不是单一视觉变体矩阵。

当前权威文件：

```text
source/component-contracts/table.json
source/component-recipes/table.json
source/figma-bindings/table.json
source/component-registry.json#/components/table
```

### 9.1 视觉轴

只保留：

```text
density: compact / small / medium / large
striping: none / zebra
```

交互 Preset、列角色、内容类型、编辑器、查询、分页、虚拟滚动、冻结、合并和 Action 不进入视觉 Variant 笛卡尔积。

### 9.2 Table 结构

```text
Table
├── Header
│   ├── Header Row
│   ├── Header Group
│   ├── Header Leaf
│   ├── Sort Indicator
│   ├── Filter Trigger
│   └── Header Action Group
├── Body
│   ├── Row
│   ├── Cell Content
│   ├── Cell Editor
│   ├── Cell Action
│   └── Selection Control
├── Loading State
├── Empty State
├── No Match State
├── Error State
└── Footer（结构占位，内部 Slot 暂不冻结）
```

### 9.3 多级表头

- 表头使用递归树；
- `group` 表示分组表头；
- `leaf` 唯一绑定 `columnId`；
- `colSpan` 根据叶子列数量自动派生；
- `rowSpan` 根据表头树深度自动派生；
- Group 默认不承担排序和筛选；
- Sort/Filter 默认只作用于 Leaf。

### 9.4 Column Schema

每列至少包含：

```text
id
role
field?
display?
editor?
editable?
required?
formatter?
slots?
```

展示、编辑和 Cell Action 独立：

```text
cell-content
cell-editor
cell-action
```

### 9.5 Runtime 能力

以下能力全部由开发侧 API 管理：

```text
freeze
cellSpan
dataWindow
dataRefresh
editingCommit
keyboardInteraction
overflowPresentation
linkedHighlight
queryCapabilities
actionAvailability
scrolling
```

表格滚动条必须复用公共 `scroll-bar` 组件，不复制滚动条 Token。

### 9.6 Overlay

独立 Overlay：

```text
row-interaction
column-interaction
row-selection
cell-selection
data-status
availability
content-role
border-emphasis
```

错误优先：

```text
data-status.error
```

可以覆盖 Disabled、Selection、Hover 和普通边框。Overlay 的激活关系由 Runtime 管理，Token 只保存视觉值。

### 9.7 Action Target

```text
table
header
column
row
cell
selection
```

Column Action 通过 Header Action Slot 承载，不能由 Column Hover 自动触发。Hover 只负责视觉反馈。

## 10. Figma 与开发侧同步要求

### 10.1 Figma 侧

- 每个组件只创建一个 V2 公共 Component Set；
- 旧组件标记为 Legacy，不再并列作为公共设计组件；
- 结构差异使用嵌套 Component、Instance Swap、Boolean 和 Slot；
- 不将 Runtime 行为做成 Variant；
- 变量放入正确的 `DSM/Component` Collection；
- 使用 Figma Binding Manifest 检查 Variable 与 Canonical Token 的对应关系；
- Figma 的 Component 结构必须能反向解释 Component Contract；
- Figma 视觉样例不代表 Runtime 能力已经实现。

### 10.2 Vue / Qt 开发侧

- Vue 与 Qt 分别提供开发侧组件库；
- 设计师和业务开发者不承担完整组合复杂度；
- Runtime API 负责状态、数据、权限、查询、提交和事件；
- Qt 原生能力不能直接等同于完整组件能力；
- QTableView/QTableWidget、QLineEdit、QTextEdit、QScrollBar 等必须经过组件适配层；
- QSS 只能表达平台支持的视觉属性，不能替代数据模型、交互逻辑、Span、虚拟化或提交事务；
- Vue 与 Qt 的组件映射必须使用同一份 Contract 和 Recipe，不得各自维护一套语义。

## 11. 复合 Token 统一管理

以下复合 Token 使用独立包：

```text
Typography
Gradient
Shadow
Blur
Motion
```

设计侧：

- 使用统一 Token 类型；
- 必要时采用 Figma Local Style + Variable 协同；
- 复合效果由角色、层级和 Component Binding 表达；
- 不把复杂复合值伪装为普通 Color/Dimension Token。

开发侧：

- 可以写入对应 Composite 配置文件；
- 由适配器转换为 Vue/CSS、Qt/QSS/绘制代码或运行时动画参数；
- 设计侧引用与开发侧运行时引用必须保留可追溯映射。

Motion 必须包含课件定义的完整类型体系，但课件只作为参考值来源，最终值必须经过评审。Spinner Motion 当前作为正式可发布的动态 Spinner 能力，不再使用 `candidate + releaseBlocked` 阻断已确认的 loading Spinner 结构。

## 12. 自动验证与门禁

每次落盘后自动执行：

```text
1. JSON 解析
2. Registry ↔ Contract 引用校验
3. Contract ↔ Recipe Slot 校验
4. Token 引用解析
5. Overlay 维度和合法组合校验
6. Component 版本一致性校验
7. Figma Binding Manifest 路径校验
8. 生成 component.json
9. 生成 component-combinations.json
10. 单元测试
```

输出必须包含：

- error / warning 分类；
- 组件名；
- 文件路径；
- JSON Pointer 或配置定位；
- 组合条件；
- 触发的规则；
- 建议修复动作。

阻断性 Error：

- 找不到组件、Contract、Recipe 或 Slot；
- Registry 与 Contract 结构不一致；
- Token 引用不存在；
- Figma Binding Manifest 指向不存在的 Canonical Token；
- 版本不一致导致无法解析；
- 合法组合无法确定或生成结果缺失。

Warning：

- 可解析但存在迁移风险；
- 历史值缺少可靠来源；
- 当前值可用但尚未完成视觉评审；
- 结构已存在但尚未有运行时适配。

成功标准：

```text
diagnostics = 0
CI check = pass
tests = pass
generated output = no drift
```

## 13. 本轮交付顺序

### 阶段 A：架构和源文件落盘

- 完成 Contract、Registry、Recipe、Figma Binding Manifest；
- 归档旧源文件；
- 更新组件规范；
- 保持 Generated Output 由编译器生成。

### 阶段 B：方案验证

- 对 Figma 做只读审计；
- 对 Vue 和 Qt 进行组件映射检查；
- 验证 Token 引用、组合解析、Overlay 优先级和 Runtime 边界；
- 验证 Button、Checkbox、Radio、Table、Select、Input 的最小可用路径。

### 阶段 C：Figma 落地

获得明确授权后：

- 创建或重建 V2 Component Set；
- 创建/整理 `DSM/Component` Variable Collection；
- 绑定 Figma Variable；
- 将旧组件移动到 Legacy 或历史区域；
- 输出 Figma 结构证据和绑定报告。

### 阶段 D：开发落地

- 实现 Vue 与 Qt 组件 Contract；
- 实现 Runtime API；
- 接入 Token 生成产物；
- 验证运行时状态、键盘、数据、权限、提交和错误处理；
- 记录平台差异和适配器边界。

### 阶段 E：剩余组件系统化改造

当本轮架构、规范和方案验证可用后，继续按照相同路径改造剩余组件：

```text
审计旧结构
→ 归档旧源
→ 建立 Contract
→ 建立有限视觉轴
→ 建立 Recipe
→ 建立 Figma Binding Manifest
→ 建立 Runtime API 边界
→ 自动生成和校验
→ Figma/Vue/Qt 证据验收
```

不得因为组件规模变大而回退到旧的单 JSON、无限 State 轴或笛卡尔积方案。

## 14. 执行要求

执行本方案时：

1. 先读当前源文件、旧文件、规范、Registry、Recipe 和生成器；
2. 明确已知事实、合理推断、建议方案和待验证项；
3. 每次只落盘已经确认的一项；
4. 每次落盘后立即执行 JSON、Compiler、CI 和测试校验；
5. 自动输出错误和警告定位；
6. 维持历史文件可回退；
7. 不直接修改 Generated Output；
8. 不向 `G:\ComponentLab-Delivery\build` 写入未经批准内容；
9. 未获得 Figma 写入授权前，只能执行只读审计和本地 Manifest 更新；
10. 完成 Button、Checkbox、Radio、Table、Select、Input 后，再进入剩余组件的系统化改造。

最终目标不是产生更多 Token，而是建立一套可以被 Figma、Vue、Qt 和生成器共同解释、共同校验、长期维护的组件设计系统。
