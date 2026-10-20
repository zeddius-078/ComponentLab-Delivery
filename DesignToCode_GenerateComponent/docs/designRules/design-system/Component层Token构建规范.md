# Component 层 Token 构建规范

> 本文维护现行设计规则，不是运行验收报告。Token 的权威层次与物理源由 `Source/ComponentKit/design-assets/DesignSystem/source/v2-source-manifest.json` 登记；规则与实现发生冲突时记录差异，不将目标规范冒充已发布能力。旧版说明和历史审计已归档。

## 当前构建入口与分层

七层为 Primitive → Profile → Theme → Semantic → Composite → Component → Content。Profile 与 Theme 分别维护在 profile.json 和 theme.json；Component 的人工视觉源是 component-recipes，Content 只负责术语与国际化。现行分层以第 343—348 条及 Manifest 为准，不能继续把早期四层编号当成当前全架构。

维护目录是 Source/ComponentKit/design-assets/DesignSystem/source。运行 Source/PrepareBuild.py 在 build/work/Source/ComponentKit 内生成；--copy-only 只复制输入。本文后续 generated 路径均指构建副本内路径，不允许在维护 Source 下回写。完整发布入口为 Source/Publish-ComponentSDK.ps1。

历史数量、阶段审计和旧模型在 docs/archive；不作为当前 Figma 变量规模或运行成功依据。已登记的只读审计验证输入按 PrepareBuild.py 保留路径，不提供 Token 值。

## 0. V2 已确认的架构决策

### 0.1 权威来源与边界

1. Component Registry 是组件层轴、合法组合、组件槽位和 Token 生成边界的权威来源。
2. `generated/component.json` 是 Registry 约束下的下游 Token 产物，不得另行发明组件轴或合法组合。
3. Primitive、Profile、Theme、Semantic、Composite、Component、Content 与 Product Application Rollout 分别保留自己的作用域；Product Application Rollout 保存在各产品专用配置/变量组包中，与公共设计系统变量组分离。
4. 设计师和开发者通过各自的组件库消费合法组件组合；组合复杂度由 Registry、生成器和编译系统承担，不要求使用者手工维护完整笛卡尔积。
5. Registry 与稀疏 Component Token Recipe 是组件层可编辑的设计决策源；两者必须物理分离为不同文件，不得合并到同一个文件，也不得通过同文件内的多个顶层字段模拟分离。Figma 变量、Runtime Token、Vue/CSS Token 和映射文件均属于解析生成产物，不得直接作为新的设计决策源修改。
6. 系统上线流程必须是“修改 Recipe → 自动 Registry 校验 → 解析生成 → 自动检查 → 发布消费产物”。新产品或创新方案在进入公共设计系统前必须经过设计系统评审；产品专用方案优先保存在 Product Application Rollout 专用配置中。
7. Component 层目标文件布局为：

   ```text
   DesignSystem/source/component-registry.json   ← Registry
   DesignSystem/source/component-recipes/        ← Recipe
   DesignSystem/generated/component.json         ← 解析生成产物
   ```

   component.json 是构建副本中的兼容投影，不作为人工设计决策源。
8. Component Recipe 必须遵循 Registry 与 Contract 的合法矩阵；不得手工扩写完整笛卡尔积。设计侧稀疏物化与 Runtime 展开分别治理。

### 0.2.1 Component Recipe 的定位

`component-recipes/` 是组件配方源目录（Recipe Source Package），承担设计系统中台的结构化设计决策职责：

- 它是可评审、可版本管理、可被解析器读取的机器可读源数据，不是普通说明文档；
- 它不是运行时服务，也不是 Figma 变量导出文件；
- 每个组件使用一个独立 Recipe 文件，例如 `component-recipes/button.json`；
- Recipe 保存该组件实际采用的 Base 设计值、引用关系、组合级覆盖和必要的决策元数据；
- Registry 保存组件契约、轴、合法组合和 Slot 定义，Recipe 不重复声明这些契约；
- Recipe 不保存解析后的完整矩阵，完整矩阵由生成器根据 Registry 和 Recipe 计算得到；
- 产品专用配置仍属于 Product Application Rollout，不直接写入公共 Component Recipe。

Recipe 内部使用两个逻辑区域：

```text
base        ← 基础设计值与引用，进入 component/{component}/base/... 路径
overrides   ← 组合覆盖记录集合
```

`overrides` 不是固定 Token 路径，也不是一个整体 Token。它由多条独立记录组成；每条记录通过组合条件、Slot、Property 和 Value 表达一个明确覆盖。未被覆盖的属性继续使用共享 Token。

组合覆盖记录使用结构化条件，不使用拼接路径字符串：

```json
{
  "when": {
    "intent": "primary",
    "appearance": "filled",
    "state": "hover",
    "size": "medium",
    "content": "text-only"
  },
  "slot": "label",
  "property": "color",
  "value": "{semantic/color/action/primary-hover}"
}
```

每条覆盖记录必须明确声明 `slot`、`property` 和 `value`；`when` 用于限定合法组件组合，由 Registry 自动校验。

Registry–Recipe 一致性校验必须自动执行，并作为生成与发布的硬门槛。自动校验至少包括：

```text
Registry 引用与版本
→ 组件是否存在
→ 轴名称与轴值是否合法
→ 组合是否被允许
→ Slot 是否属于该组件
→ Token 属性是否符合 Slot Vocabulary
→ 引用是否可解析
→ 是否存在未声明或冲突字段
```

自动校验失败时必须阻断解析或发布，不得静默降级、猜测补全或绕过 Registry。人工评审只负责设计意图、体验合理性、公共性和准入决策，不替代机器一致性校验。

自动校验必须在三个阶段执行：

1. **本地生成前**：设计师或开发者提交 Recipe 后，在本地解析前立即反馈错误；
2. **CI 检查阶段**：提交合并或变更评审时重新执行，防止本地环境遗漏或绕过；
3. **最终发布阶段**：生成 Figma、Runtime、Qt、Vue 等消费产物前再次执行，作为发布门禁。

三个阶段使用同一套 Registry–Recipe 校验规则；任一阶段失败，都不得进入下一阶段。

### 0.2.3 Error / Warning 分级规则

校验级别以“是否能够安全、确定地解析和生成”为判定标准，而不是按问题所属类别一律阻断：

| 情况 | 级别 | 处理 |
| --- | --- | --- |
| 找不到组件 | `error` | 阻断 |
| 轴结构缺失或层级完全不符合 Registry | `error` | 阻断 |
| 必填 Slot 缺失且无法推断 | `error` | 阻断 |
| 引用无法解析 | `error` | 阻断 |
| Registry 主版本不兼容 | `error` | 阻断 |
| 轴值已废弃但存在明确别名映射 | `warning` | 可继续生成 |
| Slot 已废弃但存在明确迁移映射 | `warning` | 可继续生成 |
| Registry 小版本不一致但契约仍兼容 | `warning` | 可继续生成 |
| 描述、Owner、决策原因等元数据缺失 | `warning` | 可继续生成 |
| 存在未使用 Token | `warning` | 可继续生成 |
| 组合不推荐但存在明确合法映射 | `warning` | 可继续生成 |

`error` 表示系统无法安全理解或生成；`warning` 表示系统能够确定性解析，但存在需要治理的问题。`warning` 仍必须输出定位和修复建议，不得静默忽略。只有当组合无法解析、没有合法映射或会导致生成结果缺失时，才将组合问题升级为 `error`。

### 0.2.2 自动校验诊断报文

校验失败时必须形成错误或警告条目，并输出人可读记录和机器可读诊断记录，不得只返回笼统的“校验失败”。当前阶段只固化诊断条目的最小定位结构；稳定的错误代码目录、详细触发条件、标准说明文案和修复策略，待 Registry、Recipe、Slot 与合法组合规则稳定后再统一建立。

当前阶段每条诊断至少包含：

| 字段 | 说明 |
| --- | --- |
| `code` | 可选的临时标识；稳定错误代码目录暂不建立 |
| `severity` | `error`、`warning`、`info`；阻断性问题必须为 `error` |
| `phase` | `local`、`ci` 或 `release` |
| `sourceFile` | 产生问题的 Recipe 或 Registry 文件 |
| `sourcePath` | 文件内 JSON Pointer 或等价精确路径 |
| `component` | 组件标识，例如 `button` |
| `registryRef` / `registryVersion` | 使用的 Registry 契约与版本 |
| `axes` | 发生问题的 Intent、Appearance、State、Size、Content 值 |
| `slot` / `property` | 发生问题的 Slot 与属性 |
| `message` | 当前可获得的原始问题说明；标准错误文案暂不固化 |
| `suggestion` | 当前可确定时提供；统一修复策略暂不固化 |

示例：

```text
ERROR [临时标识：待定]
source: component-recipes/button.json#/tokens/overrides/17
component: button
registry: component-registry.json@v2
combination: intent=primary / appearance=filled / state=loading / content=text-only
slot: label
property: color
message: 当前组合无法依据已确认的 Registry 规则解析。
suggestion: 待规则稳定后补充标准修复策略。
```

诊断输出必须满足以下要求：

- 终端或 CI 日志输出可直接阅读的错误条目；
- 同时生成可供工具消费的结构化诊断文件；
- 每个 `error` 都必须包含精确来源定位，不能只有文件级提示；
- 错误组合、未知字段、未知 Slot、版本不匹配和引用无法解析必须可单独识别；
- 存在 `error` 时进程返回非零状态并阻断生成或发布；
- 同一输入在本地、CI 和发布阶段应产生一致的诊断级别、来源定位和原始问题记录。

当前阶段只要求三个阶段产生一致的诊断级别、来源定位和原始问题记录；稳定错误代码及其详细报文规范列为后续治理项，不在本轮提前建立。

### 0.3 组件轴的规范名称

Component 层按以下轴分别建模：

| 轴 | 作用 | Button 当前确认值 |
| --- | --- | --- |
| Intent | 表达操作意图 | `primary`、`secondary`、`operation`、`danger` |
| Appearance | 表达表面/材质表现 | `filled`、`outlined`、`dashed`、`filled-inner-shadow`、`text`、`link` |
| State | 表达交互状态 | `default`、`hover`、`pressed`、`focused`、`disabled`、`loading` |
| Content | 表达内容结构 | `icon-only`、`icon-left-text`、`text-icon-right`、`text-only`、`icon-left-text-icon-right` |
| Size | 表达尺寸档位 | `small`、`medium`、`large` |

其中：

- `default` 是交互状态，表示未交互的初始/常态；不是 Button 的公共 Intent。历史 `normal`、`idle` 等名称迁移为 `default` 别名。
- `dark` 不是 Intent，只表达深色/反色表面环境，应由 Profile/Theme 上下文表达，例如 `surface=dark` 或 `contrast=on-dark`。
- `operation` 是绿色的优先操作意图，不使用 `success` 替代。
- `secondary` 保留为公共 Intent，不因降级而移除。
- `icon-only` 是 Content 结构，不是独立 Size；它可以拥有独立的方形几何、宽高、圆角、内边距、图标尺寸和交互尺寸。
- `loading` 统一表示等待/处理中状态，不另设 `waiting` 状态。进入 `loading` 后，Button 的可见 Content 整体切换为 Spinner；正常状态下的 `Content` 结构仍保留并继续参与 Token 解析。

### 0.4 Button 支持矩阵

当前 Button 目标矩阵确认支持：

```text
Intent × Appearance × State × Size × Content
4      × 6           × 6     × 3    × 5 = 2160 个逻辑组合
```

四个 Intent 均支持六种 Appearance；所有合法 Intent/Appearance 组合均支持六种 State、三种 Size 和五种 Content。该数量是系统可解析的逻辑组合数量，不等于需要人工创建 2160 组独立值。

### 0.4.1 Loading 状态的 Content 覆盖

`loading` 是 State 对 Content 呈现的统一覆盖规则，而不是新增的 Content 值：

```text
正常状态：Content 结构决定图标、文本和排列
loading：视觉外观复用 pressed；可见 Content 整体替换为 Spinner
```

因此，`icon-only`、`icon-left-text`、`text-icon-right`、`text-only` 和 `icon-left-text-icon-right` 在 `loading` 状态下均呈现统一的 Spinner。Spinner 使用独立的 `spinner` Slot；原有图标和文本不再作为可见内容渲染。该规则由 Component Registry 登记并由生成器/运行时适配器执行。

在同一 Intent、Appearance、Size 和其他上下文下，`loading` 的非 Content 视觉属性全部引用 `pressed`：

```text
loading/{slot}/{visual-property}
    → pressed/{slot}/{visual-property}

loading/content
    → spinner
```

因此，`loading` 不重复建立背景、边框、前景色、阴影等独立视觉 Token；它只表达 Content 替换和 Spinner Slot 所需的结构行为。

进入 `loading` 时，只替换内部可见 Content，不改变 Button 的外层几何与交互契约：

```text
保持不变：Size、Visual Size、Interaction Size、Padding、Spacing、Border Radius
替换内容：原有图标/文本 → Spinner
```

这样可以避免按钮在等待过程中发生宽度跳变、圆角变化或交互区域变化。

`loading` 同时是交互锁定状态：Button 在 Loading 期间不接受新的点击或键盘提交，不触发新的 Hover、Pressed 等交互状态转移，并保持 Loading 的按下态视觉与 Spinner 内容，直到业务流程明确结束该状态。

Loading 的生命周期由业务状态控制，组件只提供状态接口和对应视觉表现：

```text
业务状态开始处理 → 设置 component.state = loading
业务状态完成/失败/取消 → 设置目标状态
组件 → 按当前状态渲染，不负责计时、超时判断或自动恢复
```

### 0.4.2 LineEdit 的状态模型

LineEdit 不直接套用 Button 的状态行为，但保持统一的单一 `State` 轴，不新增并列的 `Value State` 或 `Validation State` 轴：

```text
State = default | hover | focused | error | disabled | loading
```

规则如下：

- `error` 保留在统一 `State` 轴中，是一种具有独立视觉/交互契约的特殊组件状态；
- `filled` 不作为交互 State，也不作为新的公共轴；
- `loading` 是 Input 的独立 State，表示输入业务正在处理；其视觉呈现、交互锁定和 Content 行为需要按 Input 自身规则确认，不能直接套用 Button 的 Loading 规则；
- `value-present` 表示输入值存在这一运行时条件，由运行时依据 Value 是否为空派生；它只控制 `placeholder` 的显示与隐藏，不产生新的边框、底色或交互 State；
- `appearance=filled` 表示输入框表面材质，是与 `value-present` 完全不同的公共 Appearance 语义；当前仍仅作为未来扩展的结构占位，不得用它解释“输入值非空”；
- 当前 `outlined` 下，输入值非空时保留原有 outlined 边框，文本区域保持透明/无额外背景，只显示实际输入内容；不得为 `value-present` 建立独立视觉 Token 或组合覆盖。
- `outlined` 的背景色只归属于 `container` Slot；`content` Slot 保持透明，不得叠加第二层内容背景。输入值为空或非空都不改变这唯一的容器背景色。
- `outlined` 进入 `hover` 时保留 `border`，Border 使用黑色；进入 `focused` 时保留 `border`，Border 高亮为紫色强调色。`focus-ring` Slot 继续保留作为结构兼容能力，但当前 V2 设计系统在任何 State、任何 Appearance 下都不启用。
- `outlined` 处于 `default` 时，Border 使用独立的中性语义色，例如 `border/normal`；该颜色不复用 hover 的黑色或 focused 的紫色。
- `outlined` 处于 `error` 时保留 `border`，Border 使用错误语义色，例如 `border/error`；该状态不启用 `focus-ring`。
- `error` 是 Border 颜色的最高优先级状态；当 `error` 与 `focused` 或 `hover` 同时成立时，仍使用 `border/error`，不被 Focus 紫色或 Hover 黑色覆盖。
- Input 的六个交互 State 均由 Registry 独立登记视觉契约、交互契约和优先级，不再通过人工逐个补充确认；具体视觉值是否引用共享语义值，由 Recipe/解析器明确记录，不能由生成器隐式推断。
- `disabled` 与 `error` 均采用禁用视觉和交互锁定契约；其中 `error` 保留错误语义 Border 的最高优先级，不能因复用禁用视觉而改用普通 `border/disabled`。
- `clear` 仅在已配置且 `value-present` 时显示；输入值为空时不显示。`multiline` 模式明确不具备 `clear` Slot，不能通过 Input 本体清除内容；多行内容仍必须提供开发侧/业务侧外部清除 API。其他允许配置 `clear` 的模式进入 `disabled`、`error` 或 `loading` 后，已显示的 `clear` 保留位置但置为 disabled，不提供清除交互。

Input 的 `loading` 专属行为：

```text
loading
├── Visual Appearance → 复用 disabled
├── 右侧内容 → 保留清除按钮但置为 disabled；Spinner 位于清除按钮右侧
├── 密码显示按钮 → 隐藏并由 Spinner 替换
├── 搜索图标 → 隐藏并由 Spinner 替换
├── 输入值 → 保留，不清空、不替换
└── Interaction → 锁定编辑、点击和提交
```

当输入框配置了清除按钮时，Loading 不得移除该按钮；清除按钮继续占据其原有位置，但必须进入 disabled，不能响应点击或键盘操作。Spinner 放在清除按钮的右侧，成为最右侧的等待反馈。

密码输入的显示/隐藏按钮、搜索输入的装饰性图标属于可被 Loading 覆盖的 trailing 内容：进入 Loading 后隐藏，由 Spinner 占据其位置。Loading 结束后，组件恢复进入 Loading 前的按钮或图标配置；该恢复由当前业务状态和组件配置共同决定，组件不自行推断业务结果。

因此，Input 的 trailing 区域在 Loading 时具有明确的优先级：

```text
清除按钮（保留，但 disabled） → Spinner（新增等待反馈，置于最右侧）
密码显示按钮 / 搜索图标 → 被 Spinner 替换
```

Input 的 trailing 区固定拆分为以下四个 Slot：

```text
clear             → 清除输入值
password-toggle   → 显示/隐藏密码
search-icon       → 搜索输入的装饰性图标
browse-action     → 打开文件/文件夹选择器
spinner           → loading 等待反馈
```

五个 Slot 不是五个新的公共组件，也不是新的 State 轴；它们是 Input 的固定结构插槽。这里的“固定”指 Slot 的适用规则固定，不代表 Slot 始终显示。`mode` 决定 trailing Slot 是否具备显示资格，当前 State、输入值和 Loading 覆盖规则共同决定实际可见性与交互能力。`clear` 可以与 `spinner` 同时存在，但 Loading 时必须 disabled；`password-toggle`、`search-icon` 和 `browse-action` 在 Loading 时隐藏并由 `spinner` 替换。

```text
text       → 不具备 password-toggle / search-icon 的显示资格
password   → 具备 password-toggle 的显示资格
search     → 具备 search-icon 的显示资格
multiline  → 不具备 clear / password-toggle / search-icon / browse-action 的显示资格
resource-picker → 具备 browse-action 的显示资格
```

在具备显示资格的 `mode` 下，`password-toggle` 进入 `disabled`、`error` 时保留位置但置为 disabled；`search-icon` 始终是非交互装饰内容，只随当前 State 更新视觉表现；二者进入 `loading` 时均隐藏并由 `spinner` 替换。

`password-toggle` 只负责在密码掩码与明文之间切换，不建立 `visibility` 轴、Reveal State 或其他公共组件轴；显示/隐藏结果属于该 Slot 的运行时行为。

`search` 模式的搜索请求可以由开发侧 API 或键盘 Enter 触发；`search-icon` 仍然只是装饰性图标，不承担搜索触发职责。

`multiline` 模式下，键盘 Enter 只插入换行，不触发提交或搜索请求；该规则属于运行时输入行为，不新增设计侧 State 或 Token 轴。

Trailing 区的横向顺序确认为：

```text
非 loading：clear → password-toggle / search-icon
loading：    clear → spinner
```

`spinner` 始终位于 trailing 区最右侧。

Input 的 Loading 不复用 Button 的 `pressed` 视觉，也不将整个输入内容替换为 Spinner。

Input 的完整主体 Slot 集合确认为：

```text
container
content
placeholder
border
focus-ring
clear
password-toggle
search-icon
browse-action
spinner
```

其中 `container`、`content`、`placeholder`、`border`、`focus-ring` 构成输入控件主体；`clear`、`password-toggle`、`search-icon`、`browse-action`、`spinner` 构成 trailing 区域。`border` 与 `focus-ring` 只负责视觉层，不承载输入内容；所有最终渲染属性仍必须通过对应 Slot 定位。

### 0.4.3 Input / LineEdit 的统一目标身份

V2 将 `Input` 与 `LineEdit` 统一为一个输入控件 Token 家族：

- 公共组件身份是统一的 `Input` 输入控件家族，覆盖单行与多行文本输入；当前 Qt Widgets 实现中，单行模式使用 `QLineEdit`，多行纯文本模式使用 `QPlainTextEdit`。这属于平台实现差异，不拆分为新的公共 Token 家族；未来若引入富文本编辑，再单独评估 `QTextEdit`，不得反向改变当前 `multiline` 的默认实现契约；
- 不包含 Label、Helper Text、Error Message 或 Form.Item 等外层表单包装；
- 普通输入、密码输入、搜索输入等属于该输入控件内部需要继续审计的能力/类型差异；
- 普通输入、密码输入、搜索输入、多行文本输入和资源选择输入统一属于公共 `Input`，通过 Input 的 `mode` 轴区分；`mode` 的值域确认为 `text`、`password`、`search`、`multiline`、`resource-picker`；
- `searchable` 不属于 Input 类型轴，只属于公共 `select` 的受治理模式；
- Input 的 Size 统一为 `small`、`medium`、`large` 三档；三档 Size 复用公共 Size 语义和间距 Token，不为 `text`、`password`、`search`、`multiline`、`resource-picker` 建立私有尺寸值域；
- Input 的公共 Appearance 词汇确认为 `outlined`、`filled`、`dashed`；五种 `mode` 均预留这三种 Appearance 的合法组合。当前只完整定义 `outlined`；`filled` 与 `dashed` 暂作为结构占位，暂不扩展具体视觉 Token、尺寸规则或实现细节；
- `outlined` 是当前阶段唯一完整落地的 Input Appearance，覆盖 `default`、`hover`、`focused`、`error`、`disabled`、`loading` 六个 State；其中 `loading` 继续遵循 Input 已确认的 Spinner、禁用清除按钮和交互锁定规则；
- `text`、`password`、`search`、`multiline`、`resource-picker` 五种 `mode` 统一支持上述六个 State。五种 `mode` 原则上复用同一套视觉 Token 值；它们的主要差异来自结构 Slot 和内容行为，例如 `password-toggle`、`search-icon`、`browse-action` 的有无、`clear` 是否配置以及单行/多行内容布局，不得为模式差异重复建立颜色、边框或状态视觉 Token；
- 多行输入只通过 `mode=multiline` 表达，不建立 `line-count`、`multiline-state` 或其他多行专属公共轴。具体高度、可输入行数、自动扩展策略以及窗口宽度适配属于开发侧 API 与运行时布局，不进入设计侧 Token 或 Figma 变量配置；
- `clear` 是可选功能 Slot，不是 Input 的强制可见元素；`multiline` 模式明确禁止该 Slot，即使没有清除按钮仍属于同一公共 `Input` 家族。多行内容的清除只能由外部按钮或业务逻辑调用开发侧/业务侧 API 完成，不得把该 API 误建模为 Input 内部 `clear` Slot。
- Input 的 `mode` 是互斥枚举：`text`、`password`、`search`、`resource-picker` 表示单行输入，`multiline` 表示多行输入；不允许 `password + multiline`、`search + multiline`、`resource-picker + multiline` 等模式组合。
- 光标（Caret）属于运行时编辑行为，不建立独立的 Token 轴、颜色 Token 或结构 Slot。设计侧仅使用黑色竖线 `|` 表示光标位置和存在性；该竖线是 Figma 设计示意，不是最终运行时绘制资源。运行时根据 Focus、可编辑性、Disabled 和 Loading 状态控制光标显示。
- `placeholder` 是内容提示，不是独立 State：输入值为空时显示，输入值非空时隐藏；该规则统一适用于 `text`、`password`、`search`、`multiline`、`resource-picker` 五种 `mode`。`placeholder` 的显示由 `value-present` 条件控制，不得与 `appearance=filled` 混淆。
- 下拉搜索若包含选项弹层、选择模型或下拉交互，不得仅因复用了输入框外观就自动归入 Input；其组件边界需单独确认；
- 当前 Registry 中 `input` 与 `line-edit` 的双身份属于历史实现结构，V2 Token 目标不再维持两套公共输入控件 Token 家族。

`resource-picker` 是 Input 家族中的资源选择变体：

```text
Input / mode=resource-picker
├── Input surface
│   ├── content
│   ├── placeholder
│   ├── border
│   └── clear
├── browse-action
└── external file/folder dialog
```

- `resource-picker` 仍然复用 Input 的 Size、Appearance、Border、State、Padding 和 Spacing Token；
- `resource-picker` 的 `content` 仍然支持直接输入和编辑资源路径；`browse-action` 是打开外部选择器的替代操作，不将该变体实现为只读路径展示器；
- `browse-action` 是可交互的操作 Slot，负责调用开发侧文件/文件夹选择 API；它不是装饰图标，也不是 Select 的 `dropdown-indicator`；
- 外部文件/文件夹选择对话框不属于 Select 的 `popup`，其中不存在 Select 的 `option-item` 选择模型；
- 文件、文件夹或路径等资源类型由开发侧 API 参数控制，不新增设计侧 `resource-kind` Token 轴；
- `clear` 继续遵循 Input 的可选清除规则；`loading` 时 `browse-action` 隐藏并由 Spinner 替换，触发器/输入区域按 Input Loading 规则锁定；
- `resource-picker` 与 `password`、`search`、`multiline` 互斥，避免生成没有语义的模式组合。

`resource-picker` 的 trailing 区顺序确认为：

```text
正常：   clear → browse-action
loading：clear → spinner
```

`browse-action` 或 `spinner` 始终位于 trailing 区最右侧。

`resource-picker` 进入 `disabled` 或 `error` 时，`browse-action` 保留原有位置但置为 disabled；输入区域与浏览操作一起锁定，沿用 Input 的禁用视觉和交互契约。`error` 的 Border 优先级仍保持高于普通 disabled Border。

下拉搜索框采用独立的 Input-led 复合组件模型：

```text
Input-led Composite
├── Input surface / text editing
├── Popup / dropdown panel
├── Option list
└── Selection behavior
```

它拥有独立的 Registry 条目和 Recipe，不是 Input 的 `type`，也不是普通 Select 通过增加一个布尔属性即可得到的 Input 变体。公共身份保留为 `select`，`searchable` 作为 Select 的受治理模式；它可以显式复用 Input 的输入表面、尺寸、Padding 和相关 Slot，但弹层、选项、选择状态和键盘导航必须由 Select 自身契约定义。

组件治理遵循“公共组件最小化、内部结构组合化”：

- `select` 是公共 Component、Registry 条目和 Recipe 边界；
- Input surface、Popup、Option List、Clear affordance 等优先作为 Select 内部 Slot/Anatomy，不单独注册为公共组件；
- 只有当某个内部结构拥有独立公共 API、独立合法组合、独立生命周期或被多个组件以稳定契约复用时，才提升为独立公共组件；
- 内部结构的存在不等于公共组件数量增加，公共 Token 仍由所属组件 Recipe 统一治理。

### 0.4.4 Select 的选择模型

V2 将 `Select` 统一为一个公共组件家族，并通过 `selection` 轴表达选择模型：

```text
selection = single | multiple
```

- `single` 表示单选；`multiple` 表示多选；两者属于同一个公共 `select` Registry 条目和 Recipe 边界；
- 不拆分为 `SingleSelect`、`MultiSelect` 两个公共组件，也不因多选需要 `tag` 表现就新增公共组件类别；
- `popup`、`option-item`、`tag` 和选择行为属于 Select 自身的 Slot/Anatomy，具体合法组合由 Select Registry 登记；历史 Registry 中的 `option` 命名迁移为规范名称 `option-item`；
- `searchable` 是与 `selection` 独立的受治理能力维度，不与 `selection` 轴混淆，也不把 searchable Select 注册为新的公共组件：

```text
selection  = single | multiple
searchable = false | true
```

四种组合全部属于 Select 的合法组合：

```text
single   + searchable=false  → 单选下拉
single   + searchable=true   → 可搜索单选下拉
multiple + searchable=false  → 多选下拉，可呈现已选 Tag
multiple + searchable=true   → 可搜索多选下拉，可同时呈现输入内容与已选 Tag
```

其中：

- `selection` 表示一次选择一个还是多个选项；
- `searchable` 表示 Select 是否提供输入筛选能力；
- `searchable=true` 不改变 Select 的最终选择结果仍然是选项，而不是普通文本；
- `Input / mode=search` 表示搜索文本输入，`Select / searchable=true` 表示带输入筛选能力的选项选择器，两者不是同一个公共组件；
- searchable Select 的 Input-like surface、`popup`、`option-item`、Tag 和 Selection Behavior 均属于 Select 内部结构，不新增 `SearchSelect`、`MultiSelect` 等公共组件；
- 四种组合必须由 Select Registry 明确登记并自动校验，不能由生成器根据布尔值隐式推断。

Select 的统一 State 轴确认为：

```text
default | hover | focused | error | disabled | loading
```

- `open` 表示 Select 弹层已展开，是独立的布尔交互条件 `false | true`，不属于互斥 State 轴，也不是新的公共组件或独立选择模型；
- `focused` 表示触发器或 searchable Select 的内部输入获得焦点；`focused` 与 `open=true` 可以同时成立；
- `disabled` 或 `loading` 成立时强制 `open=false`，Registry 和生成器必须拒绝 `disabled/loading + open=true` 的非法组合；
- `selected` 不作为 State，当前选中值由 `selection` 模型和运行时选择数据表达；
- `loading` 表示选项或选择业务正在处理，具体视觉、交互锁定和 Popup 行为由 Select 自身 Recipe 定义，不直接套用 Input 或 Button 的 Loading 规则。
- Select 进入 `loading` 时必须关闭已打开的 `popup`，并禁止重新打开 `popup`、选择 `option-item` 或通过触发器改变选择；触发器进入锁定状态。Loading 的开始与结束由业务状态控制，组件不负责计时或自动恢复。
- Select 的 `loading` 视觉复用 `disabled`；触发器中的已选值、Tag 或当前内容保持不变；`popup` 保持关闭；Spinner 显示在触发器 trailing 区最右侧。
- Select 使用 `dropdown-indicator` Slot 表示下拉方向；该箭头不单独承担点击行为，触发器整体负责打开/关闭 `popup`。`open` 时箭头旋转为展开方向，关闭时恢复收起方向；旋转属于运行时动效，不新增 State 轴。

Select 复用 Input 的公共 Appearance 词汇：

```text
outlined | filled | dashed
```

四种 `selection × searchable` 合法组合均预留这三种 Appearance；当前阶段只完整定义 `outlined`，`filled` 与 `dashed` 仅保留 Registry/Recipe 所需的结构占位，不扩展未确认的视觉 Token、Popup 样式或选项样式契约。

Select 的 `clear` 是可选功能 Slot：有选择值且已配置 `clear` 时显示，无选择值时隐藏。`single` 可通过触发器中的 `clear` 清空当前值；`multiple` 不在触发器中提供直接清空，而在 `popup` 首行提供一次性批量清空能力。进入 `disabled`、`error` 或 `loading` 时，相关清空操作按 Select 的锁定规则处理。

Select trailing 区的顺序确认为：

```text
single 非 loading：   clear → dropdown-indicator
multiple 非 loading： dropdown-indicator
loading：             spinner
```

`dropdown-indicator` 或 `spinner` 始终位于 trailing 区最右侧。

Select 的选中内容按 `selection` 区分：

```text
single   → 选中值显示在 content Slot
multiple → 每个选中值使用 tag Slot 表示，Tag 位于 content 区域
```

`tag` 不用于 `single` 模式。

`multiple` 的选择结果支持两种逐项移除路径：

```text
Tag 移除操作       → 直接取消对应选项
重新打开 popup     → 通过 option-item 逐个取消选择
clear              → 清空全部选择
```

Tag 的逐项移除属于 Select 内部行为，不新增公共 State 轴或新的公共组件类别。

在 `multiple` 模式下，`option-item` 复用多选框组件的结构与交互契约：

```text
option-item
├── checkbox
└── label
```

- 每个 `option-item` 都拥有自己的选中条件和交互状态；
- `checkbox` 与 `label` 组成一个完整的选项行，选项行的点击和键盘行为由该内部组合共同处理；
- 每个已选 `option-item` 在 Select 的 content 区对应一个 `tag`，Tag 是选项选择结果的投影，不是第二份独立选择数据；
- 通过 Tag 移除选项时，必须同步取消对应 `option-item` 的 checkbox 选中状态；通过 `option-item` 取消选择时，必须同步移除对应 Tag；
- 该内部复用不扩展 Select 的公共 State 轴，也不把每个 checkbox 注册为新的公共组件实例。

在 `single` 模式下，`option-item` 不复用 Radio 组件；它是一个独立的可选择选项项，由 Select 的选择模型保证同一时刻最多一个 `option-item` 处于选中状态。单选的选中表现属于 `option-item` 自身的选择视觉，不新增 Radio 结构。

`single` 模式下，已选 `option-item` 可以通过内部 `selected-indicator` 表示，默认表现为右侧 `checkmark`；该标记属于 `option-item` 内部结构，不注册为独立公共组件，也不改变 Select 的公共 State 轴。

`option-item` 的内部状态体系独立于 Select 父组件：

```text
option-item State = default | hover | focused | selected | disabled
```

这些状态的视觉 Token、交互契约和优先级由 `option-item` 自身登记，不能直接复用 Select 的 `default`、`hover`、`focused`、`error`、`disabled`、`loading` 视觉 Token。父 Select 的 `open` 交互条件与 `error`、`loading` 等状态可以约束 option-item 是否可交互，但不替代 option-item 的内部状态；同名状态也不构成隐式继承。

`option-item` 可以拥有内部选择表现：

```text
selected   → 显示该选项已被选择
unselected → 显示该选项未被选择
```

这里的 `selected` 只属于 `option-item` 的内部选择条件，不扩展 Select 的公共 State 轴；Select 的最终选择结果仍由 `selection` 模型和运行时数据控制。

### 0.5 共享属性与组合覆盖

共享 Token 与组合级覆盖按属性类型分别建模，不把所有值复制到每一个组合中：

- `Size` 共享尺寸相关属性；
- `Padding` 和 `Spacing` 纳入共享 Token 范围；
- `Content`、`State`、`Appearance`、`Intent` 分别保留自己的共享 Token 与组合级覆盖边界；
- 只有确实随 Intent、Appearance、State、Content 或具体 Slot 发生变化的属性，才在组合级建立覆盖；
- Base 设计值通过显式引用被组件组合消费，不依赖隐式继承。所有最终渲染属性必须通过 `Slot` 定位；组件整体属性也使用 `slot/container` 等明确目标，不允许省略 Slot 形成无归属属性。

所有轴级共享 Token 统一采用以下路径规则：

```text
component/{component}/base/{axis}/...
```

Button 示例：

```text
component/button/base/size/{size}/slot/{slot}/{property}
component/button/base/content/{content}/slot/{slot}/{property}
component/button/base/state/{state}/slot/{slot}/{property}
component/button/base/appearance/{appearance}/slot/{slot}/{property}
component/button/base/intent/{intent}/slot/{slot}/{property}
```

其中尺寸导致的 `padding`、整体布局 `spacing` 优先归入 Size 共享范围；图标与文本之间的 `gap` 等内容结构间距归入 Content 共享范围。具体部件属性通过固定的 Slot 层定位。

Base 设计值发生重叠时，按以下顺序解析，越靠后优先级越高：

```text
base/size
→ base/intent
→ base/appearance
→ base/state
→ base/content
→ 组合覆盖集合（overrides）
```

组合覆盖集合（`overrides`）拥有最终覆盖权。所有覆盖关系由 Registry/编译器显式解析，不依赖 Figma 或运行时的隐式继承。Recipe 只保存设计决策和必要覆盖，完整组合矩阵由解析器生成。

## 1. 定位与层级总览

Token 体系采用七个逻辑层。各层目录不必同构，但必须按 Manifest 建立合法、明确、可追溯的引用关系；Content 不参与视觉值展开，运行时行为不属于 Token 层。

| 层级 | 主要解决的问题 | 组织逻辑 | 示例 |
| --- | --- | --- | --- |
| 01 Primitive | 系统拥有哪些基础设计值 | 按基础属性、色系、数值组织 | `color/blue/55` |
| 02 Profile | 不同设备配置采用哪些基础值 | 按适配参数、尺度组织 | `size/control/md` |
| 03 Theme | 明暗与表面环境如何变化 | 独立主题模式与 Alias | `theme.json` |
| 04 Semantic | 某个设计值承担什么使用意图 | 按语义角色组织 | `color/text/primary` |
| 05 Composite | 哪些复合视觉值需要复用 | 排印、渐变、阴影、模糊、动效 | `typography.json` |
| 06 Component | 某个组件的哪个部件需要什么属性 | Registry/Contract + 稀疏 Recipe + Slot | `component-recipes/button.json` |
| 07 Content | 术语与实例文本如何国际化 | 英文 Key、默认语言与回退 | `content/catalog.json` |

Component 层围绕组件自身的结构和属性建立，不机械复制基础层、适配层和语义层的目录。V2 先由 Registry 定义组件轴、合法组合和 Slot，再通过 Token 引用链解析具体值。

## 2. 路径结构

V2 Component Token 路径按以下层级组织：

```text
component/{组件}/intent/{Intent}/appearance/{Appearance}/state/{State}/content/{Content}/slot/{Slot}/{属性}
```

| 层级 | 示例 | 职责 |
| --- | --- | --- |
| Collection | `Component` | Token 所属体系 |
| Component Group | `button` | 哪个组件 |
| Intent Group | `intent/primary` | 哪种操作意图 |
| Appearance Group | `appearance/filled` | 哪种表面/材质表现 |
| State Group | `state/default` | 哪种交互状态 |
| Content Group | `content/text-only` | 哪种内容结构 |
| Slot Group | `slot/label` | 哪个组件部件 |
| Token Key | `font-size` | 控制什么属性 |

组件建模顺序：先定义 Intent，再锁定 Appearance，再定义 State，最后确定 Content；具体属性通过 Slot 定位。Size、Padding 和共享 Spacing 从共享路径引用，不作为每个组合都重复展开的轴。

Button 部件清单：

| 部件 | 允许的属性 |
| --- | --- |
| Container | `background-color`、`border-radius`、`padding-inline` |
| Label | `color`、`font-size`、`font-weight` |
| Leading Icon | `color`、`size`、`asset` |
| Focus Ring | `color`、`width`、`offset` |

上述槽位的 V2 路径示例：

```text
component
└── button
    └── intent
        └── primary
            └── appearance
                └── filled
                    └── state
                        └── default
                            └── content
                                └── text-only
                                    └── slot
                                        ├── container
                                        │   ├── background-color
                                        │   ├── border-radius
                                        │   └── padding-inline
                                        └── label
                                            ├── color
                                            ├── font-size
                                            └── font-weight
```

## 3. Component Property Vocabulary

Component 末级必须来自标准化组件属性词汇表（Component Property Vocabulary），不得直接复制 Primitive Token 的末级名称。

```text
Primitive    dimension/dimension-32
Semantic     metric/interaction-size/default
Component    input/base/default/container/min-height
```

`min-height` 与 `dimension-32` 不同名是正确的：末级表达组件属性语义，具体数值沿引用链解析。

| Component 末级 | 属性含义 | 可引用的基础类型 |
| --- | --- | --- |
| `color` | 文本、图标、边框等部件颜色 | Color |
| `background-color` | 不透明背景实色 | Color |
| `font-size` | 字号 | Dimension |
| `border-width` | 边框宽度 | Dimension |
| `padding-inline` | 水平方向内边距 | Dimension / Spacing |
| `gap` | 子元素间距 | Dimension / Spacing |
| `asset` | 视觉素材 | Asset |
| `gradient` | 渐变配置 | Gradient |
| `opacity` | 不透明度参数 | Number |

组件属性名是跨平台设计语义，不意味着 Qt QSS、CSS、QPainter 原生支持完全相同的属性，最终由平台适配器转换。

## 4. Slot / Token / Mapping 三对象分离

| 对象 | 定义 | 建立要求 |
| --- | --- | --- |
| Component Slot | 组件实际存在的样式槽位 | 定义完整 |
| Component Token | 该槽位存在需要单独管理的设计决策时创建 | 按需创建 |
| Component Mapping | 把槽位或 Token 绑定到 Figma/Pixso 属性及 Qt/Vue 实现 | 逐个登记 |

治理原则：组件槽位必须定义完整；只有当槽位确实具有独立样式决策、或需要对其单独治理时，才创建 Component Token。将每个组件的所有属性全部 Token 化会产生变量膨胀。

属性统一消费语义层时直接绑定 Semantic，不创建 Component Token：

```text
component/button/intent/primary/appearance/filled/state/default/content/text-only/slot/label/font-size
    → semantic/typography/label/md
```

## 5. 引用解析

契约建模从组件出发；数值解析从组件槽位沿引用链寻找来源。

| 方向 | 内容 | 示例 |
| --- | --- | --- |
| 契约建模 | 定义组件需要什么 | Button → Label → color |
| 数值解析 | 属性的值从哪里来 | Component Slot → Semantic Reference → Primitive Reference → 具体值 |

```text
component/button/intent/primary/appearance/filled/state/default/content/text-only/slot/label/color
        （组件槽位）
        ↓
semantic/color/text/on-action                  （语义引用）
        ↓
primitive/color/neutral/gray-0                 （基础引用）
        ↓
#FFFFFF                                        （具体值）
```

基础值发生修改时，变更沿依赖关系影响引用它的 Semantic Token 与 Component Token。契约建模与数值解析是两种不同操作，不能把整个系统描述为单向正向或反向传播。

## 6. 文本 Token 规范

### 6.1 文本内容与文本样式分离

| 类别 | 示例 | 是否 Token |
| --- | --- | --- |
| 文本内容 Content | `Apply`、`Cancel`、`Wafer Size`、`Enter value`、`Invalid value` | 否，属于组件实例内容属性 |
| 文本样式 Typography | `font-family`、`font-size`、`font-weight`、`line-height`、`letter-spacing`、`color` | 是 |

文本内容通过组件属性表达：`text`、`value`、`label`、`placeholder`、`helper-text`、`error-text`。实际文字不得做成 Token。

```text
Button
├── text = "Apply"      ← Component Property（内容）
└── label               ← Text Role（样式分组）
    ├── color           ← Token
    ├── font-size       ← Token
    ├── font-weight     ← Token
    └── line-height     ← Token
```

### 6.2 Text Role Vocabulary

分组名称使用文本角色（Text Role），Token Key 使用文本属性（Text Property）。

| 文本角色 | 分组名 | 使用场景 |
| --- | --- | --- |
| 控件标签文字 | `label` | Button、Checkbox、Input |
| 输入值 | `value` | Text Field、Select、Spin Box |
| 标题 | `title` | Card、Dialog、Panel |
| 正文 | `body` | Card、Dialog、Panel |
| 描述 | `description` | Card、List、Panel |
| 占位文本 | `placeholder` | Input、Select |
| 辅助说明 | `helper-text` | Input、Form |
| 错误说明 | `error-text` | Input、Form |
| 提示文字 | `hint` | 特定场景 |
| 状态文字 | `status-text` | Status、Progress |

核心角色：`label`、`value`、`title`、`description`、`placeholder`、`helper-text`、`error-text`。

`text` 为 Fallback Text Role：仅当组件只有一个无法进一步细分语义的文本对象时使用（如 Badge、Tag、Breadcrumb Item）。Button 优先用 `label`；Text Field 优先用 `value`、`placeholder`；Card 优先用 `title`、`description`。

### 6.3 Typography Property Vocabulary

核心：

```text
color
font-family
font-size
font-weight
line-height
letter-spacing
```

必要时增加：`text-decoration`、`text-transform`、`text-align`。工业软件组件层不建议默认全部建立。

文本角色内部使用 `color`，不使用 `text-color`；同理 `label/font-size` 不重复角色名写成 `label/label-font-size`。Group 负责分类，Token Key 负责属性。

### 6.4 组件示例

Button（V2）：

```text
component/button/intent/primary/appearance/filled/state/default/content/text-only/slot/label/color
component/button/intent/primary/appearance/filled/state/default/content/text-only/slot/label/font-family
component/button/intent/primary/appearance/filled/state/default/content/text-only/slot/label/font-size
component/button/intent/primary/appearance/filled/state/default/content/text-only/slot/label/font-weight
component/button/intent/primary/appearance/filled/state/default/content/text-only/slot/label/line-height
component/button/intent/primary/appearance/filled/state/default/content/text-only/slot/label/letter-spacing
```

状态继承：Hover 仅改变颜色时，只建立对应 `state/hover` 下的 `color` 覆盖，其余属性显式引用 `state/default`，不重复建立。

Text Field、Card 等其他组件的具体轴与合法组合必须由各自 Registry 登记；本节旧版短路径仅作迁移参考，不得直接复制为 V2 Button 结构。

Text Field：

```text
component/text-field/default/label/color
component/text-field/default/label/font-size
component/text-field/default/value/color
component/text-field/default/value/font-size
component/text-field/default/placeholder/color
component/text-field/default/placeholder/font-size
component/text-field/default/helper-text/color
component/text-field/error/error-text/color
```

Card：

```text
component/card/default/title/color
component/card/default/title/font-size
component/card/default/title/font-weight
component/card/default/title/line-height

component/card/default/description/color
component/card/default/description/font-size
component/card/default/description/line-height
```

禁止使用 `component/card/default/text/title-color` 这类把角色与属性拼接成扁平名称的形式。

### 6.5 与 Semantic Typography 的关系

Component 层不重建字体体系。组件文本 Token 的本质是给组件内部不同文本角色建立语义引用，而不是重新造一套字体 Token。

```text
Typography Foundation
        ↓
Semantic Typography
        ↓
┌───────────────┼───────────────┐
↓               ↓               ↓
Button        Text Field       Dialog
↓               ↓               ↓
label         label/value      title/body
```

引用示例：

```json
{
  "font-size": {
    "$type": "dimension",
    "$value": "{semantic/typography/label/md/font-size}"
  }
}
```

## 7. Pixso/Figma 组织方式

| 层级 | 示例 | 对应 |
| --- | --- | --- |
| Collection | `Component` | Token 所属体系 |
| Group | `button/intent/primary/appearance/filled/state/default/content/text-only/slot/label` | 路径分组 |
| Variables | `color`、`font-family`、`font-size`… | Token Key |

```text
Component
└── button
    └── intent
        └── primary
            └── appearance
                └── filled
                    └── state
                        └── default
                            └── content
                                └── text-only
                                    └── slot
                                        └── label
                                            ├── color
                                            ├── font-family
                                            ├── font-size
                                            ├── font-weight
                                            ├── line-height
                                            └── letter-spacing
```

`label` 是 Group，`color` 是 Token Key，不使用 `label-color` 扁平名。该结构对 AI Parser、D2C+ 与 Token Compiler 均易于解析。

## 8. 规范性条文

1. V2 Component Token 路径按 `component/{组件}/intent/{Intent}/appearance/{Appearance}/state/{State}/content/{Content}/slot/{Slot}/{属性}` 组织；末级属性使用统一 Property Vocabulary，不复制 Primitive 命名。
2. 组件槽位定义完整；Component Token 仅在该槽位需要单独管理设计决策时创建；绑定关系通过 Component Mapping 登记。
3. 文本样式必须以 Text Role 作为上下文分组，以 Typography Property 作为 Token Key；Text Role 优先使用 `label`、`value`、`title`、`description`、`placeholder`、`helper-text`、`error-text` 等具有明确语义的名称；`text` 仅作为无法进一步细分文本角色时的通用回退名称。
4. 文本角色内部使用 `color`，不使用 `text-color`；Token Key 不重复角色名。
5. 实际文本内容属于组件实例内容属性，不得建立 Token。
6. 状态只建立与默认态有差异的属性，其余属性沿引用继承。
7. Component 层不重建字体体系，文本 Token 通过引用消费 `semantic/typography`。
8. `Size`、`Padding` 和 `Spacing` 采用共享 Token 与组合级覆盖分离的方式建模；尺寸相关间距优先由 Size 共享，内容结构间距由 Content 共享。
9. 所有轴级共享 Token 统一放在 `component/{component}/base/{axis}/...` 下；所有最终渲染属性必须使用 `slot/{slot}/{property}` 定位，不允许创建无 Slot 归属的最终属性。
10. Base 设计值的覆盖顺序为 `base/size → base/intent → base/appearance → base/state → base/content → 组合覆盖集合（overrides）`；越靠后优先级越高。
11. `Slot` 是固定的结构定位层，不是新的语义轴；它用于区分 `container`、`label`、`icon`、`border`、`focus-ring`、`spinner` 等部件的属性目标，具体 Slot 清单由 Component Registry 登记。
12. Button 的 Intent、Appearance、State、Size、Content 合法组合由 Registry 驱动生成，不手工维护完整 2160 组 Token。
13. Registry 与 Recipe 必须是物理分离的独立文件：`component-registry.json` 只定义组件契约，`component-recipes/` 只保存设计决策值与引用；任何下游产物变更都必须回溯到对应的 Recipe 或 Registry，不得直接编辑生成结果作为上线修改。
14. `source/component.json` 迁移为 `generated/component.json` 等价的解析产物，不得继续承担人工维护的 Recipe 职责。
15. Registry–Recipe 一致性校验必须自动执行，并作为本地生成前、CI 检查和最终发布三个阶段的硬门槛；校验失败时必须阻断，不得依赖人工理解或人工放行。
16. 三个阶段必须使用同一套校验规则，禁止本地、CI 和发布流程各自维护不一致的校验逻辑。
17. 新产品或创新方案进入公共设计系统前必须经过人工评审；人工评审负责设计意图、体验合理性、公共性和准入决策，不替代自动一致性校验。
18. 当前阶段自动校验必须输出结构化、可定位、可追溯的错误或警告条目；每个条目必须包含级别、来源文件、精确路径、组件、轴值、Slot、属性和当前可获得的问题记录。稳定错误代码目录、标准错误说明和修复策略暂缓建立。
19. `error` 表示无法安全、确定地解析或生成，生成器必须返回非零状态并阻断生成或发布；`warning` 表示可确定性生成但存在治理问题，不阻断流程，但不得被静默忽略。
20. Error / Warning 分级必须由同一套自动规则决定，不得依赖不同阶段的人工判断。
21. 错误代码目录及详细错误报文规范属于后续治理项；在组件轴、合法组合、Slot 和引用规则稳定前，不得将临时错误代码或示例文案视为最终契约。
22. Button 不新增 `waiting` 状态；`loading` 统一表示等待/处理中，并对所有 Content 结构执行整体 Spinner 覆盖。
23. `loading` 状态下，原有图标和文本不作为可见内容渲染，统一使用 `spinner` Slot；该状态行为由 Registry 登记，不能仅通过颜色 Token 推断。
24. `loading` 的非 Content 视觉属性必须复用同一组合下的 `pressed` 状态；不得为 Loading 重复建立背景、边框、前景色或阴影等视觉 Token。
25. `loading` 只替换内部可见 Content；Button 的 Size、Visual Size、Interaction Size、Padding、Spacing 和 Border Radius 必须保持不变。
26. `loading` 是交互锁定状态；期间不得接受新的点击或键盘提交，不得触发新的 Hover、Pressed 等交互状态转移，直到业务流程明确结束 Loading。
27. `loading` 的开始与结束由业务状态显式控制；组件只提供状态接口和视觉表现，不负责计时、超时判断、业务结果判断或自动恢复。
28. Input / LineEdit 只保留一个统一 `State` 轴：`default`、`hover`、`focused`、`error`、`disabled`、`loading`；不得新增并列的 `Value State` 或 `Validation State` 公共轴。
29. LineEdit 的 `error` 是统一 State 轴中的特殊组件状态；`value-present` 由输入值派生，不作为公共交互轴或独立 Value State，也不得改变当前 `outlined` 的边框和背景。
30. 普通输入、密码输入、搜索输入和多行文本输入属于同一个公共 `Input`，通过 Input 的 `mode` 轴区分；`searchable` 属于 `select` 模式，不属于 Input 类型轴。
31. Input 使用 `mode` 类型轴，值域为 `text`、`password`、`search`、`multiline`、`resource-picker`；`searchable` 属于 `select` 模式，不得加入 Input 的 `mode` 值域，也不得直接借用 Button 的 Intent 命名。
32. Input 的公共 Appearance 词汇为 `outlined`、`filled`、`dashed`；`text`、`password`、`search`、`multiline`、`resource-picker` 五种 `mode` 均预留这三种 Appearance。当前阶段只完整落地 `outlined`，`filled` 与 `dashed` 仅保留 Registry/Recipe 所需的结构占位，不得将占位符误认为已完成的视觉 Token 或实现契约。
33. Input 的 `loading` 视觉属性复用 `disabled`；Spinner 出现在输入框最右侧，输入值保持不变，编辑、点击和提交交互必须锁定。
34. Input 的 `loading` 不得清除或替换清除按钮；清除按钮保留但必须为 disabled，Spinner 位于其右侧并成为最右侧反馈。密码显示按钮和搜索图标在 Loading 时由 Spinner 替换，Loading 结束后恢复原有 trailing 内容。
35. Input 的 trailing 区固定包含 `clear`、`password-toggle`、`search-icon`、`spinner` 四个 Slot；Slot 不是公共组件或新的 State 轴，其可见性和交互能力由 Input 类型/模式及 `loading` 状态决定。
36. Input 的 Size 统一为 `small`、`medium`、`large` 三档，复用公共 Size 语义和间距 Token；不得按 Input `mode` 拆分私有尺寸值域。
37. 当前阶段只有 `outlined` 完整覆盖 `default`、`hover`、`focused`、`error`、`disabled`、`loading` 六个 State；`filled` 与 `dashed` 的 State 组合仍是占位，不得生成未定义的视觉 Token。
38. `text`、`password`、`search`、`multiline`、`resource-picker` 五种 Input `mode` 统一支持六个 State，并原则上复用同一套视觉 Token；模式差异只能通过结构 Slot、内容行为和对应功能接口表达，不得复制一套模式专属的状态视觉 Token。
39. Input 的主体 Slot 固定为 `container`、`content`、`placeholder`、`border`、`focus-ring`，并包含 trailing Slot `clear`、`password-toggle`、`search-icon`、`spinner`；`border` 与 `focus-ring` 只负责视觉层，不承载输入内容。
40. 多行文本输入继续属于公共 `Input` 家族，通过 `mode=multiline` 表达结构差异；当前 Qt Widgets 实现固定使用 `QPlainTextEdit`，不得仅因底层编辑器类型不同而新增独立的公共 Token 家族或公共组件类别。`QTextEdit` 仅作为未来富文本编辑需求的候选实现，不属于当前 `multiline` 默认实现。
41. 多行输入不得新增 `line-count`、`multiline-state` 或等价公共轴；具体高度、可输入行数、自动扩展和窗口宽度适配由开发侧 API 与运行时布局控制，不得要求设计侧配置这些参数。
42. `clear` 是可选功能 Slot；没有清除按钮的 Input 仍使用同一公共 Token 家族，不得因此新增组件类别或私有视觉 Token。
43. Input 的 `mode` 必须是互斥枚举；`text`、`password`、`search`、`resource-picker` 为单行模式，`multiline` 为多行模式，Registry 必须自动阻断 `password + multiline`、`search + multiline`、`resource-picker + multiline` 等非法组合。
44. 光标由运行时控制；设计侧仅使用黑色竖线 `|` 作为示意，不为光标建立 Token、Slot 或独立 State。运行时必须依据 Focus、可编辑性、Disabled 和 Loading 状态决定光标是否显示。
45. `placeholder` 不是独立 State；输入值为空时显示，输入值非空时隐藏，五种 Input `mode` 统一遵循该规则。
46. 必须区分 `value-present` 与 `appearance=filled`：前者是只控制 Placeholder 呈现的运行时条件，后者是表面材质 Appearance。当前 `outlined` 下，`value-present` 不得产生额外边框、底色或独立视觉 Token，文本区域保持透明/无额外背景。
47. `outlined` 的唯一背景色必须由 `container` Slot 承担；`content` Slot 必须透明，输入值变化不得产生第二层背景或内容背景 Token。
48. `outlined` 的 `focused` State 必须保留 `border` 并使用紫色强调色高亮；`focus-ring` Slot 仅作为结构兼容能力保留，当前 V2 在任何 State、任何 Appearance 下都不得启用或生成 Focus Ring 视觉 Token。
49. `outlined` 的 `hover` State 必须保留 `border` 并使用黑色；`focused` 使用紫色强调色。当前设计系统不启用 `focus-ring`。
50. `outlined` 的 `default` State 必须使用独立的中性语义 Border 色，例如 `border/normal`，不得复用 hover 或 focused 的状态色。
51. `focus-ring` 在当前 V2 设计系统中对 Button、Input、Select 触发器等公共控制表面默认禁用，不得由普通 State、Appearance、Profile 或产品配置重新启用；Select Popup 内的所有 `option-item` 在键盘导航或键盘激活模态下统一启用同一种局部键盘焦点环，作为可达性提示的明确例外。Focused 视觉只增加一个焦点环边框，不叠加其他颜色、背景或 `focused + disabled` 组合样式；鼠标悬停和鼠标点击不产生该焦点环。
52. `outlined` 的 `error` State 必须保留 `border` 并使用错误语义色，例如 `border/error`；该状态不得启用 `focus-ring`。
53. `error` 具有最高 Border 颜色优先级；`error + focused`、`error + hover` 等叠加情形必须使用 `border/error`，不得被其他 State 颜色覆盖。
54. Input 的六个交互 State 必须由 Registry 独立登记视觉契约、交互契约和优先级；不得要求通过逐状态人工确认才能生成，也不得由生成器隐式推断状态关系。
55. `disabled` 与 `error` 均锁定编辑、点击和提交交互，并采用禁用视觉契约；`error` 的 `border/error` 仍具有最高 Border 优先级，不得被普通 `border/disabled` 替换。
56. `clear` 仅在已配置、当前 `mode` 允许该 Slot 且输入值非空时显示；空值不显示。`multiline` 明确禁止 Input 内部 `clear`，但必须保留外部程序侧清除 API；其他允许配置 `clear` 的模式在 `disabled`、`error`、`loading` 时保留已显示的 Slot 但必须 disabled。
57. Input 功能 Slot 的适用规则由 `mode` 固定登记，但实际显示内容必须随当前 State、输入值和 Loading 状态动态解析；`password-toggle` 仅属于 `password`，`search-icon` 仅属于 `search`，不得在其他 `mode` 中自由混用。
58. `password-toggle` 在所属 `mode` 下进入 `disabled`、`error` 时保留但必须 disabled；`search-icon` 始终为非交互装饰 Slot，只随当前 State 更新视觉表现；二者进入 `loading` 时隐藏并由 `spinner` 替换。
59. Input trailing 区的 Slot 顺序为 `clear → password-toggle/search-icon`；Loading 时为 `clear → spinner`，且 `spinner` 必须位于最右侧。
60. `search-icon` 是纯装饰性图标，不得触发搜索、提交或任何点击行为；键盘 Enter 与搜索请求接口也不得由该 Slot 承担。
61. `password-toggle` 只负责密码的显示/隐藏切换，不得新增 `visibility`、Reveal State 或其他公共轴；切换结果由该 Slot 的运行时行为控制。
62. `search` 模式的搜索请求可由开发侧 API 或键盘 Enter 触发，但不得由 `search-icon` Slot 触发。
63. `multiline` 模式下，Enter 只插入换行，不触发提交或搜索请求；该行为由运行时控制，不新增设计侧 State 或 Token 轴。
64. Select 统一支持 `selection=single|multiple` 两种选择模型，保持一个公共 `select` 组件、Registry 条目和 Recipe 边界；不得拆分为 `SingleSelect`、`MultiSelect` 两个公共组件。
65. Select 使用独立的 `selection=single|multiple` 与 `searchable=false|true` 两个维度，四种组合全部合法；`searchable=true` 只增加选项筛选输入能力，不改变 Select 的选择结果语义，也不得生成新的公共组件类别。
66. Select 的互斥 State 轴为 `default`、`hover`、`focused`、`error`、`disabled`、`loading`；`open` 是独立布尔交互条件，可与 `focused` 同时成立，但 `disabled` 或 `loading` 必须强制 `open=false`；`selected` 不得作为 State 轴，`loading` 必须由 Select 自身 Recipe 定义。
67. Select 复用 `outlined`、`filled`、`dashed` 三种公共 Appearance；四种 `selection × searchable` 组合均预留这三种 Appearance，当前只完整落地 `outlined`，不得生成未定义的 `filled` 或 `dashed` 视觉 Token。
68. Select 的弹层使用内部 `popup` Slot，单个选项使用 `option-item` Slot；二者不注册为独立公共组件。历史 `option` Slot 命名必须迁移为 `option-item`。
69. Select 进入 `loading` 时必须关闭已打开的 `popup`，禁止重新打开、选择 `option-item` 或通过触发器改变选择；触发器必须锁定，Loading 生命周期由业务状态控制。
70. Select 的 `loading` 视觉复用 `disabled`；已选值、Tag 或当前触发器内容保持不变；`popup` 关闭；Spinner 位于触发器 trailing 区最右侧。
71. Select 使用 `dropdown-indicator` Slot 表示下拉箭头；点击由触发器整体处理，箭头仅随 `open` 状态执行展开/收起旋转，不新增独立 State 轴。
72. Input 新增互斥的 `mode=resource-picker`；它复用 Input 的视觉体系，通过 `browse-action` Slot 调用开发侧文件/文件夹选择 API，不属于 Select，也不使用 `popup` 或 `option-item`。
73. `resource-picker` 的 `browse-action` 是 Input 内部的可交互操作 Slot；外部文件/文件夹对话框由开发侧 API 和平台运行时负责，不得建模为 Select 的 `popup` 或 `option-item`。
74. `resource-picker` 的 trailing 顺序为正常状态 `clear → browse-action`、Loading 状态 `clear → spinner`；`browse-action` 或 `spinner` 必须位于最右侧。
75. `resource-picker` 进入 `disabled` 或 `error` 时，`browse-action` 必须保留位置但置为 disabled；输入区域与浏览操作共同锁定，`error` Border 优先级高于普通 disabled Border。
76. `resource-picker` 的 `content` 支持直接输入和编辑资源路径；`browse-action` 提供外部资源选择入口，二者是同一 Input 变体的两种输入方式。
77. Select 的 `clear` 是可选功能 Slot；有选择值且已配置时显示，无选择值时隐藏。`single` 可通过 `clear` 清空当前值；`multiple` 的批量清空不通过触发器右侧的 `clear` 执行，而只能在 `popup` 内执行；`clear` 与 `popup` 的开关状态相互独立：`popup` 未打开时点击只清空当前值，不打开浮层并恢复无选择的初始状态；`popup` 已打开时点击只清空当前值，保持浮层打开，不关闭、不重置浮层内容或搜索状态。`disabled`、`error`、`loading` 时相关清空操作按 Select 的锁定规则处理。
78. Select trailing 区的 Slot 顺序按选择模型区分：`single` 非 Loading 时为 `clear → dropdown-indicator`；`multiple` 不在触发器 trailing 区提供直接清空的 `clear`，仅显示 `dropdown-indicator`，批量清空放在 `popup` 首行；Loading 时 `dropdown-indicator` 由 `spinner` 替换，`spinner` 必须位于最右侧。
79. Select 的 `single` 使用 `content` 显示选中值；`multiple` 使用 `tag` 表示每个选中值，Tag 位于 content 区域；`tag` 不得用于单选模式。
80. Select 的 `multiple` 支持在 `popup` 内通过 `option-item` 逐个取消正常选项，或通过浮层首行的批量清空操作一次性清除全部选择；异常的禁用 Tag 与对应 `option-item` 不支持单独移除，不新增公共 State 轴或公共组件类别。
81. `multiple` 模式的 `option-item` 复用 `checkbox + label` 结构；每个已选 `option-item` 与 content 区的一个 `tag` 一一对应，Tag 与 checkbox 的选中状态必须双向同步。
82. `single` 模式的 `option-item` 不复用 Radio；它是可选择的单项，由 Select 保证同一时刻最多一个选项被选中，不新增 Radio 结构或公共组件类别。
83. `single` 模式的已选 `option-item` 使用内部 `selected-indicator` 表示，默认可呈现为右侧 `checkmark`；该标记不注册为独立公共组件或新的 Select State 轴。
84. `option-item` 必须拥有独立的 `default`、`hover`、`focused`、`selected`、`disabled` 状态契约；不得继承 Select 父组件的 State Token。父组件状态只能约束其可交互性，不能替代其内部视觉状态。
85. `option-item` 的 `selected` 只属于选项内部选择表现，不属于 Select 公共 State 轴；Select 的选择结果由 `selection` 模型和运行时数据控制。
86. `disabled` 的 `option-item` 必须继续显示在 `popup` 中，使用自身的 `disabled` 视觉 Token；它可以被方向键导航聚焦，但禁止鼠标点击或键盘激活产生选中、取消选中等选择变更。用户通过鼠标点击或按下 `Enter` / `Space` 尝试操作该项时，可通过业务侧事件接口触发 `warning` 或 `info` 反馈，也可以将回调配置为空或 `void`，此时无可见反馈；无论事件是否有反馈，都不得改变选择结果。事件接口可以携带选项唯一标识、禁用原因和当前 Select 上下文。
87. 已选中的 `option-item` 被业务侧标记为 `disabled` 时，必须区分选择模型处理；由该异常选中值引起的 Select `error` 不得等同于整个组件 `disabled`，`popup` 仍可打开并允许用户执行排错操作。若同时存在组件级 `disabled`，则组件级 `disabled` 优先：Select 使用整体禁用视觉，禁止打开 `popup`、选择、清除和其他排错操作；异常值保留。组件解除整体 `disabled` 后，若异常选中值仍存在，Select 必须自动恢复 `error` 视觉并重新开放 `popup` 与排错操作：
    - `single`：不得自动清除当前值；Select 父组件进入 `error` 状态并使用错误色；原先选中的 `option-item` 使用自身的 `disabled` 视觉且不可再次选中；其他未禁用的 `option-item` 仍可选择；`clear` 保持可用，用户可通过清空 Select 排除该异常值。用户选择其他正常选项后，Select 自动清除 `error` 状态并恢复正常视觉；用户点击 `clear` 清空异常值后，也必须立即清除 `error` 并恢复正常视觉，同时保持当前 `popup` 开关状态不变。只有整个 Select 被业务整体标记为 `disabled` 时，才锁定 Select 的全部交互。
    - `multiple`：不得自动清除异常值；只要仍存在任意一个已选中且被禁用的异常项，Select 父组件就保持 `error` 状态并使用错误色；对应 Tag 保留但使用禁用色；对应 `option-item` 使用自身的 `disabled` 视觉；Checkbox 保持“已选中 + disabled”状态；该项及其 Tag 均不可单独移除。其他正常 Tag 和未禁用的 `option-item` 仍可正常操作；用户只能在 `popup` 内执行一次性清空，直到所有异常选中项被清除后，Select 才自动清除 `error` 状态并恢复正常视觉。
88. `multiple` Select 的 `popup` 首行提供一个三态批量选择控件，不拆分为两个独立操作；首行必须固定在浮层顶部，并通过分隔线与下方的 `option-item` 子项区域隔开。首行控件复用 Checkbox 的三态视觉与交互基础，但首行本身不是 `option-item`，不得复用 `option-item` 的布局、状态或 Token。三态为 `checked`（当前结果集中的全部可用项已选）、`indeterminate`（当前结果集中部分可用项已选）和 `unchecked`（当前结果集中没有可用项被选）。`searchable=true` 时，三态计算和批量操作作用于当前搜索条件产生的全部结果集，不作用于未匹配的隐藏选项；无搜索条件时，结果集为当前加载的全部选项。通常情况下，`disabled` 项不参与三态计算；已选中后被业务标记为 `disabled` 的异常项仍必须能通过批量清空动作清除。点击规则为：`unchecked` 或 `indeterminate` → 选中当前结果集中的全部可用项；`checked` → 清除当前结果集中的全部选择，包括已选中后被业务标记为 `disabled` 的异常项。若当前结果集中的全部选项均已被禁用但仍保留异常选中值，控件保持 `checked` 且允许点击清除；清除后由于没有可重新激活的选项，控件转为 `disabled`。搜索结果为空时，首行仍保留三态控件但置为 `disabled`，不得产生选择变化。批量清空完成后，若结果集中仍存在可用选项，首行回到 `unchecked`；若没有可用选项，首行转为 `disabled`；`popup` 均必须保持打开，以便用户继续选择新的选项。每次打开 `popup` 时，必须重新校验当前结果集的选项可用性与选择结果：如果上次 `checked` 后有新的可用选项出现，不能自动选中新选项，首行必须重新计算为 `indeterminate`；如果当前已选结果包含后来被禁用的选项，则按异常选中值规则进入 `error`。`popup` 打开期间默认不实时重算三态或切换 `option-item` 的启用状态；如果业务启用了动态可用性能力，则在父层关闭或提交时执行一次最终的后端/业务校验。若最终校验发现当前选择值实际不可选，Select 必须保留当前异常选中值、关闭 `popup` 并进入 `error`；用户下次重新打开浮层后继续排错，不得静默接受或自动移除该选择。批量操作不得改变 `option-item` 自身的 `disabled` 视觉和交互契约。
89. Select Popup 的批量选择首行与下方的 `option-item` 属于不同结构：首行负责结果集级别的批量选择，`option-item` 负责单个选项选择；二者必须保持独立的 Slot/Anatomy 和 Token 解析边界，不能因为复用 Checkbox 就合并为同一种选项结构。首行文字固定显示 `Select all`，不随状态切换为 `Unselect all`；全选、半选和未选中只通过内部 Checkbox 的 `checked`、`indeterminate`、`unchecked` 表达。首行可复用 Checkbox 的 `default`、`hover`、`focused`、`disabled` 等交互状态基础，但三态选择值是首行自身的状态，不得复用 `option-item.selected`。所有 `option-item` 的键盘聚焦都使用同一种局部 Focus Ring，Focused 视觉只表现为一个焦点环边框；`disabled option-item` 也可被键盘聚焦，但不得被选中、取消选中或改变选择结果。批量选择首行作为复合控件不支持整行 Hover 或整行 Focus Ring，只有内部 Checkbox 作为键盘交互焦点目标，并使用 Checkbox 自身的 Focus Ring；首行文字区域点击时代理到内部 Checkbox，执行同样的三态批量切换，但文字区域不成为独立焦点目标。
90. Select 在父层关闭或提交时的最终后端/业务校验失败后，除保留异常选中值、关闭 `popup` 并显示 `error` 外，必须保留独立的业务 `error` 事件接口；该事件用于上层记录、提示或后续业务处理，错误文案和恢复流程由业务层决定，组件不得静默吞掉校验失败或替代业务层决定处理结果。
91. 可用 `option-item` 的鼠标与键盘激活行为按 Select 的选择模型区分：`single` 完成选择后关闭 `popup`；`multiple` 切换该项的选择状态并保持 `popup` 打开。`disabled` 的 `option-item` 在两种选择模型下均走独立的禁用项事件接口，不进入正常选择或关闭流程；无论事件由鼠标点击还是键盘 `Enter` / `Space` 触发，也无论事件配置为 `warning`、`info` 还是 `void`，`popup` 都必须保持打开。
92. `multiple + searchable` 模式下，已选 `tag` 必须继续保留在 Select 的 `content` 区；搜索文本只用于过滤 `popup` 内的 `option-item`，不得清除、替换或隐式修改已有选择。用户选中或取消一个 `option-item` 后，`popup` 保持打开且临时搜索文本保持不变，以便继续处理同一搜索结果集中的其他选项；用户主动关闭 `popup` 时，清除临时搜索文本但保留全部已选 `tag`。再次打开 `searchable` Select 时，搜索文本默认为空，展示当前结果集的完整选项列表。`single + searchable` 模式下，当前选中值继续显示在 `content` 区，搜索文本仅作为临时筛选条件；用户选中新项后替换原值并关闭 `popup`，关闭浮层时清除临时搜索文本，`content` 恢复显示新选项的正式文本。
93. `searchable Select` 进入 `loading` 时，必须关闭已打开的 `popup` 并清除临时搜索文本；当前已选值或 `tag` 保持不变，触发器进入 Select 的 Loading/锁定视觉。搜索输入触发远程加载时，同样沿用该统一 Loading 规则。Loading 期间不得重新打开 `popup`、输入搜索文本或改变选择结果。Loading 结束后，`popup` 继续保持关闭，搜索文本保持为空，用户必须再次主动打开浮层；已选值或 `tag` 保持不变。
94. 所有关闭 Select `popup` 的路径必须采用同一套关闭契约，包括用户通过 `Esc`、点击外部区域、完成单选、父层关闭以及运行时程序化关闭：清除临时搜索文本，保留当前已选值或 `tag`，并执行最终的业务/后端选择校验；若校验失败，必须保留异常选中值并进入 `error`，不得静默移除或接受该选择。不同关闭入口不得产生不同的交互状态、选择结果或校验响应。
95. Select `popup` 关闭后不得自动将键盘焦点迁移回 Select 触发器或输入区域；关闭动作本身不产生隐式焦点恢复。只有用户之后再次通过界面或键盘明确触发 Select 的对应操作时，运行时才将焦点放回相应的触发器或输入区域。
96. Select 没有任何选择值时显示配置的默认 `placeholder`；通过 `clear` 或 `multiple` 的浮层批量清空完成清除后，同样恢复显示 `placeholder`。有选择值时，`placeholder` 隐藏并由单选 `content` 或多选 `tag` 表示当前选择结果。`multiple + searchable` 没有已选 Tag 时，`tag-viewport` 保持为空，不占用无意义的固定空间；选择优先模式显示 `placeholder`，输入优先模式下 `search-input-zone` 可自动扩展并占据可用的 content 区域，随后由光标和临时搜索文本替代 placeholder。
97. `searchable Select` 支持两种运行时进入方式，但不新增设计侧轴：选择优先时按普通 Select 处理，显示 `placeholder` 或当前选中值；输入优先时，用户明确点击 `content` 的输入区域后，content 进入可编辑状态并显示光标，同时打开 `popup` 并展示当前完整选项结果，随后接受搜索文本输入。`single + searchable` 已有选中值时，进入输入模式保留当前选中值作为可编辑搜索文本，用户可以继续输入、覆盖或修改；搜索输入能力由运行时交互模式控制，不得因为支持 searchable 就让 Select 在所有时刻默认呈现为输入框。
98. `multiple + searchable` 的 `content` 必须拆分为三个内部区域，不新增公共 Select 轴：`tag-viewport`（已选 Tag 的固定显示区域）、`search-input-zone`（位于 Tag 区之后，用于进入搜索、显示光标和输入临时搜索文本的专用区域）以及 `dropdown-indicator`（下拉方向与浮层入口区域）。多选触发器不提供统一的 trailing `clear`；Tag 不得无限挤压搜索输入区域或下拉指示器，Tag 显示策略由 `tag-viewport` 的运行时/组件配置接口控制。
99. `tag-viewport` 必须保留 Tag 显示策略接口，至少支持 `single-row` 与 `multi-row` 两种模式，不新增公共 Select 设计轴：`single-row` 将 Tag 保持单行排列，并在右侧边缘使用从不透明到透明的渐消遮罩；`multi-row` 允许 Tag 换行排列，触发器可随 Tag 行数增长，达到运行时配置的最大高度或最大行数后固定高度，并于最后可见 Tag 之后显示省略号。该省略号仅是非交互溢出标记，不承担展开、清除或打开浮层操作。级联选择等必须保持单行的场景使用 `single-row`；可用高度、最大行数和具体截断位置由运行时/组件配置控制，不在设计侧 Token 中固定。`multi-row` 中，当 Tag 已占满当前行时，`search-input-zone` 必须换到下一行，不得压缩或挤压最后一个 Tag。`single-row` 无论 Tag 数量多少，都必须为 `search-input-zone` 保留最小可点击宽度；Tag 只能在 `tag-viewport` 内渐消，不得覆盖或挤掉搜索触发区域。`dropdown-indicator` 始终位于触发器最右侧的独立区域，不参与 Tag 换行，也不得移动到下一行。
100. 被 `single-row` 的渐消遮罩部分遮挡，或被 `multi-row` 的溢出省略隐藏的 Tag，不提供直接删除入口；用户必须重新打开 `popup`，通过对应的 `option-item` 取消选择。可见且未被禁用的 Tag 支持逐项删除：删除操作只移除当前 Tag，保留其他已选 Tag，并保持 `popup` 当前开关状态不变，不触发额外的浮层开关或整体清空。
101. `searchable` 的搜索文本与筛选结果属于 `popup` 子层的局部状态：输入或修改搜索文本只改变 `popup` 内显示的 `option-item` 结果，不得单独改变 Select 父组件的当前值、Tag、父层视觉或 `popup` 开关状态。`multiple + searchable` 中删除可见 Tag 时，也必须保持当前搜索文本和搜索结果不变；只有用户明确选择/取消 `option-item` 或删除 Tag，才允许改变 Select 的选择结果。搜索结果为空时，只在 `popup` 内显示空结果状态，不触发 Select 父组件的 `error`，也不改变已有选择。
102. 搜索结果为空时，`popup` 必须显示独立的空状态结构，并包含小型插画；空状态仅用于反馈当前无匹配结果，不属于 `option-item`，不可点击、不可键盘激活、不可选择或取消选择，也不得触发选项事件。空状态只替代选项区域，`search-input-zone` 仍保持可编辑，用户可以继续修改搜索文本以重新获取或筛选结果。Select 初始就没有任何选项时，与搜索后无匹配结果复用同一种空状态结构，仅允许由业务文案或上下文说明区分，不新增结构或 Token 轴。
103. `searchable Select` 的远程搜索请求失败时，必须进入 Select 的 `error` 状态，保留当前已选值或 `tag`，并触发独立的业务 `error` 事件；该情况不等同于“搜索结果为空”，不得使用普通 Empty State 冒充请求失败。远程请求失败后不自动重试，也不保留失败的临时搜索文本；用户必须重新打开 `popup` 并重新输入搜索文本，才发起新的远程请求。
104. 远程搜索失败后的 `error` 属于最近一次请求的失败结果。用户重新输入并发起新的远程搜索时，Select 进入 `loading`，Loading 视觉暂时覆盖该请求错误；请求成功后清除本次远程请求错误并恢复正常交互，请求再次失败则重新进入 `error`。该恢复规则不改变异常选中值导致的 Select `error` 处理规则；两类错误必须由运行时按错误来源分别维护，不能静默互相覆盖。
105. Select 的多个错误来源必须分开记录，但共享同一个 `error` 视觉状态；错误来源优先级为：组件级 `disabled` > 异常选中值 `error` > 远程请求 `error` > 正常状态。远程请求成功只能清除远程请求错误，不能清除仍然存在的异常选中值错误；只有用户修复或清除异常选中值后，Select 才能恢复正常。组件级 `disabled` 生效期间覆盖所有错误视觉和交互，解除后按仍存在的错误来源重新计算状态。
106. 远程请求失败、异常选中值、父层业务校验失败等错误来源，不得分别建立新的 Select State 或 Component Token 路径；它们统一映射到 `select/state/error` 的视觉与交互契约。具体错误来源、错误码、请求上下文和业务 Payload 由运行时状态与事件接口保存，不进入设计侧 Token 轴。
107. Select 的本地选项、远程选项和混合数据源均属于开发侧运行时数据获取配置，不得建立 `data-source` 设计轴或新的 Component Token 家族。无论数据来源如何变化，均复用同一套 `selection`、`searchable`、`state`、`appearance`、`option-item`、`empty-state` 和 `loading` 契约。
108. Select 的 `selection=single|multiple` 与 `searchable=false|true` 是 Registry 中的能力和合法组合维度，但不得直接扩展为公共 Component Token 路径层级。四种组合通过 Slot/Anatomy 表达结构差异；只有某个合法组合确实产生独立视觉值时，Recipe 才能登记稀疏组合覆盖。不得机械生成 `selection/searchable` 的完整笛卡尔积 Token。
109. Select 的 Component Token 路径采用分层建模：共享尺寸、Padding、Spacing 和 Radius 进入 `component/select/base/...`；外观与父组件状态进入 `component/select/appearance/{appearance}/state/{state}/slot/{slot}/{property}`；内部 `option-item`、`tag`、`popup-bulk-control` 和 `empty-state` 等结构拥有各自明确的 Slot 状态或属性路径。`selection` 与 `searchable` 不进入主 Token 路径，只作为 Recipe 的 `when` 条件参与必要的稀疏组合覆盖。
110. Figma 的 `Component` 变量组必须与 Select 的 Component Token 逻辑路径保持一致：使用 `select/base`、`select/appearance/{appearance}/state/{state}` 和 `select/slot/{slot}` 等结构；`selection`、`searchable` 与 `data-source` 不建立 Figma 变量组。`filled` 与 `dashed` 可以保留空结构占位，但不得填入尚未确认的视觉变量；Figma 变量只保存设计值，合法组合和结构关系仍由 Registry/Recipe 管理。
111. Figma Select Component Set 的变体属性由 Registry 轴映射：`Selection`、`Searchable`、`Size`、`Appearance`、`State`、`Open`。`State=focused` 与 `Open=true` 必须能够并存；`data-source`、`query`、`selected-values`、`option-availability`、Tag 行数和错误来源不得成为公共 Figma 变体轴；Slot 的可见性和交互能力由合法组合、状态与运行时规则派生。
112. Figma Component Set 是 Registry、Recipe 和 Component Token 编译后的设计消费产物，不是新的权威设计决策源。Figma 中的变体集合、变量绑定和组件结构必须由合法组合生成；手工修改不得直接成为系统上线依据，设计决策必须回写到 Registry、Recipe 或对应的 Token 源后再重新生成并校验。
113. Select 在 Figma 中只作为公共发布组件；`popup`、`option-item`、`tag`、`popup-bulk-control`、`empty-state` 等内部结构可以使用嵌套或 Local Component 维护，但不得作为独立公共组件库入口。内部结构仍必须保留独立的 Slot、状态和 Token 映射；只有未来具备独立公共 API、生命周期或跨组件稳定复用契约时，才允许提升为公共组件。
114. Select 的每个最终视觉属性必须保持 Slot 一一对应：Registry Slot 是结构契约，Figma 节点是设计定位，代码渲染目标是运行时定位，Component Token 的 `slot/{slot}` 是值绑定定位。不得将多个独立 Slot 展平为无归属的 `select/*` 属性集合，也不得产生无法回溯到 Slot 的最终视觉属性。
115. Select Recipe 只保存 Registry 引用、Base 设计值与引用、Appearance/State 引用、`open` 组合覆盖、Slot Token 引用和必要的稀疏组合覆盖，不重复声明组件轴、合法组合或 Slot 契约，也不保存 `data-source`、查询文本、选中数据、动态可用性和其他运行时业务状态。`selection` 与 `searchable` 只允许作为覆盖记录的 `when` 条件出现。
116. Button 作为第一批迁移基准组件，用于验证 Registry–Recipe–Slot–Generator 的通用架构；其他组件复用这套治理机制，但必须根据自身语义定义独立轴、合法组合和 Slot，不得把 Button 的 `Intent/Appearance/State/Content` 轴机械复制到 Select、Input 或其他组件。
117. Button 的合法组合必须使用可校验的声明式矩阵契约，不得仅使用无法定位约束的字符串 `"cartesian"`。矩阵契约必须明确参与生成的轴、约束集合和 Content 到 Slot 的结构规则；当前 Button 在无额外轴约束时生成 `4 × 6 × 6 × 5 × 3 = 2160` 个逻辑组合，由生成器计算，不得人工维护。
118. Button 的 Slot/Content 契约必须覆盖 `container`、`content`、`label`、`icon`、`border`、`focus-ring` 和 `spinner`；五种 Content 结构通过 Slot 组合表达，`icon-only` 不新增 Size 轴，Loading 通过 Spinner 覆盖普通 Content。`focus-ring` 保留结构能力但当前 Button 规范不启用；`dark` 由 Profile/Theme 表达，不新增 Button Intent 或 Slot。
119. Button 必须以当前 V2 新规范为唯一目标重新建模；现有 Button Registry、Recipe、Component Token、生成产物及历史映射不得作为新结构的兼容约束。旧资料统一迁移到 `history` 作为可追溯的历史证据，新 Button Registry、Recipe、Figma 变量/组件集和运行时生成产物必须按新规范重新建立；旧值只有在经过新规则重新判定后才能被采用，不得静默继承。
120. Button 的历史迁移采用组件级快照，不得为了归档 Button 而移动或替换包含其他组件的共享 `component-registry.json`。旧 Button Registry 条目、旧 Recipe、旧生成结果和相关映射分别保存到 `history/button/` 下的对应版本目录；历史内容只用于追溯和比对，不参与当前生成流程。当前源目录只保留按新规范重建的 Button Registry 条目与 Recipe。
121. Button 重建后的活动 Registry 只使用新规范中的规范名称；`default` 是唯一的初始交互状态名称，历史 `normal`、`idle` 等别名不得继续作为活动轴值或活动兼容契约。旧别名只保留在 `history/button/` 和迁移诊断记录中；如迁移输入仍出现旧别名，必须先转换为规范名称并记录来源，无法确定时输出诊断，不得静默扩展新轴。
122. Button 新 Registry 的活动轴固定为 `intent=primary|secondary|operation|danger`、`appearance=filled|outlined|dashed|filled-inner-shadow|text|link`、`state=default|hover|pressed|focused|disabled|loading`、`content=icon-only|icon-left-text|text-icon-right|text-only|icon-left-text-icon-right` 和 `size=small|medium|large`。`dark` 仅表达反色表面环境，由 Profile/Theme 管理，不得进入 Button Intent 活动轴；历史状态别名不进入活动轴；所有合法组合由声明式矩阵契约和 Slot 规则生成。
123. Button Recipe 必须按新 V2 结构完全重建：尺寸、Padding、Spacing、Radius 等进入 Base 层；Appearance/State 的视觉差异按 Slot 组织；Content 通过 Slot 组合规则表达；Loading 通过 Spinner 与 Content 覆盖表达；Intent 的视觉差异由组合覆盖解析。旧 Recipe 的历史覆盖不得直接继承，必须逐条重新判定、迁移或输出诊断；`old-tree-recovery` 不得作为新设计决策来源。
124. Button 的 Figma 变量组与 Component Set 必须由新 Registry/Recipe 生成：变量组按 `button/base/...` 与 `button/intent/{intent}/appearance/{appearance}/state/{state}/content/{content}/slot/{slot}/{property}` 组织；Component Set 使用 `Intent`、`Appearance`、`State`、`Content`、`Size` 属性。不得手工维护 2160 个组件；`dark` 由 Profile/Theme 表达，历史状态别名不得进入 Figma 活动变体。
125. Button 重建必须通过完整验收门槛后，才能作为其他组件迁移模板：Registry、Recipe、矩阵生成、错误/警告诊断、`generated/component.json`、Figma 变量与 Component Set、Vue/Qt 映射以及 History 归档均必须可验证；任一阻断性 `error` 都禁止进入下游生成或发布，`warning` 必须保留定位和处理记录。
126. Button 矩阵生成器必须先完成 Registry/Recipe 的矩阵解析、Slot 校验、Token 解析、合法组合清单和 Error/Warning 诊断，再生成 `component.json`、Figma 输入和平台映射输入。生成器不得把 2160 个组合写回为人工源文件；每个生成记录必须保留组件、轴值、Slot、属性、解析值和源路径的可追溯关系。
127. Button 采用完整垂直切片方式作为第一批迁移样本：先完成旧资料组件级归档，再重建 Registry/Recipe，改造现有 V2 编译器，生成矩阵与诊断，生成 Component Token、Figma 变量/Component Set 以及 Vue/Qt 映射，最后通过完整验收门槛。只有 Button 全流程通过后，才抽取编译器、校验器、诊断格式、历史归档格式、映射协议和 Figma 生成机制等通用技术路径；其他组件的轴、合法组合、Slot、Recipe 值和运行时行为必须重新定义，不得机械复制 Button。
128. Button、Select 与 Input 的 Token 模型和数据模型设计原则必须一致，但不要求三者的数据结构同构、轴集合相同、字段数量相同或参数组合相同。统一的是 Registry/Recipe/Slot/Generator/Validation 的职责边界、权威来源、引用关系、生成流程、诊断格式、Figma 产物边界和历史迁移原则；每个组件必须根据自身语义独立定义轴、合法组合、Slot、Recipe 数据和运行时契约，不得为了形式一致而增加无意义字段，也不得因为组件差异而另建一套特殊架构。
129. Input 采用与 Button、Select 相同的组件级历史迁移规则：现有 Input Registry 条目、`input.json`、生成产物和映射归档到 `history/input/`；活动源只保留按新规范重建的 Input Registry 条目与 Recipe，旧值不得静默继承。
130. Input 自身的合法组合由 `mode=text|password|search|multiline|resource-picker`、`appearance=outlined|filled|dashed`、`state=default|hover|focused|error|disabled|loading` 和 `size=small|medium|large` 独立定义：五种 Mode 均预留三种 Appearance 与六种 State，`outlined` 当前完整落地，`filled`/`dashed` 仅保留结构占位；Mode 之间互斥，模式差异通过 Slot 显示资格和必要覆盖表达，不复制整套视觉 Token。
131. Input 的 Component Token 路径采用 Base 设计值与视觉状态分层：Size、Padding、Spacing、Radius 和 Typography 进入 `component/input/base/...`；Appearance/State 的视觉差异进入 `component/input/appearance/{appearance}/state/{state}/slot/{slot}/{property}`；`mode` 不进入主 Token 路径，只通过 Registry 的 Slot 规则和 Recipe 的 `when` 覆盖表达结构资格与必要差异。
132. Input 的 Figma 变量组必须与 Component Token 路径一致；Component Set 使用 `Mode`、`Appearance`、`State`、`Size` 属性。`value-present`、Caret、行数、运行时高度、资源类型和错误来源不得成为公共 Figma 变体属性；Mode 虽不进入主 Token 路径，但作为 Input 的结构变体保留在 Figma Component Set 中。
133. `TextBrowser` 保持独立公共组件边界，不并入 Input 的 `mode`。Input 负责可编辑文本值，TextBrowser 负责文档或富文本展示；二者的内容语义、Slot、State、运行时 API、Qt 身份和 Figma 组件契约不同。TextBrowser 可以复用 Semantic/Base Typography、颜色、Surface、Spacing、Radius 和 Profile/Theme，但不得通过 Input Mode 复用编辑、Placeholder、Caret、Clear 或输入校验契约。
134. `TextBrowser` 当前属于低优先级组件：本阶段只冻结其独立公共边界、与 Input/resource-picker 的职责边界以及最小 Registry 入口，不作为 Button、Select、Input 的 Token 架构样板，也不优先展开完整变体矩阵、Figma 变量重建或运行时迁移。待核心组件完成新 V2 架构验证后，再单独审计其内容类型、链接/代码展示、状态、Slot 和合法组合。
135. Input 的 `multiline` 模式明确禁止内部 `clear` Slot：组件本体不显示、不保留、不可点击清除按钮；但运行时必须提供外部程序侧/业务侧清除 API，允许其他按钮或业务逻辑直接清空其值。该 API 不进入 Input 的 Component Token、Figma 变量或公共视觉 Slot；`text`、`password`、`search`、`resource-picker` 是否配置内部 `clear`，仍由各自 Recipe 的 Slot 资格决定。
136. Input 的 Mode–Slot 资格矩阵固定为：`text` 允许 `clear`；`password` 允许 `clear` 与 `password-toggle`；`search` 允许 `clear` 与装饰性 `search-icon`；`resource-picker` 允许 `clear` 与 `browse-action`；`multiline` 禁止 `clear`、`password-toggle`、`search-icon` 和 `browse-action`。各模式仍共享同一套主体视觉 Token，专属 Slot 只通过 Registry 资格和 Recipe 覆盖表达。
137. Input 在 Figma 中只建立一个公共 `Input Component Set`，不拆分为 `Input`、`Password`、`Search`、`TextArea` 等多个公共组件集。Component Set 使用 `Mode`、`Appearance`、`State`、`Size` 四个结构/视觉属性；`Mode` 虽不进入主 Component Token 路径，但必须作为结构变体保留。`value-present`、Caret、实际文本、运行时高度和 `clear` 可见性不得成为公共变体；`clear`、`password-toggle`、`search-icon`、`browse-action`、`spinner` 通过内部 Slot 表达。`filled` 与 `dashed` 暂保留结构占位，不得误认为已完成视觉实现。
138. Input 的 trailing 操作在 Figma 和代码结构中统一收敛为一个内部 `ActionSlot`，其子项包括 `clear`、`password-toggle`、`search-icon`、`browse-action` 和 `spinner`。`ActionSlot` 不是公共 SDK 组件、不是新的 Input 轴，也不形成独立公共 Token 家族；实际子项由 Mode–Slot 资格和 Loading 覆盖规则通过实例替换决定。正常状态下按 Mode 显示对应操作，Loading 时保留 `clear` 但置为 disabled，并用 `spinner` 替换密码、搜索或资源浏览操作。
139. 现有 Figma `.DSM/Input/TextArea` 集合必须迁移为统一 `Input Component Set` 的 `Mode=multiline` 结构，不得继续作为独立公共组件集。多行输入在 Figma 中只保留代表性布局样本，不建立 `line-count`、`height`、`auto-grow` 或其他多行尺寸变体；实际高度、可输入行数、自动扩展和窗口适配由运行时 API 与布局控制，Caret 仍属于运行时行为。
140. Input 的 Qt 实现映射固定为：`mode=text|password|search|resource-picker` 使用 `QLineEdit` 语义与单行编辑能力，`mode=multiline` 使用 `QPlainTextEdit` 的多行纯文本编辑能力；当前项目不存在 Qt Widgets 的 `QTextArea` 类。`QTextEdit` 与 `QTextBrowser` 分别属于富文本编辑和只读富文本浏览能力，不得误写为当前 Input 多行实现。
141. Input 的设计 Token 家族与 Qt 运行时组件身份分离：公共 Token 家族统一命名为 `Input`，覆盖单行、多行、密码、搜索和资源选择模式；运行时保留 `ComponentLab/Input → commonWidget::Z_InputField : QWidget` 的复合组件身份，以及 `ComponentLab/LineEdit → commonWidget::Z_LineEdit : QLineEdit` 的原生单行控件身份。`LineEdit` 不得重新建立独立的公共 Input Token 体系，`Z_InputField` 的密码、搜索、资源选择和多行行为也不得强行塞入原生 `Z_LineEdit`。
142. 新 Input 公共组件只负责输入控件本体，不包含 `Label`、`Hint/Helper Text`、`Error Message`、字符计数 Footer 或 `Form.Item` 布局。上述内容必须由外部 `Field/Form Wrapper` 负责组织和 Token 化；现有 `Z_InputField` 中的 `label`、`hint`、`footer`、`count` 及其对应 API 属于旧复合实现，迁移时不得继续扩展为 Input 的公共 Slot、Token 或 Figma 变体。
143. 当前 Input 迁移阶段只冻结 `Field/Form Wrapper` 与 `Input` 的职责边界，不立即建立 Field/Form 的完整公共 Token 模型、轴集合或 Figma Component Set。未来若需要建设 Field/Form，必须作为独立复合组件单独审计；在此之前，外部包装层不得反向污染 Input 的轴、Slot、Component Token 或 Figma 变体。
144. 当前 `Z_InputField` 中通过 Search 图标切换 `Loading` 图标、且未提供独立 `browse-action` 与 `spinner` 的实现属于历史实现，不得作为新 V2 运行时契约。新实现必须按显式 `ActionSlot` 重建：增加独立 `spinner` 与 `browse-action` 结构，由 Mode–Slot 资格和 Loading 覆盖规则解析；不得通过替换既有图标资源来模拟 Spinner，也不得把资源浏览动作伪装为 Search 操作。
145. 新 Input 运行时必须新增 `resource-picker` Mode，与 Registry 的 `mode=resource-picker` 对齐。该 Mode 复用单行 `QLineEdit` 输入结构，增加独立 `browse-action` 并通过外部文件/文件夹选择 API 返回资源路径；文件/文件夹类型属于程序侧参数，不新增 Figma 轴。`resource-picker` 不得引入 Select 的 `popup`、`option-item` 或搜索逻辑，Loading 时由独立 `spinner` 替换 `browse-action`。
146. Input 的活动 Appearance 只允许 `outlined`、`filled`、`dashed`。运行时旧值 `solid` 不得继续作为活动 Appearance 或新 Token 路径；`figma-light` 只表示预览/表面环境配置，必须迁移到 Profile/Theme，不得进入 Input 的 Appearance 轴。当前 `outlined` 为唯一完整实现，`filled` 与 `dashed` 仅保留结构占位。
147. Input 的活动 State 只允许 `default`、`hover`、`focused`、`error`、`disabled`、`loading`；运行时旧值 `focus` 必须迁移为 `focused`，不得进入新 Registry、Component Token 或 Figma 变体。`focus-ring` 仍是独立的 Slot 名称，不得被当作 State；当前 Focused 视觉使用紫色 Border，Focus Ring 能力保留但全局不启用。
148. Input 保留可选的 `prefix` 与 `suffix` 内部 Slot，以承载文本或图标前后缀；二者不新增公共轴、不拆分为新的组件家族、不进入 Mode 组合，也不属于 `ActionSlot`。只有经过设计证据和 Recipe 配置的模式才显示对应 Slot；前后缀的存在不得改变 Input 的主体 Appearance、State、Size 或 Loading 契约。
149. Input 的 Registry/Generator 必须解析完整的 `5 Mode × 3 Appearance × 6 State × 3 Size = 270` 个逻辑组合，但 Figma 只完整发布 `outlined` 的视觉实现；`filled` 与 `dashed` 在 Figma 中保留结构占位，不得被视为已完成的视觉 Token 或批准的最终变体。270 个组合不得由设计师手工维护，Figma 产物必须由 Registry/Recipe 生成。
150. 新 Input 的 `base` 层只能保存与 `Appearance`、`State` 和 `Mode` 无关的公共设计值，包括 Size 几何、Padding、Spacing、Radius、Typography 以及 Action/Prefix/Suffix 的结构尺寸；禁止出现 `base/state/{state}/...`。所有外观或状态差异必须进入 `appearance/{appearance}/state/{state}/slot/{slot}/{property}`；旧 Recipe 中的 `base/state/...` 只作为历史证据，不得被新 Recipe 继承。
151. `prefix` 与 `suffix` 的合法 Mode 资格固定为四种单行模式：`text`、`password`、`search`、`resource-picker`；`multiline` 暂不支持这两个内部 Slot。多行输入需要附加操作或辅助信息时，由外部 Wrapper 或未来独立结构承载，不得为了复用单行前后缀而破坏多行文本编辑区域。
152. Input 的 `readOnly` 不建立独立 State、Component Token 或 Figma 变体；其 UI 视觉必须直接复用 `disabled` 状态的视觉契约，包括 Border、Background、Content、Placeholder 和 ActionSlot 的禁用表现。`readOnly` 与 `disabled` 的运行时语义仍保持区别：`readOnly` 保留查看、选择和复制能力但禁止编辑，`disabled` 则整体锁定交互；若同时存在更高优先级的 `error` 视觉，则继续遵循 Error Border 优先级规则。
153. Input 同时处于 `readOnly` 与 `loading` 时，`loading` 优先于 `readOnly` 解析。组件使用 Loading 的视觉与交互契约：锁定全部操作、保留当前值、已配置的 `clear` 保持 disabled，`password-toggle`、`search-icon`、`browse-action` 等操作由最右侧独立 `spinner` 替换；Loading 结束后恢复 `readOnly` 对应的 disabled 视觉和只读交互语义，不新增组合 State 或 Token 路径。
154. Input 同时处于 `error` 与 `loading` 时，采用“Error 视觉保留、Loading 交互优先”的组合规则：`border` 继续使用错误语义色并保持 Error Border 优先级；内容和值保留；所有编辑与操作由 Loading 锁定；`clear` 保持 disabled；`password-toggle`、`search-icon`、`browse-action` 由最右侧独立 `spinner` 替换。该组合不新增 State 或 Token 路径。
155. Input 同时处于 `readOnly` 与 `focused` 时，`readOnly` 的 Disabled 视觉优先：Border、Background、Content、Placeholder 和 ActionSlot 均复用 Disabled 契约，不显示 Focused 紫色 Border，也不启用 `focus-ring`。运行时仍可获得焦点并进行查看、选择和复制，但不得恢复编辑交互或新增组合 State。
156. Input 的状态冲突必须通过 Registry 中分离的 `visualResolution`、`interactionResolution` 和 `slotResolution` 三套规则解析，不得新增 `error-loading`、`readonly-focused`、`disabled-loading` 等组合 State。视觉规则负责 Border/Background/Content 的优先级，交互规则负责编辑、聚焦和锁定行为，Slot 规则负责 `clear`、操作按钮与 `spinner` 的显示、替换和 disabled 状态；三者的解析结果共同生成当前组件表现。
157. Input 的 `error` 必须由业务侧/开发侧通过显式 API 设置，Input 不根据输入值、Validator 结果、`value-present` 或格式规则自行推断 Error State，不生成错误文案，也不新增 Validator/Message 等 Token 轴。Input 只负责呈现 Error Border/视觉、按已确认规则锁定交互并向外透传状态；错误文案和恢复流程由外部 `Field/Form Wrapper` 或业务层负责。
158. Input 的 `border` 与 `focus-ring` Slot 统一使用规范属性 `border-width`；旧 Registry/Recipe 中的 `width` 只作为迁移别名，必须在解析时归一化并记录来源，不得进入新的 Component Token、Figma 变量或运行时映射。`visual-width` 仅用于几何尺寸，不得替代边框宽度属性。
159. Input 生成器必须按 Mode–Slot 资格省略不适用的 Slot：不生成 `null`、空字符串或虚假占位 Token，也不因 Slot 不适用而报错；只有 Slot 在当前合法组合中适用但缺少必要值时，才输出可定位的阻断性 `error`。例如 `multiline` 不生成 `clear`、`password-toggle`、`search-icon` 或 `browse-action`，而 `password` 必须为适用的 `password-toggle` 提供完整解析值。
160. Input 的视觉完整性校验必须区分正式 Appearance 与结构占位：`outlined` 缺少适用 Slot 的必要视觉值时输出阻断性 `error`，禁止发布；`filled` 与 `dashed` 在 `structural-placeholder` 状态下缺少视觉值时只输出可定位的 `warning`，允许生成结构骨架但不得生成可发布的完整 Figma/运行时视觉产物。只有 Appearance 被提升为正式发布状态后，缺值才升级为阻断性 `error`。
161. Input 的 `legalCombinations` 必须使用声明式矩阵契约，不得继续使用旧字符串 `"cartesian"`。契约至少声明 `mode`、`appearance`、`state`、`size` 参与生成的轴、约束集合、Mode–Slot 资格规则和 Appearance 完整性状态；生成器负责计算 270 个逻辑组合、执行 Slot/Token/占位校验并输出 Error/Warning，不得将组合展开写回为人工源文件。
162. Input 的 `placeholder` 不拥有独立颜色 Token；Placeholder 文本颜色必须跟随当前 Appearance/State 解析出的文本色（`content/color`）或其明确的状态文本继承值，不得生成 `slot/placeholder/color`。`placeholder` Slot 只负责显示条件、结构定位和必要的排版属性；旧 Figma/Recipe 中独立绑定的 Placeholder 颜色只作为历史证据，不得继承到新 V2。
163. Input 的 `placeholder` 排版属性必须完全继承 `content` 的 Typography 解析结果，包括 `font-family`、`font-size`、`font-weight`、`line-height` 和 `letter-spacing`；不得建立 `slot/placeholder/font-*` 或其他独立 Placeholder Typography Token。Placeholder Slot 只保留显示条件和结构定位职责。
164. 用户所说的 `Happy 1/2/3/4` 统一规范化为 `Heading 1/2/3/4`，活动命名使用 `heading` 语义，不得把语音转写结果 `happy` 写入 Typography、Component Token、Figma 变量或运行时映射。Heading 预设属于统一 Typography Composite Token 包，组件只引用，不在组件层重复展开字体属性。
165. `Unicode` 在本项目中不定义为视觉文本样式预设，而定义为字体包的字符覆盖与 fallback 元数据。它不拥有独立的字体家族、字号、字重、行高或字间距 Token，也不得成为组件轴、Figma 变量轴或运行时字符能力轴；字符覆盖由 HarmonyOS 字体包及平台 Typography Adapter 负责。
166. Typography Composite 包的视觉文本角色使用同级语义结构：`semantic/typography/body/{preset}`、`data/{preset}`、`display/{preset}`、`heading/{preset}`、`label/{preset}`、`subtitle/{preset}` 等。每个视觉预设统一解析为 `font-family`、`font-size`、`font-weight`、`line-height`、`letter-spacing` 等复合属性；Unicode 只允许作为保留的策略元数据存在，不得伪装成视觉预设。Component 层只保存对 Typography Composite 的引用，不得为单个组件复制字体复合结构。

167. Typography Composite 在设计源中必须作为单一复合 Token 引用存在，组件不得分别绑定 `font-family`、`font-size`、`font-weight`、`line-height`、`letter-spacing` 等字体属性。Composite 的 `$value` 统一保存这些属性的语义引用；编译到 Figma、Vue、Qt 等平台时，由编译器展开为目标平台所需的独立属性。组件层示例：`slot/content/typography: {semantic.typography.body.body-1}`。该展开属于编译产物，不得反向成为组件设计源。

168. Typography Composite 作为公共设计系统包，必须物理独立保存于 `DesignSystem/source/typography.json`。该文件负责 `body`、`data`、`display`、`heading`、`label`、`subtitle` 等视觉文本角色及其预设，同时保存字体包与字符覆盖策略元数据；普通 `semantic.json` 不再承载 Typography Composite 的完整复合结构。组件 Token、Figma 变量和各平台运行时均通过公共 Typography Composite 引用，字符覆盖由平台适配器解析；产品专用 Typography 扩展仍遵循产品专用配置与公共设计系统物理分离的规则。

169. `typography.json` 只负责 Typography Composite 的语义组合与引用，不得拥有第二套字体原始值。Composite 的 `fontFamily`、`fontSize`、`fontWeight`、`lineHeight`、`letterSpacing` 等成员必须引用 Primitive 或 Profile 中已有的权威值；不得在 `typography.json` 内重复写入原始字号、行高、字重、字族或字距。Primitive/Profile 负责值，Typography Composite 负责组合，Component 负责使用。

170. Typography Composite 定义为独立的 Token 构件包，不强行等同于 Figma 原生单个变量。设计源可以保存完整的复合构件及其引用关系；Figma、Vue、Qt 等下游根据平台能力将其拆解为可绑定的原子属性集合。Typography Composite 因此从普通 Semantic 变量层中物理拆出，但仍属于公共设计系统能力；Figma 侧不得因无法原生表达复合变量而重新建立一套不透明的字体语义。

171. Figma 侧每个 Typography Composite 必须展开为一个可识别的 Text Style 与变量协同构件组，例如 `DSM/Typography/Body/Body-1`。Text Style 是设计师实际应用的复合入口；`DSM/Typography` 变量组提供 `font-family`、`font-size`、`font-weight`、`line-height`、`letter-spacing` 等可绑定原子变量。`font-family` 使用 String Variable 且值必须是单一字体名，当前绑定 `HarmonyOS Sans`；`HarmonyOS Sans SC` fallback 不进入 Figma Font Family Variable，而由 Composite 元数据和平台适配器处理。该变量组和 Text Style 都是源端 Composite 的平台展开物，不是新的设计决策源；组件仍从源端引用单一 Composite，由同步/编译过程负责绑定组内原子变量。

172. 所有 Composite Token 必须采用统一的平台适配模型，不得只为 Typography 单独设计规则。字体、渐变、阴影、模糊、动效参数及其他由多个属性共同构成的视觉或行为 Token，均按“逻辑 Composite 源 → 平台适配表示 → 生成产物”三层处理：设计源保存一致的 Token 类型、身份、组成成员、引用关系和版本；Figma 根据能力采用变量、Local Style 或变量与 Style 协同的方式表现；开发侧可以在统一生成配置中保存完整 Composite，并按目标平台需要展开为可消费的字段。Figma 的 Style/Variable 结构和开发侧配置结构都是适配产物，不得反向成为 Composite 的设计决策源。

173. 建立全局 `Composite Registry` 作为所有复合 Token 的规则与适配权威源，保存 Composite 类型、组成属性、可引用的 Primitive/Profile 范围、Figma 表现策略、开发侧展开策略、校验规则和版本信息。建议物理结构为 `DesignSystem/source/composite-registry.json`，并由各类数据包分别保存实例，例如 `typography.json`、`effects.json`、`motion.json`。Registry 负责规则与编译约束，具体数据包负责 Composite 实例；数据包不得重复定义平台适配规则，平台生成物也不得反向覆盖 Registry 或数据包。

174. 所有 Composite 实例采用统一的外层数据结构：`id` 标识唯一身份，`type` 标识 Composite 类型，`version` 标识版本，`value` 保存该类型的组成属性，`metadata` 作为可选扩展信息。Typography、Gradient、Shadow、Blur、Motion 等类型只允许在 `value` 内定义各自的类型专属字段，不得改变外层身份、类型和版本结构。Composite Registry 按该统一外壳发现、版本化、校验和编译所有 Composite。

175. Composite 数据源必须保持平台无关，不得在实例中直接保存 `figma`、`css`、`qt`、`qss`、`vue` 等平台专属字段。源数据只表达设计语义、组成属性和对 Primitive/Profile 的引用；Figma、Vue、Qt、QSS 等平台差异统一由 Composite Registry 及其适配器规则处理。禁止因平台差异在源端复制出多份同义 Composite。

176. 开发侧 Composite 生成物必须同时保留来源引用和解析结果：`$ref` 指向设计源中的 Composite 身份，`$resolved` 保存经过 Registry 与适配器解析后的平台可消费属性。运行时直接消费 `$resolved`，不承担再次解析 Composite 的职责；生成器必须校验 `$ref` 与 `$resolved` 的一致性，并在解析失败、版本不一致或结果过期时输出可定位诊断。

177. Figma Composite 产物也必须保留来源追溯信息，包括源端 `sourceRef`、`sourceVersion` 和 `compositeType`。信息可按 Figma 能力写入变量描述、Local Style 描述或生成器维护的映射记录，但必须能够定位到同一个 Composite 源。Figma 产物的手工修改、来源缺失、版本过期或类型不匹配必须可被审计并输出诊断；Figma 产物不得成为新的设计决策源。

178. `Composite Registry` 必须为每种 Composite 显式声明平台适配策略。每个类型至少声明 Figma 表现策略和开发侧展开策略，例如 `variable-group`、`local-style-and-variables`、`resolved-config` 等。Composite 数据源不得携带平台专属字段；编译器根据 Registry 选择 Figma 变量、Local Style、二者协同或开发侧解析配置，并对未声明策略的 Composite 产生阻断性错误。

179. 所有 Composite 必须经过统一自动校验后才能生成。校验至少覆盖：`id/type/version` 外层结构、类型专属 `value` schema、Primitive/Profile 引用解析、Registry 类型存在性、Figma 适配策略、开发侧展开策略、`$ref` 与 `$resolved` 一致性，以及 Figma 产物的 `sourceRef/sourceVersion/compositeType` 可追溯性。缺失、结构不一致、引用无法解析或策略未声明属于 `error` 并阻断生成；过期、弃用、未使用或可兼容偏差属于 `warning` 并允许继续生成。

180. Typography Composite 的公共视觉角色集合确定为同级结构：`body`、`caption`、`data`、`display`、`heading`、`label`、`subtitle`。`caption` 不再作为普通 Semantic 中的孤立文本属性，而必须与其他视觉角色一样进入 `DesignSystem/source/typography.json`，通过统一的 Composite 外壳、引用规则、Figma 构件组和开发侧展开流程管理；Unicode 不属于该视觉角色集合，只保留为字体包策略元数据。

181. Typography preset 采用“角色 + 层级”的规范命名，例如 `heading/heading-1`、`body/body-1`、`caption/caption-1`。现有 sToken 文件可以作为角色、层级、原始值和历史命名的迁移参考数据源，但不再作为 V2 活动规范的最终权威；旧命名必须经过新角色体系、Composite 外壳、引用关系和命名规则归一化后，才能写入 `typography.json`。`happy-*` 等历史名称只允许作为迁移别名，不得继续作为活动 Token 名称。

182. 历史 SToken 名称与 V2 活动名称的映射必须物理独立保存于迁移资产，例如 `DesignSystem/source/migrations/token-aliases.json`。该映射文件只用于迁移、兼容性检查、旧引用定位和诊断，不参与新 Token 的运行时生成，不得被 Component、Figma 或平台运行时直接引用。迁移完成后，活动源只能使用 V2 规范名称。

183. Composite 数据包按类型物理拆分，至少包括 `DesignSystem/source/typography.json`、`gradient.json`、`shadow.json`、`blur.json` 和 `motion.json`。每个数据包只保存对应类型的 Composite 实例与设计引用，不重复保存平台适配规则；所有数据包统一由 `DesignSystem/source/composite-registry.json` 管理类型 schema、平台策略、版本约束和自动校验。

184. `motion.json` 只保存动效参数与组合关系，例如 `duration`、`delay`、`easing`、`property`、`from`、`to`、`sequence`；不得保存触发条件、业务状态、组件生命周期、交互事件或播放控制。Motion Composite 负责描述“怎么动”，Component 与 Runtime 负责决定“什么时候动、是否动以及由谁触发”。

185. `Composite Registry` 必须为每种 Composite 定义独立的 `valueSchema`，并由统一编译入口自动校验。`valueSchema` 至少声明必填字段、字段类型、允许的 Primitive/Profile 引用、字段之间的约束和平台适配策略。Typography、Gradient、Shadow、Blur、Motion 可以拥有不同的类型专属字段，但必须通过同一个 Registry 校验入口；缺失 schema、字段不完整、类型错误或引用越界属于阻断性 `error`。

186. Composite 允许引用 Primitive、Profile；只有在 `Composite Registry` 明确声明允许时，才可以引用其他 Composite。编译器必须构建 Composite 依赖图，检测未声明的跨类型引用、无法解析的引用和循环依赖；未声明引用、越界引用或循环引用均属于阻断性 `error`。Composite 的依赖方向必须可追溯，不得通过平台生成物隐式建立依赖。

187. Composite 使用稳定 `id` 与独立 `version`，版本号不得写入 Token 路径或替换 Composite 身份。兼容的数值或引用调整递增 `patch`；新增可选字段或兼容扩展递增 `minor`；字段语义变化、字段删除或平台契约变化递增 `major`。Figma 与开发生成物通过 `sourceRef + sourceVersion` 判断产物是否过期，不得因版本升级复制出新的同义 Token 身份。

188. Component 层必须整体引用 Composite，不得局部覆盖 Composite 内部字段。组件只能保存一个完整 Composite 引用；如果组件需要不同的字号、字重、行高、阴影参数、渐变结构或动效参数，必须在对应的公共 Composite 数据包中新增或调整 Composite，再由组件引用新的完整身份。Component 层不得通过额外的 `fontSize`、`fontWeight`、`shadowBlur` 等字段拆散或覆盖 Composite。

189. Component 可以根据 `Appearance`、`State` 或 `Slot` 选择不同的完整 Composite 身份，但不得修改被引用 Composite 的内部字段。例如，`state/default/slot/container/shadow` 与 `state/hover/slot/container/shadow` 可以分别引用两个完整 Shadow Composite；组件可以替换 Composite 引用，但不得在组件层增加 `shadowBlur`、`fontSize` 等局部覆盖字段。

190. Figma Composite 适配器必须输出可追溯的构件记录，至少包含 `sourceRef`、`sourceVersion`、`compositeType`、`variableGroup`、可选的 `localStyle` 和 `bindings`。Variables 用于保存可绑定的原子值，Local Style 用于表达 Figma 更适合承载的整体视觉效果，一个 Composite 可以同时使用二者。Component 只能使用适配器生成的绑定关系；Figma 无法表达的部分必须由 Registry 显式标记并输出诊断，不得通过未登记的手工结构补齐。

191. Composite 生成物必须与 Component 生成物物理分离，建议结构为 `DesignSystem/generated/composites/{platform}.json` 与 `DesignSystem/generated/component.json`。前者保存各平台解析后的 Composite、适配器记录和绑定信息；后者只保存组件对 Composite 的引用、合法组合和 Slot 绑定，不得重复展开同一个公共 Composite。各平台运行时可直接消费对应的 Composite 生成物，Component 生成器不得复制公共 Composite 的解析结果。

192. Typography Composite 的首轮迁移以现有 SToken 值为基准，不重新设计字体数值，不进行无必要的值重算或视觉调整。迁移工作只负责：按新角色体系归位、按“角色 + 层级”规范命名、封装为 Typography Composite、建立旧名称映射并接入统一校验。发现值缺失、重复或冲突时必须记录为迁移诊断，未经确认不得擅自修改 SToken 原始值。

193. 首轮 Typography Composite 迁移按现有 SToken 迁移结果建立默认绑定：`fontFamily` 使用当前公共 HarmonyOS 字体引用，`letterSpacing` 使用当前公共 `default` 引用，`fontSize` 与 `lineHeight` 保留现有 preset 引用，`fontWeight` 按现有 `body`、`display`、`heading2`、`label-strong`、`subemphasis` 等旧语义引用。对于现有数据中没有明确字重绑定的 preset，不得擅自创建新字重，只记录为待确认迁移诊断。

194. 渐变 Token 同步纳入 Composite 迁移范围。现有 `gradient-stop-01` 至 `gradient-stop-05`、组件层 `gradient` 属性以及各组件契约中的散落渐变字段均视为历史输入；新结构统一迁移到 `DesignSystem/source/gradient.json`，以完整 Gradient Composite 保存渐变类型、方向/几何、停止点顺序、停止点位置和颜色引用。迁移首轮保留现有颜色与几何值，不进行视觉重算；Component 层只引用完整 Gradient Composite，不得继续直接维护单个 `gradient-stop-*` 字段。

195. Gradient Composite 使用有序 `stops` 数组表达停止点，统一支持最少 2 个、最多 16 个停止点；每个停止点至少包含 `position` 和 `color`，`position` 必须位于 `0..1` 且按非递减顺序排列，`color` 必须是可解析的颜色 Token 引用。固定的 `gradient-stop-01` 至 `gradient-stop-05` 仅作为历史迁移别名，不再作为 V2 活动字段或 Component Slot。

196. Gradient Composite 首轮正式支持的类型仅为 `linear`，对应当前项目已有的 `linearGradient` 与 Qt `qlineargradient` 能力。`radial`、`conic` 等类型可以在 `Composite Registry` 中预留扩展位，但在完成对应的 schema、Figma 适配器、开发侧展开器和运行时验证前，不得进入活动 Gradient Composite；提前使用属于阻断性 `error`。

197. Linear Gradient 的方向与几何属于 Gradient Composite 本身，使用 `0..1` 归一化坐标保存 `geometry.from` 与 `geometry.to`。坐标不得依赖组件像素宽高，Component 层不得单独维护渐变方向或几何值；`from` 与 `to` 不得相同。Figma、CSS、Qt 等平台的坐标格式由适配器转换，组件尺寸不得成为 Gradient Token 的设计源。

198. Gradient 是可复用的视觉 Composite，不新增独立的 `gradient` Appearance 轴。`Appearance` 负责表达填充、描边、文本、链接等材质语义；具体 Slot 可以引用完整 Gradient Composite，例如 `appearance/filled/state/default/slot/container/background` 引用 `gradient.button.primary.default`。Gradient 不得与 Appearance 轴合并，也不得通过新增 `appearance=gradient` 表达。

199. Gradient 的引用必须遵循 Slot 适用性规则：某个 Appearance 或 State 不支持渐变时，直接省略 Gradient 引用，不得写入 `null`、空值、透明渐变或伪造的 `gradient-stop`。如果该 Appearance/State 已声明支持 Gradient，但缺少对应完整 Composite，则属于阻断性 `error`；适用性由 Registry 与 Recipe 共同声明并自动校验。

200. Gradient Composite 的 `stops[].color` 只能引用可解析的 Primitive 或 Semantic Color Token，不得在活动 `gradient.json` 中直接保存 Hex、RGB 或其他平台颜色字面量。迁移阶段可以在诊断或历史映射中保留原始色值，但进入活动 Composite 前必须归一化为 Color Token 引用；颜色解析由 Composite Registry 与颜色 Token 解析器统一完成。

201. 结构、几何、停止点顺序和颜色引用完全相同的 Gradient 必须去重并复用同一个 Composite 身份。不同 Component、Appearance 或 State 只能通过引用同一个 Gradient Composite 使用，不得复制同义的停止点、方向或颜色结构；编译器应检测可判定的重复实例并输出 `warning`，活动源不得保留无意义的重复副本。

202. Gradient Composite 优先采用语义用途命名，例如 `gradient/brand/primary`、`gradient/brand/primary-hover`、`gradient/surface/raised`、`gradient/status/danger`；不得因为当前使用者是某个 Component 就默认加入组件名称。只有当渐变的几何、颜色组合或视觉语义确实无法跨组件复用时，才允许使用 `gradient/component/{component}/...` 的组件限定命名。

203. Shadow Composite 使用有序 `layers` 数组表达多层阴影，首轮支持 0–8 层；每层至少包含 `inset`、`offsetX`、`offsetY`、`blur`、`spread` 和 `color` 等阴影参数。Shadow 只保存阴影本身，不得包含背景渐变、组件边框半径、组件尺寸或其他几何布局参数。`filled-inner-shadow` 等材质表现由 Gradient Composite 与 Shadow Composite 组合表达，历史混合字段必须在迁移时拆分。

204. Shadow 层中的 `blur` 表示该阴影自身的模糊半径，只能引用 Primitive/Profile 数值，不引用独立 `Blur Composite`。`blur.json` 仅用于独立施加于图层或背景的模糊效果，例如 backdrop blur、layer blur 和 surface blur；Shadow Composite 与 Blur Composite 的职责不得混合。

205. Blur Composite 首轮只迁移已有权威 SToken 或活动设计源中的 Blur 值；组件演示契约、实验参数或局部实现数值不得直接升格为公共 Blur Composite。`blur.json` 与 Registry 可以先建立类型 schema、适配策略和结构占位，但没有权威来源时不得创建新的设计值，相关缺口只输出迁移诊断。

206. Motion Layer V2 的类型覆盖基线来自外部《动效设计入门》课件，但课件 HTML、图片、演示代码和其他原始资料不得复制、迁入或作为 ComponentLab-Delivery 的运行时依赖。项目内只保存从外部资料提炼出的可执行设计规则：Position、Scale、Rotation、Opacity 四类属性；Linear、Ease In、Ease Out、Ease In Out 四类缓动；组合、交错、循环、反馈、过渡、进入、退出和等待等 Motion Composite 类型，以及对应的参数 Schema、组件 Hook 和平台适配规则。

207. Motion Composite 统一使用 `tracks`、`sequence`、`stagger`、`repeat`、`timing` 和 `metadata` 结构：`tracks` 表达 Position、Scale、Rotation、Opacity 的并行属性轨道；`sequence` 表达串行步骤；`stagger` 表达多元素交错；`repeat` 表达 Loading、Skeleton、Pulse、Spin 等循环；`timing` 引用 Duration、Delay、Easing；`metadata` 标记 `feedback`、`transition`、`loading`、`enter`、`exit` 等语义用途。触发事件、业务条件和生命周期仍由 Component Recipe 与 Runtime 管理，不得写入 Motion 参数本体。

208. Motion Track 统一使用 `keyframes[]` 表达属性变化，简单的 `from/to` 只作为两帧关键帧的简写。关键帧的 `at` 使用 `0..1` 归一化时间，`value` 必须根据 `position`、`scale`、`rotation`、`opacity` 等属性类型校验；必要时允许按关键帧区间声明缓动。该结构必须覆盖 Button Press、Pulse、Skeleton、Modal 等多阶段动效，Component 层仍只引用完整 Motion Composite。

209. Easing 采用“语义预设 + Primitive 曲线值”结构，至少包含 `linear`、`ease-in`、`ease-out`、`ease-in-out`；非线性预设保存为可追溯的 cubic-bezier 数值。Motion Composite 只能引用语义 Easing Token，不得直接写入 cubic-bezier 数组；课件中的其他曲线必须先命名并登记为 Motion Primitive，才能被 Composite 使用。

210. Motion 时间参数统一采用毫秒：`duration` 和 `delay` 的单位为 `ms`，二者只能引用 Motion Primitive；关键帧 `at` 使用 `0..1` 归一化进度，不携带时间单位。Component 不得直接写入时间字面量，Figma、Vue、Qt 等适配器负责将毫秒值转换为目标平台格式。

211. Motion Composite 使用 `repeat` 表达循环行为，至少包含 `count` 与 `direction`；`count` 支持 `1`、有限整数和 `infinite`，`direction` 支持 `normal`、`reverse`、`alternate`、`alternate-reverse`。Loading/Spinner 通常使用 `infinite + normal`，Skeleton/Pulse 可使用 `infinite + alternate`，一次性反馈使用 `count=1`。循环配置属于 Motion Composite，不属于 Component State；Component 与 Runtime 只负责启动、停止或取消。

212. Motion Composite 使用独立 `stagger` 配置表达交错动效，至少包含引用 Motion Primitive 的 `interval` 和播放 `direction`。List Item、Skeleton、多 Card 或 Popup 子项等集合由 Runtime 根据实际元素数量计算时间偏移；Component 不得按元素序号硬编码 `0ms/120ms/240ms` 等延迟。`stagger` 只改变每个元素的开始时间，不改变元素自身的 Motion Track。

213. Motion Composite 使用 `sequence.steps[]` 表达串行动效，每个 Step 必须引用一个完整 Motion Composite，不得在 Sequence 内复制子 Composite 的 Track。默认按前一步完成后顺序执行，后续可扩展 `startOffset` 或并行组；Registry 必须检测 Sequence 的循环引用。Component 只引用最终的 Sequence Composite，不直接管理内部步骤。

214. Motion Composite 使用公共 `metadata.purpose` 标记语义用途，至少包括 `feedback`、`transition`、`state-change`、`loading`、`enter`、`exit`、`attention`。该分类只用于 Composite 发现、Component Hook 约束、Figma/Runtime 适配和自动审计，不表示触发事件、不替代 Component State，也不得将组件名称写入公共 Motion Composite 的语义分类。

215. Motion Property 分为 Core 与 Extended 两级。Core 必须支持 `position`、`scale`、`rotation`、`opacity` 四类课件核心属性；Extended 纳入课件演示中明确出现的 `size/width/height`、`color/background` 和 `shadow` 等属性。Extended 属性必须先在 Composite Registry 声明 schema 与平台策略才能使用，不要求首轮立即建立全部公共数值；`gradient`、`shadow`、`blur` 等复合结构仍通过各自 Composite 引用，Motion 不得复制其内部字段。

216. Position Track 支持 `logical-px` 与 `relative` 两种明确空间。固定局部位移使用 `logical-px` 并引用 `motion.distance` Primitive；容器、页面、Drawer、Tab 和 Bottom Sheet 等随布局变化的位移使用 `relative`。Position 值不得使用无单位数字，Component 不得直接写入像素字面量；Figma、Vue、Qt 适配器负责将两种空间转换为目标平台格式。

217. Core Motion Property 的值域固定为：`scale` 使用无量纲比例；`rotation` 使用 degrees；`opacity` 使用 `0..1`。Scale 不使用百分号，Rotation 不使用弧度或无单位角度，Opacity 不使用 `0..100` 百分制；Registry 必须自动校验越界值，Component 不得直接写入这些属性字面量。

218. Component Motion Hook 使用公共事件分类：`enter`、`exit`、`open`、`close`、`press`、`hover`、`focus`、`state-change`、`loading-start`、`loading-stop`、`refresh`、`expand`、`collapse`、`attention`。Component Registry 声明组件支持的 Hook，Component Recipe 将 Hook 绑定到完整 Motion Composite，Runtime 根据真实交互事件触发；Hook 不保存动效参数，不直接绑定平台 API，也不得替代 Component State。

219. Figma 中的 Motion Hook 不得新增 Component Variant 轴。静态 Variant 只保存组件状态与结构；Prototype/Smart Animate 表达交互转场；Variables/Local Styles 保存可绑定的时间、缓动和属性值；Motion Mapping Metadata 保存 `sourceRef`、Hook、Composite 和版本。不得因 `duration`、`easing`、`repeat` 或 Motion 组合扩张 Variant 数量，Figma 只表现由 `motion.json` 与 Registry 生成的 Motion 映射。

220. Motion 的平台运行时适配必须保持分层：Vue 侧由 CSS `transform`、`opacity`、尺寸属性、transition/keyframes 或 Vue Motion Adapter 执行；Qt 侧由 `QPropertyAnimation`、`QParallelAnimationGroup`、`QSequentialAnimationGroup` 和对应 Runtime Adapter 执行；QSS 只保存静态状态，不承担动态播放。`motion.json` 编译为 `generated/composites/vue.json`、`generated/composites/qt.json` 等平台产物，Component Recipe 只绑定 Hook 与 Composite，运行时负责实际执行。

221. Motion Registry 必须执行专项自动校验：Track property 必须已登记；`keyframes.at` 必须位于 `0..1` 且按非递减顺序排列；属性值必须符合对应值域；`duration`、`delay`、`easing`、`repeat`、`stagger` 必须可解析；`sequence.steps` 引用必须存在且不得循环；Component Hook 必须声明支持该 Motion Composite；目标平台适配器必须支持所需属性。结构错误、无法解析、循环引用和平台不支持属于 `error`；兼容性降级或可选能力缺失属于 `warning`。

222. Motion 数据物理分层固定为：`DesignSystem/source/primitive.json` 的 `motion` 命名空间保存 `duration`、`delay`、`easing`、`distance`、`scale`、`rotation`、`opacity` 等原子动效值；`DesignSystem/source/motion.json` 保存 `feedback`、`transition`、`loading`、`enter`、`exit`、`sequence`、`attention` 等完整 Motion Composite；`DesignSystem/source/composite-registry.json` 保存 Motion schema、平台适配策略和校验规则。Component Recipe 只引用 `motion.json` 中的 Composite。

223. Motion Layer 首轮建立完整的最终态结构草案，并允许在评审前填入候选预设值。候选值必须显式标记为 `candidate`、`pending-review` 或等价状态，并记录来源为内部提案、课件参考或迁移推断；候选值不得进入发布版 Figma、Vue、Qt、QSS 生成物。评审通过后，候选值才能转为活动 Primitive/Composite；未通过的候选值必须保留诊断，不得被运行时隐式消费。

224. 本轮已确认内容按增量方式落盘：Button、Input、Select 的活动源分别位于 `DesignSystem/source/component-recipes/{button,input,select}.json`，组件契约位于 `source/component-registry.json`；Typography、Gradient、Shadow、Blur、Motion 分别位于 `source/{typography,gradient,shadow,blur,motion}.json`，统一类型规则位于 `source/composite-registry.json`，Motion 原子候选值位于 `source/primitive.json#/root/primitive/motion`。历史恢复文件只保存在 `docs/archive/design-system-history/history/pre-v2-recovery-20261018/`，不参与当前生成。源文件按发布单元判断：`draft-v2` 或仍为 `candidate` 的组件/Token Slice 不得直接更新 Figma、Vue、Qt 或 QSS 生成物；已评审为 `active` 且在源清单登记为 released slice 的切片可以进入对应运行时发布路径；必须先完成 Registry 驱动编译器和自动校验接线。

225. 后续组件治理采用“单步对齐、单步确认、单步落盘”的增量协作闭环：每一轮只处理一个明确的组件问题、一个轴/Slot/组合规则或一个迁移决策；对齐完成后，必须先获得确认，再同步更新规范文档、Registry/Recipe/Composite 源文件、必要的历史记录和源清单，并执行 JSON、引用和结构校验；校验结果必须记录在本轮交付中。未确认内容只能保留为事实、候选值、诊断或待决策项，不得提前写入活动 Token、Figma 变量或平台生成物。

226. 组件审计顺序按依赖关系推进：优先处理被其他组件 Anatomy/Slot 复用的基础组件或结构；当前 Checkbox 作为 Select 多选 `option-item` 的复用结构，列为 Button、Input、Select 之后的下一审计对象。组件顺序可以调整，但每次调整必须说明依赖原因，并保持“一组件一轮确认”的边界。

227. Checkbox 的 `value` 与 `state` 必须保持独立轴：`value` 使用 `unchecked`、`checked`、`indeterminate` 表达选择值或聚合选择结果；`state` 使用 `default`、`hover`、`focused`、`pressed`、`disabled` 表达交互状态。二者允许组合，例如 `checked + disabled` 表示已选但当前不可用，`indeterminate + focused` 表示部分选中且获得键盘焦点；不得把 `checked`、`unchecked` 或 `indeterminate` 误建模为交互 State。

228. Checkbox 的 `focused` 状态启用键盘焦点环：Focus Ring 只表现为边框，不叠加背景、文字或选中颜色变化；鼠标 Hover 和 Press 不触发 Focus Ring。Select 的多选 `option-item` 和 Popup 批量控制复用 Checkbox 时，沿用该键盘焦点规则；Select 触发器自身仍遵循 Select 的公共 Focus Ring 规则，不因复用 Checkbox 而改变。

229. Checkbox 的三种 `value` 只改变 `indicator` Slot 的值表现：`unchecked` 使用空指标，`checked` 使用选中指标与 Checkmark，`indeterminate` 使用部分选中指标与横线。Value 变化不得改变 Label、Size、布局或交互 State；Select 的多选 `option-item` 与 Popup 批量控制复用同一套三态指标结构。

230. Checkbox 进入 `disabled` 后必须保留当前 `value` 及其指标结构：`checked + disabled` 仍显示选中指标，`indeterminate + disabled` 仍显示部分选中指标；Indicator 与 Label 切换为禁用视觉，所有鼠标、键盘和程序侧交互均锁定。`disabled` 不得自动清除或重置 Value；Select 中异常已选 Checkbox 必须复用该规则。

231. Checkbox 的 `pressed` 只表达激活进行中的交互视觉，不改变 `value` 轴；按下期间必须保留当前 Value 及其 Indicator 结构，释放后的选中、取消选中或其他值变更由运行时交互逻辑负责。Token、Recipe 和视觉解析器不得在进入 `pressed` 时隐式修改 Value。

232. Checkbox 的交互 State 视觉优先级固定为 `disabled > pressed > focused > hover > default`。当多个交互条件同时成立时，由最高优先级的活动 State 决定视觉表现；`value` 轴不参与该优先级竞争，必须继续保留当前的 `unchecked`、`checked` 或 `indeterminate` 表现。

233. `indeterminate` 的 Value 跃迁属于运行时行为，不属于 Checkbox Token。通用 Checkbox 只提供 `indeterminate` 的视觉表现，由业务运行时决定激活后的 Value；Select Popup 批量控制单独使用三态规则：`unchecked` 或 `indeterminate` 激活后变为 `checked`，`checked` 激活后变为 `unchecked`。Token、Recipe 和视觉解析器不得隐式执行 Value 跃迁。

234. Checkbox 的 `indicator` 与 `label` 组成一个统一交互区域：点击 Label 必须代理到 Checkbox 控件，执行与点击 Indicator 相同的运行时动作；Label 不建立独立 Focus、Hover 或 Press 目标，键盘 Focus 只归属于 Checkbox 控件本身。Disabled 时整个交互区域一并锁定。

235. Checkbox 增加 `content` 轴，值域为 `control-only` 与 `control-with-label`。`control-only` 表示纯 Checkbox 控件，用于表格首项、批量选择等场景，Label Slot 不参与结构和交互；`control-with-label` 表示 Checkbox 与文本 Label 的组合，Label 参与统一交互区域。两种 Content 仍共享 `value`、`state` 和 `size` 轴，不拆分为两个公共组件家族。

236. Checkbox 的 `size` 轴使用四档语义名称：`compact`、`small`、`medium`、`large`，分别引用现有 Profile 的 20、24、28、32 尺寸值。四档 Size 同时适用于 `control-only` 与 `control-with-label`；实际数值继续由 Profile 提供，Checkbox 不重复建立原始尺寸值。

237. Checkbox 必须区分 `Visual Size` 与 `Interaction Size`：`compact/small/medium/large` 的 Visual Size 分别引用 20/24/28/32；对应 Interaction Size 引用下一档 24/28/32/36，保持 4px 步进。Visual Size 归属 `indicator` Slot 的 `visual-size`，Interaction Size 归属 `container` Slot 的 `interaction-size`；两者不得使用同一个属性或互相替代。

238. Checkbox 的 Interaction Size 作用范围由 `content` 决定：`control-only` 将 Interaction Size 应用于整个 Checkbox 控件区域；`control-with-label` 只将 Interaction Size 应用于 Indicator 区域，Label 按文本内容自然布局，但仍属于与 Indicator 相同的统一交互区域。不得因为 Label 可点击而把 Label 文本尺寸强行扩展为 Checkbox 的方形 Interaction Size。

239. Radio 的 `value` 与 `state` 必须保持独立轴：`value` 使用 `unselected`、`selected` 表达选择值；`state` 使用 `default`、`hover`、`focused`、`pressed`、`disabled` 表达交互状态。历史 `selected-hover`、`selected-pressed`、`selected-disabled` 等组合名称不得进入 V2 State 轴；它们应由 `value=selected` 与对应交互 State 的组合解析。

240. Radio 的 `focused` 状态启用键盘 Focus Ring：Focus Ring 只表现为边框；鼠标 Hover 和 Press 不触发 Focus Ring。该规则与 Checkbox 的 Focus Ring 规则一致，但只共享交互原则，不自动共享 Radio 的视觉值或其他组件 Token。

241. Radio 进入 `disabled` 后必须保留当前 `value` 及其 Indicator 结构：`selected + disabled` 仍显示选中表现，`unselected + disabled` 仍显示未选中表现；Indicator 与 Label 切换为禁用视觉，所有鼠标、键盘和程序侧交互均锁定。`disabled` 不得自动清除或重置 Value。

242. Radio 的 `indicator` 与 `label` 组成一个统一交互区域：点击 Label 必须代理到 Radio 控件，执行与点击 Indicator 相同的选择动作；Label 不建立独立 Focus、Hover 或 Press 目标，键盘 Focus 只归属于 Radio 控件本身。Disabled 时整个交互区域一并锁定。

243. Radio 的 `size` 轴与 Checkbox 对齐，使用 `compact`、`small`、`medium`、`large` 四档语义名称；Visual Size 分别引用 20/24/28/32，Interaction Size 分别引用下一档 24/28/32/36，保持 4px 步进。Visual Size 归属 `indicator` Slot 的 `visual-size`，Interaction Size 归属 `container` Slot 的 `interaction-size`；该对齐只统一尺寸建模原则，不表示 Radio 与 Checkbox 共享其他视觉 Token。

244. Radio 的两种 `value` 只改变 `indicator` Slot 的结构：`unselected` 使用空心圆，`selected` 使用选中环与中心圆点。Value 变化不得改变 Label、Size、布局或交互 State；Disabled 状态继续保留对应 Value 的结构并切换禁用视觉。

245. Radio 的 `selected / unselected` Value 变更由 Radio Group 或运行时负责，单个 Radio 不独立执行 Value 切换；选择一个 Radio 时，由 Group 取消其他成员的选中状态。Token、Recipe 和视觉解析器只消费当前 Value，不得隐式维护互斥选择逻辑或自动修改 Value。

246. Radio 的 `pressed` 只表达按下期间的交互视觉，不改变 `value` 轴；按下期间必须保留当前 `selected` 或 `unselected` 表现，释放后的选择变更仍由 Radio Group 运行时统一处理。Token、Recipe 和视觉解析器不得在进入 `pressed` 时隐式修改 Value。

247. Button、Input、Select 的 `spinner` Slot 必须至少登记 `color`、`size`、`stroke-width`、`track-opacity`、`arc-start-angle` 和 `arc-sweep-angle`；其中 `arc-sweep-angle` 表达可见弧长的角度范围，`arc-end-angle` 不单独保存，运行时按 `arc-start-angle + arc-sweep-angle` 推导，避免起止角与弧长产生冲突。三类组件共享 Spinner 几何参数，组件 Recipe 只保留必要的尺寸、颜色或位置覆盖。

248. Spinner 只能由组件的 `loading` 结构状态触发，不得新增 `spinner-active`、`spinner-rotating` 或其他 Spinner 专用 State 轴。`loading` 负责结构替换、视觉复用和交互锁定；Spinner 的连续旋转由 Vue/Qt 等运行时渲染器执行，Motion Token 只提供旋转轨道、持续时间、延迟、缓动和循环参数。

249. Motion Composite 采用 Token Slice 发布：`motion.loading.spinner` 及其实际依赖的 Primitive 参数评审通过后允许运行时消费；整个 Motion 包仍可保持 `candidate`，其他未评审 Motion 必须继续阻断 Figma、Vue、Qt 和 QSS 的发布生成。源清单必须同时记录包级状态、已发布切片和仍被阻断的候选切片，不能用包级 `active` 掩盖未评审动效。

250. Typography Composite 的字体资产与文字角色必须分离建模：`primitive.font-family` 保存可核验的字体家族资产，`semantic.typography.font-family` 保存供角色引用的语义别名，`typography.json` 保存 `fontFamily`、`fontSize`、`fontWeight`、`lineHeight` 与 `letterSpacing` 的复合角色。不得把 Figma 字体名称直接散落到组件 Recipe，也不得把字体家族、字号、字重和行高重新复制到组件层。

251. 当前 V2 Typography 使用一个统一管理的 HarmonyOS 字体包，正式解析顺序为 `HarmonyOS Sans` → `HarmonyOS Sans SC`：前者作为全局主字体，负责英文、数字和其他拉丁字符；后者作为简体中文缺字 fallback。二者不是两个互相竞争的设计系统字体，而是同一字体包中的字符覆盖分工。Inter 与 Noto Sans SC 不属于当前 V2 Typography 的正式字体资产。

252. Typography Composite 的默认角色引用 `semantic.typography.font-family.harmony-os-sans`，fallback 引用 `semantic.typography.font-family.harmony-os-sans-sc`；Figma 侧通过 Local Style 与 Variable 协同表达，Vue/Qt 侧由 Typography Adapter 按同一字体包顺序生成目标平台字体配置。字体包注册不等于 Unicode 视觉角色，Unicode 只作为字符覆盖策略元数据存在。

253. Unicode 不再作为 Typography 视觉 Token 的发布前置条件；Typography 包仍可因生成器、Figma 适配器或平台展开链路未接通而保持 `candidate` 且 `releaseBlocked=true`。该阻断属于发布管线状态，不得重新转译为 Unicode 缺失错误；生成器必须区分字体包策略完整性与平台发布接线状态。

254. Typography 的语义字重沿用 HarmonyOS 字体包中已登记的 `regular`、`medium`、`semibold` 与 `bold`，不得因为字体家族切换而自动降级或重映射。`body`、`heading2`、`label-strong`、`display`、`emphasis`、`title-strong` 与 `subemphasis` 按现有 Semantic 字重角色解析；`unicode` 保持空结构并作为 `reserved-metadata` 记录字体包字符覆盖策略。

255. Typography 的 Figma 映射必须由 `Composite Registry` 驱动：Registry 保存 `DSM/Typography` Variable Collection、`DSM/Typography/{role}/{preset}` Text Style 命名模板、各复合成员的 Variable Binding 和 `sourceRef/sourceVersion/compositeType` 追溯元数据。组件实例只应用 Text Style，不得在 Button、Input、Select 或其他组件内部重新绑定字体原子属性；Figma 产物未通过 Registry 编译与版本校验前，不得手工写入活动变量组或 Text Style。

256. Typography 的 Figma 迁移采用“并行重建”而不是旧样式原地改名：新建 `DSM/Typography` Variable Collection，并按 `DSM/Typography/{role}/{preset}` 建立 V2 Text Style；旧的 `DSM/Heading/*`、`DSM/Body/*`、`DSM/Label/*`、`DSM/Data/*`、`DSM/Display/*` 与 `DSM/Subtitle/*` 样式保留为 Legacy、只读和迁移参考，不得继续作为新组件的活动入口。`Standard` 与 `Wide` 必须作为同一 Variable Collection 的 mode 表达，默认 mode 为 `standard`，不得为两个 mode 复制两套 Text Style。旧样式到新样式的显式映射只保存在 `DesignSystem/source/migrations/token-aliases.json`，不参与运行时 Token 生成。

257. Typography 必须拥有独立的 Composite 编译阶段。编译器负责校验 `typography.json` 的角色、preset、五个必需成员及其 Primitive/Profile/Semantic 引用，按 `standard` 与 `wide` materialize Vue/Qt 展开值，并生成只读的 Figma Typography Mapping Manifest。该 Manifest 只描述 `DSM/Typography` Variable Collection、Text Style、mode、fallback 和 `sourceRef/sourceVersion/compositeType`，不得直接写入 Figma；只有 Registry 版本校验、来源哈希和产物漂移校验全部通过后，才允许进入单独的 Figma 同步阶段。Typography 编译错误必须阻断总编译，候选包可以生成带 `releaseBlocked` 的评审产物，但不得被误标为正式发布。

258. Figma 审计确认：现有 Typography 资产位于旧的 `DSM · 03.semantic` Variable Collection，使用 `Light / Dark` mode，并存在旧的 `DSM/*` Text Style；该集合不能直接转换为 V2 Typography，因为 V2 的 Typography mode 语义是 `standard / wide`，不是主题 mode。迁移必须新建独立的 `DSM/Typography` Collection，旧集合和旧样式保留为 Legacy。旧样式映射必须覆盖审计到的 `DSM/Label/AuxiliaryEmphasis` 与 `DSM/Label/Caption`，不得因旧页面未展示而遗漏。

259. Figma Typography 原子变量命名必须由 `Composite Registry` 固定模板生成：共享字体变量为 `font-family/harmony-os-sans`；每个角色与 preset 的字号、字重、行高和字间距分别使用 `font-size/{role}/{preset}`、`font-weight/{role}/{preset}`、`line-height/{role}/{preset}`、`letter-spacing/{role}/{preset}`。这些变量只能作为 `DSM/Typography/{role}/{preset}` Text Style 的绑定成员，Text Style 仍是设计师应用入口；不得为 `standard` 与 `wide` 复制变量或样式命名空间，两个 mode 只切换同一组变量的值。

260. Figma Typography 原子变量的类型、Scope 和 Text Style 绑定字段必须固定：`font-family/harmony-os-sans` 使用 String Variable、`FONT_FAMILY` Scope，并绑定 Text Style 的 `fontFamily`；字号、字重、行高和字间距变量使用 Float Variable、`TEXT_PROPERTY` Scope，分别绑定 `fontSize`、`fontWeight`、`lineHeight` 和 `letterSpacing`。所有数值变量必须在同一变量组内使用 `standard / wide` 两个 Mode；绑定字段不得由同步器推断或改名。

261. Figma Typography 同步必须遵循固定写入边界：先预检 `HarmonyOS Sans` 与 `HarmonyOS Sans SC`，再依次创建 `DSM/Typography` Collection、`standard / wide` Mode、原子变量、30 个 Text Style 及其变量绑定，最后校验 Collection 名称、Mode 与默认值、变量命名/类型/Scope、Text Style 数量/命名/绑定以及 `sourceRef/sourceVersion/compositeType` 追溯信息。任一编译或预检错误都必须在写入前终止；旧的 `DSM · 03.semantic` Collection 与 `DSM/*` Legacy 样式完全不修改。同步器不得执行部分写入后继续运行。

262. Figma Typography 同步必须幂等且具备异常恢复边界：目标 Collection 不存在时允许创建；目标存在且结构、来源版本与追溯信息一致时，只能更新同步器登记的 `DSM/Typography` 受管资源；目标存在但结构或来源版本不一致时必须阻断并报告，不得覆盖或自动迁移。同步器不得删除或修改非受管资源、旧 Collection、Legacy 样式或其他来源创建的节点；重复执行必须得到同一结构结果。

263. Figma Typography 采用双层来源追溯：每个 Variable 与 Text Style 的 description 只保存精简 JSON 元数据 `sourceRef`、`sourceVersion`、`compositeType`，用于在 Figma 内定位源端；完整的变量绑定、Mode、迁移映射、版本和校验记录统一保存于 `generated/composites/typography-figma.json`。Figma description 与 Manifest 都是生成产物，不是设计决策源；设计决策唯一来自 `DesignSystem/source/typography.json`，同步器不得把完整设计规则反写进 Figma description。

264. Figma Typography 同步错误统一输出到 `generated/reports/typography-figma-sync-diagnostics.json`，默认严重级别为阻断性 `error`。必须区分 `FIGMA-FONT-PREFLIGHT`（字体预检）、`FIGMA-COMPILE-BLOCKED`（编译/来源阻断）、`FIGMA-TARGET-STRUCTURE-MISMATCH`（Collection/Mode 结构不一致）、`FIGMA-VARIABLE-CONTRACT-MISMATCH`（变量名称、类型、Scope 或绑定字段不一致）和 `FIGMA-TEXT-STYLE-CONTRACT-MISMATCH`（Text Style 数量、名称、绑定或追溯信息不一致）。每条报文必须包含 `severity`、`code`、`phase`、`target`、`sourceRef`、`sourceVersion`、`location`、`message` 和 `blocking`，并明确定位到 Figma 目标或源文件路径；未完成定位的泛化错误不得作为同步结果发布。

265. Typography 同步先读取目标环境中的实际 Collection、Mode、Text Style 与字体能力，再与生成 Manifest 比较；旧数量审计仅保留在 docs/archive。字体不可用或来源身份不一致时禁止实际同步，不以历史环境可用性代替当次检查。

266. Checkbox 不展开保存完整的 `value × state` 笛卡尔积。解析顺序固定为：共享 Base → Value 结构 → Value 调色板分组 → State 稀疏覆盖 → 独立 Focus Ring。`unchecked` 使用 `unchecked` 调色板组；`checked` 与 `indeterminate` 共用 `selected` 调色板组，但分别保留勾选标记与横杠标记结构。State 覆盖只保存实际变化的属性，未声明属性继承前一层；`disabled` 必须保留 Value 结构，Focus Ring 不得替换 Indicator 自身边框。具体视觉颜色值仍需后续单独完成，不得从旧恢复文件隐式复制。

267. Checkbox 当前视觉映射使用公共 Semantic Token 作为 V2 候选值：`unchecked` 组引用控件背景、默认边框和边界线宽；`selected` 组引用品牌交互默认色、边界线宽和反色标记；`hover`、`pressed`、`disabled` 分别覆盖对应调色板组，`focused` 只写入独立 Focus Ring，`disabled` 另覆盖 Label 禁用色。该映射来自当前 Semantic 层和已审计的隔离 Demo 证据，不等同于最终视觉批准；Recipe 保持 `draft-v2`，后续视觉评审只需调整 Palette/State 引用，不得恢复旧树的完整 State Token 复制方式。

268. Table V2 必须按复合数据表格建模，不得继续将 Table 视为单一视觉变体矩阵。Table 的权威模型由共享尺寸/间距绑定、表格结构、列职责、Cell 内容展示、Cell Editor、运行时状态层和视觉 Token Recipe 组成；其中 `component-recipes/table.json` 只保存视觉 Token 映射，结构与运行时语义必须保存在独立的 `source/component-contracts/table.json`。

269. Table 的视觉轴只保留有限、稳定且可由设计师直接选择的维度。当前 V2 视觉轴为 `density`（`compact`、`small`、`medium`、`large`）和 `striping`（`none`、`zebra`）。`interactionPreset`、行选择模式、单元格选择模式、全选范围、列职责、内容类型和编辑器类型属于结构/运行时配置，不得生成 Figma 视觉 Variant 或组件 Token 笛卡尔积。

270. Table 必须复用公共 Size、Interaction Size、Spacing 和 Padding Token。Table Recipe 只能建立 Slot 到公共 Token 的映射；不得因为 Table 需要行高、Cell 内边距、选择列宽或操作列间距而复制新的 Primitive/Shared 数值。表格容器外部布局距离属于布局层，Cell 内部左右留白使用 `padding-inline` 明确表达。

271. Table Column 必须区分列职责：`data`、`row-selection`、`row-index`、`row-actions`、`row-expand`、`row-drag` 和 `row-indicator`。表头全选控件、行选择控件和业务布尔字段编辑器不是同一个语义；可以复用 Checkbox 的视觉 Token，但必须通过不同的结构/运行时契约区分作用范围。

272. Table Cell 的 `display` 与 `editor` 必须是独立维度。可输入、多行、下拉、搜索、带按钮、主辅内容和整行选中可以在同一 Cell/Row 中同时出现，不得压缩为单一 `type` 属性。字段值来源、内容保护方式、格式化方式、编辑器类型和提交规则属于 Component Contract 或 Runtime，不属于视觉 Token。

273. Table 状态必须按层叠加，不得建立包含 `normal`、`hover`、`selected`、`focus`、`editing`、`error` 和 `disabled` 的互斥大 State 轴。至少区分指针交互、焦点、行选择、单元格选区、编辑会话、校验、可用性、持久化和数据质量；`loading` 与 `empty` 属于结构状态，不得转译为行交互 State。

274. Table Recipe 使用稀疏 `overlays` 表达视觉覆盖。Overlay 通过已登记的维度条件、Slot 和属性保存实际变化的 Token，输出路径为 `component.table.overlay.{dimension}.{value}...`；未覆盖的属性继承共享 Base 或其他解析层。Overlay 不得重新展开为所有可能的维度组合，运行时按照 Registry 声明的解析优先级合成最终视觉。

275. Table 的排序、筛选、分页、虚拟滚动、选择集合、焦点记录、编辑草稿、校验和提交由 Runtime/Component Contract 管理。Token 只负责容器、表头、行、Cell、边框、选择控件、排序指示器、状态指示器、空状态和加载状态的视觉取值；Token 不保存字段绑定、数据源、权限、事件或业务提交逻辑。

276. 旧 Table Recipe 的视觉值可以作为迁移基线，但旧的 `state` 复合名称、`item/content/icon` 模糊 Slot 和 `cartesian` 组合规则不得进入 V2。迁移时将 `alarm/error/warning` 映射为数据状态，将 `*-hover` 和 `*-select` 映射为状态与交互/选择 Overlay，将 `emphasis/invert/revise/unit` 映射为内容角色，将 `zebra` 映射为 Striping；无法从旧资料确定的值必须保留为迁移缺口，不得静默伪造。

277. Table V2 支持 `flat` 与 `hierarchical` 两种表头模式。多级表头必须建模为递归表头树，节点类型只有 `group` 与 `leaf`；`group` 表示跨多个子列的表头，`leaf` 必须唯一绑定一个 `columnId`。多级表头是结构/运行时 Contract，不是新的视觉 Variant 轴。

278. 多级表头的 `colSpan` 与 `rowSpan` 不由设计师或开发者手工填写，也不进入 Token。`colSpan` 根据节点子树中的叶子列数量派生，`rowSpan` 根据表头树最大深度和当前叶子所在层级派生。表头树中的列顺序必须与正文列顺序一致，每个数据列只能出现一次；行选择、序号、操作和展开等辅助列也必须以表头叶子节点表示。

279. 多级表头默认只有叶子表头承担排序、筛选和调整列宽命中；组表头默认只承担分组标题展示。若业务需要组级排序、筛选或批量操作，必须在 Component Contract 中显式声明，不能由表头层级自动推断。

280. Figma 使用 `Header Row`、`Header Group`、`Header Leaf` 和 `Header Divider` 嵌套组件表达多级表头；顶层 `ComponentLab/Table` 仍只保留稳定的 `density` 与 `striping` 视觉轴。表头层级数量不生成 `level-1`、`level-2` 等 Token；默认复用统一 `slot/header/*` Token。只有经过视觉评审确认组表头与叶子表头确实存在稳定的视觉差异时，才允许增加 `header-group` 与 `header-leaf` 角色 Token。

281. Figma 中的多级表头实例是 Contract 的可视化样例，不代表可以在设计侧任意改变列树。设计、生成器和运行时必须共同校验：叶子列唯一性、列顺序、跨度派生结果、排序/筛选命中范围，以及表头与正文列的对应关系。

282. Table 的冻结能力由开发侧 Runtime API 管控，不属于设计侧 Variant、Token 或 Figma 变量。Contract 可声明冻结表头、前置列和末尾列；冻结实现必须处理滚动同步、冻结边界、遮挡层级、列宽变化、选择、编辑和焦点的一致性。Figma 只保留冻结效果的示例，不建立 `freeze-left`、`freeze-right` 或 `freeze-header` 视觉轴。

283. Table 表头全选控件只在 `rowSelectionMode=multiple` 时出现，使用 `unchecked`、`indeterminate`、`checked`、`disabled` 四态。全选状态只根据 Runtime API 声明的 `current-page`、`filtered-result` 或 `all-records` 作用范围计算；不可选择的行必须排除在统计范围之外。表头全选只改变行选择集合，不修改业务字段；其视觉复用 Checkbox Token，表格仅负责选择列的结构、尺寸和作用范围。

284. Table 正文单元格支持由开发侧 Runtime API 显式声明的 `rowSpan` 与 `colSpan`，但正文合并不属于设计侧 Token、Figma Variant 或内容类型推断能力。每个合并区域必须绑定稳定的单元格地址并指定一个主单元格，其余位置由运行时标记为占用；启用前必须校验它与排序、筛选、分页、虚拟滚动和编辑能力的兼容性。

285. Table 复合单元格由 Runtime API 管理。展示布局可声明为 `single`、`primary-secondary`、`horizontal`、`vertical` 或 `composite-value`；编辑组的字段、布局、联合校验范围和提交范围由运行时声明。复合单元格属于 Cell 内部结构，不等同于 `rowSpan/colSpan`；Table 只负责 Cell 的布局、间距和插槽，Input、Select、Checkbox 等嵌套编辑器继续复用各自的 Component Token。复合单元格不生成新的 Table 视觉轴或笛卡尔积。

286. Table 数据生命周期状态统一为 `ready`、`loading`、`empty`、`no-match`、`error` 五类，作用范围是整个 Table。`loading`、`empty`、`no-match` 和 `error` 属于结构状态；行级 `data-status.warning/error/alarm` 仍表示业务数据状态，二者不得混用。状态转换、数据源、失败详情和重试由 Runtime API 管理，Figma 仅提供对应 State Slot 或结构样例，Token 只负责视觉表现。

287. Table 表头操作必须通过明确 Slot 建模。内置表头操作至少包括 `sort` 与 `filter`，分别使用 `sort-indicator` 和 `filter-trigger`；通用 `action-group` 与 `header-action` 用于承载其他表头操作。表头操作不新增 Table 视觉轴，表头间距由 `slot/header/action-gap` 管理，具体操作控件继续复用自身 Component Token。

288. 开发侧可通过 Runtime API 注册自定义表头操作。每个扩展动作至少声明 `actionId`、作用目标 `header-cell` 或 `table`、目标 Slot、`enabled`、`visible` 和事件处理；自定义动作不得绕过表头事件优先级、权限判断或当前数据生命周期状态。Figma 只提供可替换的 Header Action Slot，不预先穷举业务操作。

289. Table 的列布局由 Runtime/Component Contract 管理。列宽策略支持 `auto`、`fixed`、`minmax`、`flex`，列宽调整、列重排和列显隐均由开发侧 API 控制，不生成 Figma Variant 或 Component Token 轴。选择列、序号列和操作列可使用固定宽度；数据列可使用自适应策略。任何列布局变化都必须保持多级表头、正文、冻结列和编辑器的对齐。

290. Table 的数据窗口能力由 Runtime API 管理。分页支持 `none`、`page`、`cursor`，虚拟化支持 `none`、`rows`；它们不进入 Figma Variant 或 Token。选择集合、键盘焦点和编辑草稿必须基于稳定 `recordId` 与字段/编辑组标识保存，不能依赖当前行号或当前可视窗口。数据窗口变化必须保持表头、列布局、冻结列和编辑器的结构视觉契约。

291. Table 数据刷新支持 `manual`、`polling` 和 `push` 三种 Runtime 方式。刷新协调必须通过稳定 `recordId` 保留滚动位置、已选记录、焦点记录和编辑草稿；外部数据更新不得静默覆盖未提交草稿。草稿冲突由 Runtime 根据 `preserve-and-mark-stale` 或 `reject-update` 策略处理，不进入 Figma Variant 或 Component Token。

292. Table 编辑校验范围与提交范围必须独立建模，均支持 `cell`、`edit-group`、`row`、`column` 和 `table`。`row` 作用于一个记录声明的字段集合；`column` 作用于显式声明记录集合中的同一字段，适用于批量编辑或批量校验；`edit-group` 作用于具有业务关联的多个字段。提交触发方式支持 `explicit`、`blur`、`enter` 和 `runtime`，全部由 Runtime API 控制，不进入 Figma Variant 或 Component Token。

293. Table 键盘交互支持 `tab-sequence`、`grid-navigation` 和 `native-editor` 三种 Runtime 模式。键盘事件优先级固定为已打开浮层、嵌套编辑器、Cell、Row；焦点归属和导航由 Runtime API 管理。方向键、Enter、F2、Esc 的具体行为必须服从当前模式，输入法候选确认不得误触发表格提交。

294. Table 单元格内容溢出时复用公共 Tooltip/Popover 组件，不建立 Table 自有气泡 Token。Table 只提供触发条件和完整内容 Slot；触发方式支持 `hover`、`focus`、`keyboard`、`explicit`，内容支持 `single-line` 与 `multi-line`，浮动方向支持自动定位及上下左右方向，尺寸支持自动或受约束模式。气泡的视觉 Token、最大尺寸、滚动和边界避让由 Tooltip/Popover 组件管理，不能因气泡出现而改变表格行高。

295. Table 的 Row Hover、Column Hover 和 Cell Selection 必须作为独立 Overlay 叠加，由 Runtime API 声明目标范围和激活关系。悬停某个 Cell 时可以同时激活 Row 与 Column Overlay；Cell Selection 不因 Hover 清除。Recipe 可以分别提供 `row-interaction`、`column-interaction` 和 `cell-selection` 的视觉 Token，但 Token 不保存触发关系、业务动作或目标集合；Hover 只提供视觉反馈，不直接执行行级或列级业务操作。

296. Table Action 必须通过 Runtime API 显式声明目标范围，支持 `table`、`header`、`column`、`row`、`cell`、`selection` 六类 Target。`table` 使用 `table-action-group`，`header` 与 `column` 使用 `header-action-group`，`row` 使用 `row-action-group`，`cell` 使用 `cell-action`，`selection` 使用 `selection-action-group`。Column Action 只能通过表头 Action Slot 承载，不能由 Column Hover 自动触发；Row Action 绑定稳定 `recordId`，Selection Action 绑定当前选择集合。

297. Table 的 Sort 与 Filter 是独立的 Header Action。Sort 支持 `none`、`single`、`multi`，默认只作用于业务数据列的叶子表头，并依据底层值比较；Filter 支持 `none`、`column`、`table`，由 Runtime Query 管理。表格筛选、整表搜索和编辑器候选搜索必须保持独立，不得因复用搜索控件而混淆数据作用范围。

298. Table 的 `loading`、`empty`、`no-match`、`error` 均替换 Body 区域，并分别使用 `loading-state`、`empty-state`、`no-match-state`、`error-state` Slot；当列结构已知时 Header 默认保持显示，Footer 是否显示由 Runtime API 决定。状态 Slot 的颜色、排印和最小高度由 Table Recipe 管理，Retry 等动作继续通过 Runtime Action 注入。

299. Table Column 必须通过独立 Schema 定义，至少包含稳定 `id` 与 `role`，可选包含 `field`、`display`、`editor`、`editable`、`required`、`formatter` 和 `slots`。`display.type`、`editor.type` 与 `contentLayout` 相互独立；展示、编辑和 Cell Action 分别进入 `cell-content`、`cell-editor`、`cell-action` Slot。字段绑定、列配置、编辑能力和 Slot 注入由 Runtime/Component Contract 管理，不生成 Table 视觉轴。

300. Table Overlay 必须按属性类型分别解析。`loading`、`empty`、`no-match`、`error` 等结构状态先替换 Body；背景从 Base、Striping、Hover、Selection、Availability 到 Data Status 逐层解析；边界从 Base、Focus、Cell Selection 到 Data Status 解析。`data-status.error` 具有最高语义视觉优先级，可以覆盖 Disabled、Selection、Hover 和普通边框；该优先级保存在 Contract/Registry，不写入单个 Token 值。

301. Table Action 的可见性、可用性、禁用原因和执行状态由 Runtime API 管理。`visible=false` 时不渲染；`visible=true` 且 `enabled=false` 时保留位置但禁止交互；执行状态统一为 `idle`、`pending`、`success`、`failed`。`pending` 锁定声明的 Action Target，`failed` 保留当前数据和编辑草稿；Button、Spinner、Tooltip 和 Error Feedback 继续复用各自组件 Token，不建立 Table Action 专属视觉轴。

302. Table 的纵向和横向滚动条必须复用公共 `scroll-bar` 组件及其 Token，Table 不得复制 `thumb`、`track` 或 `line-button` 的视觉值。滚动条显隐策略、内容尺寸、滚动位置和滚动所有权由 Runtime API 管理；Table 只声明滚动容器与公共 ScrollBar 的组合关系。

303. Table 的 Figma 结构与变量绑定以 `source/figma-bindings/table.json` 为权威规范。公开身份只有 `ComponentLab/Table`，视觉 Variant 仅保留 `density` 与 `striping`；Header Group、Header Leaf、Cell Content、Cell Editor、Action Group 和数据状态均作为嵌套结构或可替换 Slot。Figma 变量使用 `DSM/Component` 集合和 `table/` 前缀，Canonical Token 使用 `component.table.` 前缀。该 Manifest 只描述结构和绑定，不代表已经写入或修改 Figma 文件。

304. 所有进入公共设计系统的组件最终都必须采用统一的 Component Contract 文件模型。每个公共组件的目标源包由 `component-registry.json`、`source/component-contracts/{component}.json`、`source/component-recipes/{component}.json` 和 `source/figma-bindings/{component}.json` 组成；这四类文件分别负责组件登记与视觉契约、结构与运行时契约、视觉设计值、Figma 结构与绑定。未迁移组件可以暂时缺少后两类或保留旧资料，但不得将该暂态误认为最终架构。

305. Component Contract 的统一是职责模型统一，不是数据字段同构。Registry 仍是组件轴、合法组合、Slot 名称和 Slot 属性的唯一权威来源；Contract 不重复声明这些内容，也不保存解析后的 Token 值。Contract 负责组件 Anatomy、结构关系、状态层、组合规则、运行时边界、生命周期、键盘行为、校验与提交约束，并通过 `registryBindings` 引用当前 Registry。

306. Component Contract 必须具备统一的最小结构：`component`、`version`、`$status`、`contractKind`、`registryBindings`、`structure`、`stateLayers`、`compositionRules` 和 `tokenBoundary`。Table 可以在此基础上增加列职责、展示类型、编辑器类型、数据窗口和数据生命周期等深层契约；Button、Input、Select、Checkbox、Radio 等组件不得为了满足统一校验而机械增加 Table 专属字段。

307. Button、Input、Select 的独立 Contract 首批落盘于 `source/component-contracts/button.json`、`source/component-contracts/input.json` 和 `source/component-contracts/select.json`。Button Contract 负责 Intent/Appearance/State/Content 的结构和 Loading、Focus、Disabled 语义；Input Contract 负责 Mode、Action Slot、Multiline、Resource Picker、Loading 和 Error 优先级；Select Contract 负责 Selection/Searchable/Open、Popup、Option Item、Tag、批量控制和选择校验。具体视觉值仍只允许写入各自 Recipe。

308. Component Contract 的深度按组件语义决定：基础组件使用轻量结构与状态契约；Button、Input、Select、Checkbox、Radio 等交互组件必须记录 Slot 资格、交互状态和运行时边界；Table、Tree、Navigation 等复合数据组件可以记录数据模型、编辑、选择、生命周期和 Runtime API。统一的是权威来源、物理分离、校验和生成流程，不是每个组件的轴集合或字段数量。

309. Component Compiler 必须校验所有独立 Contract：文件中的 `registryBindings.source`、`registryBindings.axes`、`registryBindings.slots` 必须与 Registry 完全一致；Registry 必须登记对应 `configuration.contract`；通用 Contract 缺失、引用不一致、结构类型错误属于阻断性 Error。只有 `contractKind=data-table` 时，才额外要求 Table 的列职责、内容展示类型和编辑器类型段落。

310. draft-v2 Contract 与 Figma Binding Manifest 的结构落盘不自动触发 Figma 写入或 Vue/Qt/QSS 发布；真实同步必须经过相应发布切片评审。

311. Button、Input、Select 的 Figma Binding Manifest 分别位于 `source/figma-bindings/button.json`、`source/figma-bindings/input.json` 和 `source/figma-bindings/select.json`。Manifest 的 `variantAxes` 必须与 Registry 完全一致，变量集合统一使用 `DSM/Component`，Canonical 前缀分别为 `component.button.`、`component.input.` 和 `component.select.`；Figma 组件的完整组合由生成器管理，不要求设计师手工维护完整矩阵。

312. Component Compiler 必须对 `draft-v2` 且 `writePolicy=specification-only` 的 Figma Binding Manifest 执行自动校验，包括组件身份、文件名、Variant 轴和值、变量集合、变量前缀、嵌套结构和绑定规则。Manifest 结构错误、Registry 轴不一致和命名空间不一致属于阻断性 Error，但校验过程不得调用 Figma 写入能力。

313. Checkbox 与 Radio 也必须采用独立 Component Contract。Checkbox Contract 负责 `value`、`state`、`content`、`size`、三态指标、键盘焦点、禁用保值和共享交互区域；Radio Contract 负责 `value`、`state`、`size`、键盘焦点、禁用保值和 Radio Group 运行时选择。两者不得因为视觉结构相近而合并为同一个公共组件或复制 Button 轴。

314. 核心组件目标源包登记 registry、独立 contract、recipe 与 figmaBindingManifest；各组件迁移状态读取当前源，不由阶段完成列表推断已写入 Figma 或已进入运行时发布。

315. Menu 必须分为三层建模：`Menu Surface`、`Menu Atom` 和 `Runtime Action Model`。`Menu Surface` 是公共设计组件 `ComponentLab/Menu`，负责命令菜单弹层的表面和整体组合；`Menu Atom` 是 UCD 可独立管理、评审和维护的内部受治理结构，负责 MenuItem 行的图标、内容、快捷键、Check/Radio 指示器和子菜单提示；`Runtime Action Model` 由 Qt 的 `QMenu`、`QAction`、`QActionGroup` 及其适配层负责，保存命令身份、可用性、勾选值、快捷键、子菜单关系、弹层生命周期和触发事件。三者不得合并成一个 Token 层或一个运行时对象。

316. Menu Atom 的“独立管理”不等于“独立公共组件”。UCD 可以对 Menu Atom 建立独立的内部 Figma 资产、视觉检查和变更记录，但业务侧仍通过 `ComponentLab/Menu` 的结构 Slot 使用它；Menu Atom 可以复用公共 Typography、Icon 和 Spacing 等基础/共享 Token，但不得继承 Button 的组件语义、Intent 或交互行为。

317. Menu 的设计侧只表达 Menu Surface 与 Menu Atom 的视觉结构和有限状态样例；Action 文本、快捷键、启用/禁用、Check/Radio 值、Submenu 层级和命令触发均属于 Runtime Action Model。Figma 中的 `MenuItem` 保持内部解剖结构，不作为与 Menu 并列的业务公共组件；Token 不保存 QAction 身份或业务动作。

318. Menu 的设计到 Qt 映射必须经过明确的 Adapter：`Menu Surface → Z_Menu/QMenu`，`Menu Atom → QAction 的可视化菜单项`，`Check/Radio → QAction/QActionGroup`，`Submenu → QAction 关联的子级 QMenu`。Adapter 只负责结构、Token 和平台视觉能力的适配，不得夺取 Qt Runtime 对动作状态、键盘导航、弹层生命周期和事件触发的权威。

319. Menu 的交互 State 统一采用已确认的公共状态词汇：`default`、`hover`、`pressed`、`focused`、`disabled`、`loading`。`selected`、`emphasis` 和 `none` 不得作为 Menu State；Check/Radio 的已勾选值由 Runtime Action Model 的 `QAction` 数据管理，语义强调应通过后续明确的语义属性或视觉 Token 表达，`none` 不得用于伪造状态或结构值。

320. Menu 状态必须按作用范围分层：`default`、`hover`、`pressed`、`focused`、`disabled` 仅作用于 Menu Atom 的菜单项视觉；`loading` 仅属于 Runtime Action Model 的动作生命周期，由 Qt Runtime/Adapter 控制锁定、反馈和必要的内容替换，不得成为 Menu Surface 的整体 State，也不得因此新增 Menu Surface 的笛卡尔积。Menu Atom 可以提供承载运行时反馈的结构 Slot，但具体是否显示及其行为由 Runtime 决定。

321. Menu Atom 固定包含 `icon`、`content`、`shortcut`、`indicator` 和 `submenu` 结构 Slot，Menu Surface 另外包含 `container`、`separator` 和 `border`。这些 Slot 的结构可以由 UCD 独立管理，但不自动生成新的组件轴；`indicator` 复用 Checkbox/Radio 的视觉 Token，`icon` 与 `submenu` 复用公共 Icon Token，`shortcut` 复用 Typography 语义，只有 `separator` 的颜色、厚度和内缩作为 Menu 自有视觉 Token。Slot 是否实际出现由 Runtime Action Model 的 Action 数据决定。

322. Menu Atom 的右侧视觉轨道按 `content → icon → indicator` 顺序组织：`indicator` 永远位于最右侧；存在 `indicator` 时，`icon` 紧邻其左侧；不存在 `indicator` 时，`icon` 占据最右侧位置。没有 `icon` 时必须保留 Icon 空位，Content 使用固定对齐列，确保不同菜单项的文字始终对齐。该规则属于 Menu Atom 的结构布局 Contract，不生成新的 State 或 Type 轴。`submenu` 与右侧轨道的优先级另行定义。

323. Menu Atom 必须保留稳定的 `decorationArea`。该区域不是 Icon 必填位，而是可容纳 `icon`、Check/Radio `indicator` 及后续受治理装饰内容的固定结构区域；区域为空时仍保留其宽度，Content 不得因装饰内容有无而横向移动。`decorationArea` 属于布局结构，不是新的 Token 轴、State 或 Type。

324. `indicator` 与 `submenu` 不得在同一个 Menu Atom 中共存。Indicator 模式的右侧轨道为 `content → icon → indicator`，Submenu 模式为 `content → icon → submenu`，普通模式为 `content → icon`；三种模式都必须保留稳定的 decorationArea 和 Content 对齐列。该互斥关系由 Registry/Contract 自动校验，不通过 Figma Variant 穷举。

325. Menu Atom 的 `shortcut` 必须位于 Content 与终止装饰之间，形成固定、右对齐的 Shortcut 列。没有快捷键时保留列宽，避免 Icon、Indicator 或 Submenu 的位置随内容变化；Shortcut 只是 Runtime Action Model 的展示数据，不是独立交互控件、State 或 Type 轴。实际快捷键绑定和触发继续由 `QAction::shortcut` 管理。

326. Menu 的 `separator` 是 Action Group 之间的结构分隔，不是 Menu Atom，也不代表 QAction。它只允许出现在两个 Action Group 之间，不得出现在首项或末项；不参与 Hover、Pressed、Focused、Disabled、Loading、Shortcut 或键盘焦点。Separator 不占用 Menu Atom 的 decorationArea 和 Content 对齐列，其颜色、厚度和内缩由 Menu 自有 `separator` Token 管理，是否出现由 Runtime Action Model 的分组结构决定。

327. Qt 5 `QMenu` 原生提供 `addSection()`、`addSeparator()` 和 `title` 能力，但 `addSection()` 的文字提示是否实际渲染由 Qt Style/平台决定，`QMenu::title` 也不等同于弹层内部的可见分组标题。本项目当前不建立 `Menu Header` 设计 Slot、Token 或公共组件；Menu 只使用稳定的 `Separator` 表达 Action Group 边界。未来若业务确需可见分组标题，作为 Runtime/Adapter 扩展单独评审，不回填为 Menu Atom 或新的设计轴。
328. `QMenu::title` 仅作为 Runtime Action Model 的菜单身份数据，由 Qt Runtime/Adapter 保存和传递；它不得生成 Menu Header 的视觉结构、Figma Slot、Component Token 或公共变体。`Separator` 继续作为唯一已确认的设计侧分组边界。
329. 组件 Token 必须区分两种物化策略：设计侧采用 `sparse`，运行时采用 `expanded`。Figma/Pixso 只承载 Primitive/Semantic 引用、Component Base、必要的稀疏覆盖、Composite 引用以及已批准的结构/Slot 绑定；`generated/component.json`、`generated/component-combinations.json` 和运行时合法组合展开结果不得直接导入为设计变量。
330. Registry 中的轴、合法组合和 Slot 是逻辑模型，不等于 Figma/Pixso 中必须逐项建立的变量。组件生成器可以为 Qt/Vue/CSS 展开完整合法组合，但设计侧通过 Base、Alias、Slot 和少量覆盖表达同一组决策，避免把笛卡尔积复制为变量或人工维护的完整变体集合。
331. Figma 设计变量集合不设全局“必须压到多少个”的项目目标线。`Registry + Contract + Recipe` 决定设计侧应物化哪些 Base、Composite、Slot 和必要 Override；Figma/Pixso 的实际变量数量只作为外部审计证据，不得反向修改 V2 源模型。5000 个只是 Figma 单个 Collection 的平台硬上限，超过才产生阻断性 Error；该上限不约束 Runtime 生成产物的逻辑 Token 数量，也不意味着项目应主动建设到 5000 个。依据：[Figma Variables and Collections](https://help.figma.com/hc/en-us/articles/15145852043927-Create-and-manage-variables-and-collections)。
332. 数量必须分开统计：设计侧稀疏 Token、Figma Component Set 变体和 Runtime 展开叶子。历史 Button 数量只作为归档迁移证据；当前数量必须由 Registry + Contract + Recipe 重新计算并分别验收，不能反向用现存 Figma 数量定义合法模型。
333. Button 的现有 Figma 组件必须先标记为 `migration-evidence-only`，暂停继续绑定、补映射或删除。因为它存在 `dark` Intent、`operation/production`、`pressed/focused` 与 `active/focus-visible`、`small/medium/large` 与 `xs` 等历史漂移，且无法证明是由 Registry 合法组合规则生成的设计子集。必须先由 Registry + Contract + Recipe 定义合法模型，再确定 Figma 哪些轴物化为 Variant、哪些能力使用 Component Property/Instance Swap，之后在隔离候选区重建 Component Set，最后进行稀疏 Token 绑定和开发一对一映射。
334. Button Figma 重建的顺序固定为：`Registry + Contract + Recipe → Figma 物化规则 → 规范 Component Set → 稀疏 Token 绑定 → 开发映射`。禁止采用“现有不确定变体 → 强行绑定 Token → 固化开发映射”的逆向流程。Theme/Profile 不进入 Button Variant；Content 优先使用文本、Boolean 和 Instance Swap 属性，不机械创建五套 Content 变体。
335. Button 设计变量的过度展开诊断必须记录变量数量与实际解析值数量的差异。当前证据包括：Label 636 个变量对应 13 种解析值，Content 108 个变量对应 8 种解析值，Icon 42 个变量对应 3 种解析值。相同解析值应通过共享 Base、Alias 或必要 Override 复用，不得因为组件变体不同而复制为组合级 Slot Variable。
336. Pixso 不预设未经实测的固定变量上限。Pixso 同步器必须在发布前读取目标工作区的实际容量或执行最小导入探测；容量未知时只能报告 `capacity-unverified`，不得宣称“可承载上万变量”。设计源超过已验证预算时，优先减少物化变量；只有在完成源端裁剪后仍有必要，才按稳定职责拆分变量集合。
337. 设计变量集合的拆分不得以“组件逻辑组合数”或“生成文件大小”为依据机械拆分。拆分只能发生在已经裁剪后的稳定职责边界，例如 Primitive、Semantic、Typography Composite、Component Base/Overrides；拆分后必须保留跨集合 Alias、命名空间、版本和回溯关系。生成产物与 Figma/Pixso 设计源物理分离，任何设计侧修改必须回写 Registry/Recipe/Composite 源后重新编译。
338. Menu 的 Surface 与 Atom 状态必须分离：`container` 只保存菜单表面的背景、渐变、圆角、内边距和边界；`item` 保存菜单项的 Hover、Pressed、Disabled 等行级视觉覆盖。不得把 Menu Atom 的交互背景继续写入 `container`，否则会导致悬停或禁用一个菜单项时错误改变整个 QMenu 表面。
339. Button 必须保留独立的 `Appearance` 外观材质轴，但不得把 `full`、`filled`、`outline`、`outlined`、`ghost`、`text` 等同义词并列写入 Token、Registry 或 Figma Variant。活动源只保留一套规范名称：`filled`、`outlined`、`dashed`、`filled-inner-shadow`、`text`、`link`；产品文案别名只允许存在于展示层或迁移说明中。`default` 保留给 `State`，不得作为 Appearance 名称。
340. Button 的 Intent 与 Appearance 通过角色解析组合，不通过组合级 Override Token 复制结果：Appearance 定义材质和所需颜色角色，Intent 提供 `on-filled`、`on-surface` 等语义角色，State 提供交互状态变化。合法的 Intent × Appearance 组合仍由 Registry 管理，但其解析结果不得物化为 Figma 设计变量。
341. 组件 Token 采用“语义最小化”原则：Token 数量应在不丢失稳定设计决策的前提下尽可能少。相同语义通过 Base、Alias 或 Composite 复用；只有当某个差异同时满足“产生真实视觉差异”“需要独立治理或绑定”“不属于运行时组合结果”时，才允许新增 Token。Token 数量是审计结果，不是脱离组件模型预先规定的项目目标。
342. 阴影、渐变、模糊、Typography、Spinner 和动效参数等复合视觉特征必须由独立 Composite 管理。组件层只保存对标准 Composite 的引用，或保存一个可被编译器解析的 Composite 角色，不得在每个组件、Intent、Appearance 或 State 下重复拆解复合参数。Figma 可以使用本地样式与变量协同表达，开发侧由 Composite 配置解析为平台所需的具体参数。

343. V2 Token 架构正式采用七个逻辑层，并固定依赖方向：`01 Primitive → 02 Profile → 03 Theme → 04 Semantic → 05 Composite → 06 Component → 07 Content`。其中 Primitive 保存原子值，Profile 保存平台/分辨率/密度适配，Theme 保存主题与表面环境，Semantic 保存跨组件语义角色，Composite 保存可复用的字体/渐变/阴影/模糊/动效等复合视觉特征，Component 保存组件专属视觉契约，Content 只保存术语、中文化/英文化文本及多语言国际化适配内容。

344. 七层是逻辑架构，物理迁移按层进行。当前编译器保留生成的 `component.json` 兼容投影；Theme 已迁移到独立的 `source/theme.json` 并由加载器直接读取，Composite 继续以独立包保存，Content 的目标源目录为 `source/content/*.json`。Content 不由 Component Contract 承担，也不保存 Slot、运行时状态、数据源、事件或业务规则。兼容投影和迁移状态必须记录在 `source/v2-source-manifest.json#/authority/tokenArchitecture/compatibility`，后续迁移不得改变层依赖方向。

345. Theme 与 Profile 必须物理分层、语义分工：Profile 只处理平台、分辨率、密度及其派生的尺寸/排印适配；Theme 只处理 light/dark 等主题与表面环境。颜色主题不得因为当前仍位于 Semantic 文件而被重新归入 Profile；迁移到独立 Theme 文件前，Semantic 的主题模式值属于明确的兼容状态，不得新增第二套并行主题值。

346. Composite 是 Component 的上游，不是 Component 内部重复展开的参数集合。Component Recipe 只引用 Composite 角色；Content 不是视觉 Token 层，只承载术语和多语言实例文本，不得把 Slot 结构、图标实例、数据源、事件、运行时状态或业务规则复制成 Content 或 Component Token。生成的 `component.json`、组合展开结果和平台映射文件均属于产物，不得反向成为七层源文件。

347. 七层架构的阶段验收条件是：Manifest 登记完整、依赖顺序固定、Theme 实际模式值与 Semantic 角色通过 Alias 连接、生成产物与源物理分离、兼容投影可编译、现有 V2 测试通过。Composite 的统一解析入口和 Content 的独立数据入口仍需后续分阶段迁移；每层迁移后都必须重新生成、审计和回退验证。

348. Composite 源包必须声明 `layer=composite`，并由 `composite-registry.json` 登记唯一类型、源文件、成员契约、Figma 表达策略和运行时适配策略。当前正式 Composite 类型为 `typography`、`gradient`、`shadow`、`blur`、`motion`；每个类型必须有对应源包、匹配的 `kind` 和受治理的 `$status`。Registry 与源包不一致属于阻断性 Error。

349. Component Recipe 使用 Composite 时只能引用完整 Composite 角色，例如 `{typography.label.label-1}` 或 `{motion.loading.spinner}`；不得在组件层复制 Typography、Gradient、Shadow、Blur 或 Motion 的内部参数。Spinner 的尺寸、颜色等组件绑定仍属于组件 Slot，Spinner 的动态轨迹和动效属于 Motion Composite；两者不得混为一个组件 Token。

350. Content 的最小数据模型是“英文术语身份 + Locale + 文本值 + 英文默认回退”。Content Key 和术语身份统一使用英文；`en-US` 是默认语言，也是所有缺失翻译时的最终回退语言。中文及其他 Locale 只能作为本地化文本覆盖，不得因为语言变化复制 Component Token、Variant 或视觉值。组件只引用英文 Content Key；语言选择、回退和文本注入由国际化适配层处理，视觉 Token 不读取或保存具体业务文案。

351. Content 的首个物理入口为 `source/content/catalog.json`，其条目只允许保存 `values`、`fallbackKey` 和说明信息。`values` 的键必须来自 `supportedLocales`，`defaultLocale` 与 `fallbackLocale` 固定为 `en-US`；Content Catalog 不得保存 Slot、组件状态、数据源、事件、业务规则、视觉 Token 或运行时控制参数。Compiler 必须自动校验 Content Catalog 的层级、英文默认回退、Locale 和字段边界。

352. 组件 Registry 中的 `Content` 轴只表达组件内部内容结构，不表达具体中文或英文文本。Button 的五种 Content 结构固定为 `icon-only`、`icon-left-text`、`text-icon-right`、`text-only`、`icon-left-text-icon-right`；文本由 `labelText` 属性引用英文 Content Key，图标由 `leadingIcon`/`trailingIcon` Instance Swap 注入，显示布尔值只控制结构显隐。Theme、Locale 和具体文案不得进入 Button Variant 或 Component Token。

353. Input 不建立独立的 `Content` Variant 轴。`mode` 只表达输入能力与结构模式：`plain`、`password`、`search`、`multiline`、`dropdown-search`、`file-browser`；`valueText` 是实例内容，设计侧可放置示例文本或英文 Content Key，运行时由受控输入值接管；`placeholderText` 是英文 Content Key，缺失翻译时回退到 `en-US`。`clear`、`password-toggle`、`search-icon`、`browse-action` 和 `spinner` 的出现由 Mode/State 与 Runtime 规则决定，不属于多语言 Content。

354. Select 不建立独立的 `Content` Variant 轴。`selection`、`searchable`、`open` 和 `state` 分别表达选择模式、搜索能力、浮层状态和交互状态；`placeholderText`、`selectedValueText`、Tag 文本、Option Item 文本、批量控制文本和空状态文本属于实例内容或英文 Content Key。Multiple 的 Tag 与 Option Item 必须作为嵌套结构实例承载，不能将每个选项值生成 Component Token 或 Variant；Popup 内的临时 `searchText` 由 Runtime 保存，不写入 Content Catalog。

355. Checkbox 的 `Content` 轴只保留 `control-only` 与 `control-with-label` 两种结构。`control-only` 不产生 Label 内容；`control-with-label` 的 `labelText` 是英文 Content Key，缺失翻译回退 `en-US`。Label 与 Indicator 共享同一交互区域，但 Label 文本、Value、State、Size 和 Interaction Size 必须分别建模；Label 内容不得改变 Value 视觉 Token 的命名或结构。

356. Radio 的 `Content` 轴同样只保留 `control-only` 与 `control-with-label` 两种结构。`labelText` 是英文 Content Key，缺失翻译回退 `en-US`；Radio Group 的互斥选择、选中迁移和业务值属于 Runtime，不属于 Content。Radio 的 Label 与 Indicator 共享交互区域，但 Content 不得承载 Value、State、Size、Interaction Size 或 Radio Group 选择逻辑。

357. Table 不建立单一的 `Content` 轴。Table Content 按结构 Slot 分为 `headerText`、`cellDisplayText`、`cellEditorText`、生命周期状态文本和 Action 文本：表头、空/加载/无匹配/错误状态文本使用英文 Content Key；Cell 展示值、编辑器值和溢出完整内容属于运行时实例内容；表头、Cell、编辑器和 Action 只能通过已定义 Slot 注入。列 Schema、字段绑定、记录值、编辑器类型、选择/排序/筛选状态和业务规则不得进入 Content Catalog 或 Table Token。

358. Menu 的 Content 只保存菜单项文本和可展示的快捷键文本。`itemText` 可以引用英文 Content Key；`shortcutText` 可以来自 Runtime 的 `QAction::shortcut` 展示值；Separator 没有 Content。`QAction` 身份、触发事件、启用状态、Checked 值、Submenu 关系、Indicator 状态和 Separator 位置由 Runtime Action Model 管理，不得进入 Content Catalog、Menu Token 或 Figma Variant。

359. Tree 的 Content 只保存节点显示文本、可选辅助文本和空状态文本。节点文本可以引用英文 Content Key，也可以由 Runtime 数据提供；节点 ID、父子层级、展开/收起、选中/禁用、懒加载和键盘导航均由 Runtime 管理。层级结构和节点状态不得进入 Content Catalog、Tree Token 或 Figma Variant。

360. Content 层已批准为全组件通用规则，不要求每个组件复制一份 Content 文件。所有 Registry 组件默认继承 `source/content/catalog.json` 的英文 Key、`en-US` 默认语言和 `en-US` 最终回退；已有 Component Contract 可以声明组件专属的文本来源、嵌套实例或 Runtime 临时文本，但不得改变 Content 层的语言治理，也不得把视觉、结构、状态、数据源或业务规则纳入 Content。尚未建立独立 Contract 的组件暂按全局 Content Policy 执行，待组件迁移时再补充特化契约。
361. Tool Button 必须作为独立组件族重建，不继承 Button 的 Intent、Appearance 或 Content 轴。Tool Button 的设计模型只保留四个相互独立的维度：`presentation=standard|auto-raise` 表达表面呈现策略，`value=unchecked|checked` 表达可持久化的勾选值，`state=default|hover|pressed|focused|disabled` 表达交互视觉，`menu=none|split` 表达单动作或分裂菜单结构。当前合法逻辑组合数为 30，组合由 Registry 约束计算，不等于 Figma 必须建立 30 个变量或手工变体。
362. Tool Button 的历史复合状态不得继续进入活动源：`checked-hover` 必须解析为 `value=checked + state=hover`，`hover-pressed` 必须解析为 `state=pressed` 的交互覆盖，`auto-raise` 必须迁移为 `presentation=auto-raise`，`open` 只属于 Runtime 菜单生命周期。历史 Recipe 仅作为迁移证据，不得继续作为 Registry、Contract、Figma Binding 或开发映射的事实源。
363. Tool Button 的 `menu=split` 只表示结构上存在主动作区与次级菜单触发区；分隔线和动作区宽度是稀疏组件视觉 Token，菜单打开/关闭、QAction 身份、键盘导航和事件触发均由 Runtime API 管理。`presentation=auto-raise` 与 `menu=split` 当前禁止组合，避免把两个互相冲突的结构/表面策略隐式叠加。
364. Tool Button 的 Figma 物化采用 `Registry + Contract + Recipe → Figma Binding Manifest → Component Set` 顺序。Label 和 Icon 使用文本/Instance Swap/Boolean 属性，不新增内容轴；Figma 当前不绑定未经审计的旧 Tool Button 变体，也不把 Runtime 的 `open` 状态写入设计变量。新模型落盘于独立 Contract、Recipe 和 Binding 文件，旧模型应移入 history 后再执行任何 Figma 迁移。
365. 七层适配不是为每个组件复制七套 Token，而是登记 Primitive/Profile/Theme/Semantic/Composite/Content 与 Component 的参与关系。Recipe 是组件视觉源，Runtime 不属于 Token 层；哪些组件完成适配以当前 Manifest 和独立 Contract 的证据核对，不维护第二份阶段名单。
366. 组件七层适配记录必须明确五类边界：Theme 通过 Semantic Alias 提供主题角色，Profile 提供尺寸/密度/交互尺寸，Composite 提供 Typography/Gradient/Shadow/Blur/Motion 等复合引用，Content 只提供英文默认及多语言实例内容，Runtime 保留状态生命周期、数据、事件和业务规则。该记录位于 `source/v2-source-manifest.json#/componentLayerBindings`，由 Compiler 自动校验；它是依赖登记，不是第二个 Token 值源。
367. Component Recipe 可以引用下游所需的 Primitive 或 Semantic 原子值，但不得把 Theme 模式值、Composite 内部参数或 Content 文案复制进组件层。Theme 的颜色必须经由 Theme → Semantic Alias 解析；Typography、Motion 等复合视觉特征必须经由 Composite 契约或受治理角色引用；Content Key 不得成为视觉 Token。缺少某层的专属值不代表缺少七层适配，必须记录该层的引用路径或“不拥有值”的边界。
368. 三项目 Source 仅保留维护实现与必要输入；固定规范、样例、测试保存在 docs；全部生成输出在 build。历史报告、旧设计证据、Raw Token 和旧组件模型统一进入 docs/archive，归档原文不改写。build/reports 是当次可再生输出，需永久保留的结果先归档。例外仅为 PrepareBuild 显式登记的归档审计只读验证输入；不得把它们作为 Token 值源。
369. 组件到 Qt/Vue 的基础映射必须单独保存在 Runtime Mapping Manifest，不得写入 Component Recipe 或 Token。映射记录 `Registry/Contract/Recipe → Adapter → 平台类或组合结构`；Chart 的 `line/bar` 复用同一绘图类，`presentation=panel` 映射为容器、标题 Slot、Chart 和关闭动作 Slot 的组合，不假设不存在的独立平台类。映射状态为 `contract-pending` 时只能作为待实现登记，不得宣称已经完成平台适配。
370. 父组件与子组件采用分层映射：`TabWidget → QTabWidget/Z_TabWidget`，其 `tab-bar → QTabBar`、`tab-item` 和 `content-stack` 作为内部 Anatomy/子结构；`tab-item` 不提升为独立公共组件。Navigation Bar 的 `tabs` Slot 复用 `TabWidget.tab-bar` 映射，不建立并列的 Tabs Contract、Token 或 Qt 公共类；Menu Item 仍属于 Menu Atom，不能与 Tab Item 合并。
371. Side Menu 的公共身份为 `Sidebar → commonWidget::Z_Sidebar`；其内部导航项统一命名为 `Sidebar/NavItem`，Qt 侧映射为私有 `QToolButton`（`sidebarRoute=true`）并由 `QButtonGroup` 管理互斥路由。不得创建 `Z_SidebarItem` 公共 SDK 类，也不得将其命名或映射为 `MenuItem`；`MenuItem` 继续保留给 `QMenu/QAction` 命令菜单语义。路由 ID、标签、当前路由和跳转事件由 Runtime 管理，NavItem 只承载结构与视觉。
372. Side Menu 的已确认内容结构为 `brand-header + account + section/nav-group + submenu + footer`。展开模式下 Submenu 内嵌显示；折叠模式下 NavItem 只显示 Icon，Submenu 通过公共 Popover/Menu 浮层显示，完整 Label 通过 Tooltip 可访问。`expanded/collapsed` 是 Runtime 布局模式，不进入 State 轴；Container 仍保留 `width/min-width/collapsed-width` 等视觉 Token。
373. Switch 作为二值设置控件独立建模，不与 Checkbox 合并：`value=off|on` 表示受控设置值，`state=default|hover|pressed|focused|disabled|loading` 表示交互/可用性状态，`size=small|medium` 表示轨道与旋钮视觉尺寸。`value` 与 `state` 必须分离，不能用 `checked` 代替 Switch 的值，也不得把 on/off 做成 State。
374. Switch 的结构 Slot 固定为 `track`、`knob`、可选 `label`、`focus-ring`、`loading-spinner` 和 `content`。Label 与 Switch 共享同一交互区域，不产生独立焦点目标；Label 文本属于 Content 层，Label 视觉属于 Component Recipe。
375. Switch 的 `disabled` 与 `loading` 均保留当前 value 并锁定交互；`loading` 只替换 Knob 内部内容为共享动态 Spinner，不新增 Value 或组件类型。Spinner 的尺寸、颜色、线宽、起止角和弧长引用公共 Spinner 视觉/动效 Composite，播放生命周期由 Runtime 控制。
376. Switch 的 `focused` 仅表示键盘焦点，焦点视觉采用独立的 border-only Focus Ring；鼠标 Hover/Pressed 不激活 Focus Ring。`hover` 与 `pressed` 只覆盖 Track 的对应 Value 视觉，不复制完整组合级 Token。
377. Switch 的 Qt 映射为 `Switch → commonWidget::Z_Switch → QAbstractButton`。Runtime 负责 value mutation、键盘/指针事件、禁用权限、Loading 生命周期和 Spinner 播放；Component Recipe 只负责 Track、Knob、Label、Focus Ring 和 Spinner 的视觉引用。现有 `Z_DesignComponents.cpp` 中的硬编码 Hover/Pressed 颜色及 Spinner 弧线属于迁移警告，发布前必须改为读取编译后的 Token。
378. `commonWidget::Z_Switch` 不得描述为 Qt Widgets 原生 `QSwitch`：Qt 5 Widgets 没有原生 Switch 控件。本项目采用自定义绘制的 `Z_Switch`，以 `QAbstractButton` 作为行为基类，复用 `checkable/checked/toggled`、键盘/指针事件和禁用能力；它不继承 `QCheckBox`，避免把 Checkbox 的选择语义和原生指标带入 Switch。Qt Quick 的 QML `Switch` 属于另一套技术栈，不参与当前 Qt Widgets 映射。
379. Badge 作为非交互状态标记独立建模，不建立 `hover`、`pressed`、`focused`、`disabled` 或 `loading` State 轴。其活动轴固定为 `kind=text|dot` 与 `tone=neutral|info|success|warning|danger`；`tone` 是语义色调，不是交互状态，`kind` 是展示结构，不是业务类型。
380. Badge 的结构 Slot 固定为 `container` 与 `label`。`kind=text` 渲染运行时 Label，`kind=dot` 不渲染 Label；Label 文本属于 Content/Runtime 实例内容，不得生成按文案复制的 Component Token。Badge 的圆角、尺寸、内边距、文字颜色和 Typography 由 Recipe/Composite 管理。
381. Badge 的 Qt 映射为 `Badge → commonWidget::Z_Badge → QWidget`，采用自定义绘制，不映射为 `QAbstractButton`、`QCheckBox` 或其他可交互控件。Runtime 只负责 label、kind、tone、可访问名称和内容测量；Badge 不承担点击、焦点、事件触发或业务状态生命周期。
382. Badge 的 `tone` 颜色必须引用 Semantic 状态/交互角色：`neutral`、`info`、`success`、`warning`、`danger`；相同 tone 的 Text 与 Dot 复用同一语义颜色，不复制两套色值。Dot 与 Text 的几何差异通过 kind 覆盖表达，不将运行时文本宽度物化为 Token。
383. Chip 作为可选择控件独立建模，不与 Badge 或可关闭 Tag 合并。其活动轴固定为 `orientation=horizontal|vertical`、`value=unselected|selected` 和 `state=default|hover|pressed|focused|disabled`；`horizontal/vertical` 不得继续登记为 State，`value` 不得并入 State。
384. Chip 复用 Qt `QAbstractButton` 的 `checkable/checked/toggled` 行为，并由 `commonWidget::Z_Chip` 自定义绘制。它不是 Qt Widgets 原生 Chip，也不应改为 `QCheckBox`；Qt 基类只提供选择行为，Chip 的表面、方向布局、焦点环和禁用视觉由 Component Recipe 管理。
385. Chip 的结构 Slot 固定为 `container`、`label` 和 `focus-ring`。Label 是 Runtime/Content 文本；Chip 当前不增加 `loading`、`dismissible` 或关闭动作轴，关闭能力继续由 Tag 的独立 Runtime Contract 负责。`focused` 仅表示键盘焦点，焦点视觉采用独立 border-only Focus Ring。
386. Chip 的 `value` 与 State 分层解析：`unselected/selected` 提供基础表面和文字角色，`hover/pressed/focused/disabled` 只提供稀疏覆盖；Disabled 保留当前 Value 并锁定交互。Orientation 只改变结构几何，不复制 Value × State 的完整设计变量矩阵。
387. `Container` 不作为公共 Component Contract 或 SDK 组件建立。它是内部结构能力，使用 `QWidget` 或需要边界绘制时使用 `QFrame` 承载；其背景、内边距、圆角和 Content Gap 等视觉值可以继续由兼容 Recipe 保存，并由 Card、Table、Sidebar、Navigation Bar 等父组件通过结构 Slot 引用。不得创建 `ComponentLab/Container` 或 `commonWidget::Z_Container`，也不得为通用容器新增 Runtime Mapping。
388. `NumberInput` 是唯一的公共数字输入组件身份；`SpinBox` 与 `DoubleSpinBox` 只作为 Qt 编辑器适配器，不得分别建立两套组件级视觉 Token。`type=integer|decimal` 属于 Runtime 能力选择，不能复制视觉 Token；`controls=plus-minus|up-down|split-plus-minus|none` 才表达可见步进结构。其 Slot 固定包含 `container`、`field`、`stepper`、`increment`、`decrement`、`divider`、`icon`、`focus-ring` 和 `loading-spinner`。
389. NumberInput 的根边框必须是连续容器边框；按钮 Slot 必须同时作为完整视觉区域和命中区域，不能只绘制半包围边线或依赖彼此独立的硬编码矩形。`split-plus-minus` 的左右按钮与 Field 通过共享分隔线分区，`plus-minus/up-down` 的上下按钮通过同一分隔线分区；按钮之间不得出现不可解释的 1px 空洞，也不得重复绘制外框。
390. NumberInput 的 `hover/pressed` 覆盖只作用于当前 `increment` 或 `decrement` 子控件，并且绘制区域必须与 Qt `hitTestComplexControl()` 使用同一套 Slot 几何。`focused` 只绘制容器级 border-only Focus Ring；`readonly` 视觉别名为 `disabled` 并锁定编辑，`error` 与 `loading` 同样锁定交互，Loading 通过共享 Spinner 视觉/动效引用表达。数值、最小值、最大值、步长、精度、Locale 和分组格式均由 Runtime API 管理，不进入组件 Token 轴。
391. `Statistic` 是 Data Display 的唯一公共组件身份；历史 `DataDisplay` 只保留为迁移别名，不得建立并列公共组件。Statistic 只表达一个数据值展示结构：`size=compact|default`、`state=default|loading|empty|error`，以及 `title`、`value`、`prefix`、`suffix`、`feedback` 等 Content/结构 Slot。它不是 Card、Chart、Table、数据集容器，也不承担排序、筛选或聚合逻辑。
392. Statistic 的数值、Locale、精度、分组格式、空值和错误生命周期由 Runtime API 管理；Component Recipe 只保存容器间距、标题/数值/前后缀 Typography 与颜色、Loading Skeleton 和 Feedback 的视觉引用。Loading、Empty、Error 不复制一套新的 Value Token，未声明的视觉属性从共享 Base 继承。Qt 映射为 `Statistic → commonWidget::Z_Statistic → QWidget`，发布前仍需将运行时绘制中的字面视觉值迁移为编译后 Token 读取。
393. 日期时间输入统一采用 `DateTimeInput` 公共身份，`mode=date|time|datetime` 由 Runtime 选择 `QDate/QTime/QDateTime` 值类型及对应原生编辑器适配器；`DatePicker`、`TimePicker`、`DateTimePicker` 只保留为历史别名或内部适配命名，不得分别复制组件 Token。Qt 原生控件不作为最终视觉事实源。
394. DateTimeInput 的字段和弹层必须采用项目自有组合：`container`、`field`、`calendar-action`、`spin-action`、`divider`、`popup-surface`、`calendar-header`、`weekday-row`、`day-grid`、`day-cell`、`nav-button`、`action-button`、`range-overlay`、`focus-ring` 和 `loading-spinner`。Figma 证据节点为 `195:935`、`194:606`、`193:361`、`193:614`。
395. 日历弹层是 DateTimeInput 的私有结构，不另建并列公共 `Calendar`/`DatePicker` 视觉组件。DayCell 的 `default/hover/pressed/focused/selected/selected-start/selected-middle/selected-end/today/disabled/out-of-month/in-range/hover-preview-range` 是内部状态模型；范围选择、月份导航、弹层开关、候选日期和校验由 Runtime API 控制，不物化为 DateTimeInput 的组件轴。
396. `focused` 继续采用 border-only Focus Ring；`readonly` 复用 disabled 视觉并锁定编辑；`error` 与 `loading` 同样锁定交互。Recipe 负责字段、弹层、日期单元格、范围高亮、导航按钮和操作按钮的视觉 Token；Qt 原生编辑器只负责值、解析、格式和键盘步进能力，最终渲染由计划中的 `commonWidget::Z_DateTimeInput` 自定义复合组件承担。
