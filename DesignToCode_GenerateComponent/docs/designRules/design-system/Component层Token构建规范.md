# Component 层 Token 构建规范

> **V2 架构生效声明（2026-10-17）**：本文后续的 V2 规则是 Component 层 Token 的目标规范，也是新的唯一约束来源。旧版 sToken 结构、旧版 Component 路径和历史 Figma 变量组仅作为迁移证据，不再作为新设计或新 Token 的约束。V2 属于必要的重大架构纠错，不是对旧结构的兼容性扩展。

## 0. V2 已确认的架构决策

### 0.1 权威来源与边界

1. Component Registry 是组件层轴、合法组合、组件槽位和 Token 生成边界的权威来源。
2. `generated/component.json` 是 Registry 约束下的下游 Token 产物，不得另行发明组件轴或合法组合。
3. Primitive、Profile、Semantic、Product Application Rollout 分别保留自己的作用域；Product Application Rollout 保存在各产品专用配置/变量组包中，与公共设计系统变量组分离。
4. 设计师和开发者通过各自的组件库消费合法组件组合；组合复杂度由 Registry、生成器和编译系统承担，不要求使用者手工维护完整笛卡尔积。
5. Registry 与稀疏 Component Token Recipe 是组件层可编辑的设计决策源；两者必须物理分离为不同文件，不得合并到同一个文件，也不得通过同文件内的多个顶层字段模拟分离。Figma 变量、Runtime Token、Vue/CSS Token 和映射文件均属于解析生成产物，不得直接作为新的设计决策源修改。
6. 系统上线流程必须是“修改 Recipe → 自动 Registry 校验 → 解析生成 → 自动检查 → 发布消费产物”。新产品或创新方案在进入公共设计系统前必须经过设计系统评审；产品专用方案优先保存在 Product Application Rollout 专用配置中。
7. Component 层目标文件布局为：

   ```text
   DesignSystem/source/component-registry.json   ← Registry
   DesignSystem/source/component-recipes/        ← Recipe
   DesignSystem/generated/component.json         ← 解析生成产物
   ```

   现有 `source/component.json` 应迁移为生成产物，不再作为新的人工设计决策源。
8. 当前阶段只确认 Component 层 Token 设计与落地方案；在矩阵生成器和 Registry 契约明确前，不手工扩写完整 Button Token 矩阵。

### 0.2.1 Component Recipe 的定位

`component-recipes/` 是组件配方源目录（Recipe Source Package），承担设计系统中台的结构化设计决策职责：

- 它是可评审、可版本管理、可被解析器读取的机器可读源数据，不是普通说明文档；
- 它不是运行时服务，也不是 Figma 变量导出文件；
- 每个组件使用一个独立 Recipe 文件，例如 `component-recipes/button.json`；
- Recipe 保存该组件实际采用的共享值、引用关系、组合级覆盖和必要的决策元数据；
- Registry 保存组件契约、轴、合法组合和 Slot 定义，Recipe 不重复声明这些契约；
- Recipe 不保存解析后的完整矩阵，完整矩阵由生成器根据 Registry 和 Recipe 计算得到；
- 产品专用配置仍属于 Product Application Rollout，不直接写入公共 Component Recipe。

Recipe 内部使用两个逻辑区域：

```text
shared      ← base/{axis} 共享设计决策
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
| Content | 表达内容结构 | `icon-only`、`icon-leading`、`icon-trailing`、`text-only`、`icon-leading-trailing` |
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

因此，`icon-only`、`icon-leading`、`icon-trailing`、`text-only` 和 `icon-leading-trailing` 在 `loading` 状态下均呈现统一的 Spinner。Spinner 使用独立的 `spinner` Slot；原有图标和文本不再作为可见内容渲染。该规则由 Component Registry 登记并由生成器/运行时适配器执行。

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
- `clear` 仅在已配置且 `value-present` 时显示；输入值为空时不显示。进入 `disabled`、`error` 或 `loading` 后，已显示的 `clear` 保留位置但置为 disabled，不提供清除交互。

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
multiline  → 不具备 password-toggle / search-icon 的显示资格
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

- 公共组件身份是统一的 `Input` 输入控件家族，覆盖单行与多行文本输入；单行模式可适配 QLineEdit，多行模式可适配 QTextEdit，但这属于平台实现差异，不拆分为新的公共 Token 家族；
- 不包含 Label、Helper Text、Error Message 或 Form.Item 等外层表单包装；
- 普通输入、密码输入、搜索输入等属于该输入控件内部需要继续审计的能力/类型差异；
- 普通输入、密码输入、搜索输入、多行文本输入和资源选择输入统一属于公共 `Input`，通过 Input 的 `mode` 轴区分；`mode` 的值域确认为 `text`、`password`、`search`、`multiline`、`resource-picker`；
- `searchable` 不属于 Input 类型轴，只属于公共 `select` 的受治理模式；
- Input 的 Size 统一为 `small`、`medium`、`large` 三档；三档 Size 复用公共 Size 语义和间距 Token，不为 `text`、`password`、`search`、`multiline`、`resource-picker` 建立私有尺寸值域；
- Input 的公共 Appearance 词汇确认为 `outlined`、`filled`、`dashed`；五种 `mode` 均预留这三种 Appearance 的合法组合。当前只完整定义 `outlined`；`filled` 与 `dashed` 暂作为结构占位，暂不扩展具体视觉 Token、尺寸规则或实现细节；
- `outlined` 是当前阶段唯一完整落地的 Input Appearance，覆盖 `default`、`hover`、`focused`、`error`、`disabled`、`loading` 六个 State；其中 `loading` 继续遵循 Input 已确认的 Spinner、禁用清除按钮和交互锁定规则；
- `text`、`password`、`search`、`multiline`、`resource-picker` 五种 `mode` 统一支持上述六个 State。五种 `mode` 原则上复用同一套视觉 Token 值；它们的主要差异来自结构 Slot 和内容行为，例如 `password-toggle`、`search-icon`、`browse-action` 的有无、`clear` 是否配置以及单行/多行内容布局，不得为模式差异重复建立颜色、边框或状态视觉 Token；
- 多行输入只通过 `mode=multiline` 表达，不建立 `line-count`、`multiline-state` 或其他多行专属公共轴。具体高度、可输入行数、自动扩展策略以及窗口宽度适配属于开发侧 API 与运行时布局，不进入设计侧 Token 或 Figma 变量配置；
- `clear` 是可选功能 Slot，不是 Input 的强制可见元素；没有清除按钮的输入控件仍属于同一公共 `Input` 家族，只是不渲染 `clear` Slot，也不提供对应清除交互。
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
default | hover | focused | open | error | disabled | loading
```

- `open` 表示 Select 弹层已展开，是 Select 的交互 State，不是新的公共组件或独立选择模型；
- `focused` 表示触发器或 searchable Select 的内部输入获得焦点；`focused` 与 `open` 可以同时成立；
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

这些状态的视觉 Token、交互契约和优先级由 `option-item` 自身登记，不能直接复用 Select 的 `default`、`hover`、`focused`、`open`、`error`、`disabled`、`loading` 视觉 Token。父 Select 的 `open`、`error`、`loading` 等状态可以约束 option-item 是否可交互，但不替代 option-item 的内部状态；同名状态也不构成隐式继承。

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
- 共享值通过显式引用被组件组合消费，不依赖隐式继承。所有最终渲染属性必须通过 `Slot` 定位；组件整体属性也使用 `slot/container` 等明确目标，不允许省略 Slot 形成无归属属性。

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

共享值发生重叠时，按以下顺序解析，越靠后优先级越高：

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

Token 体系分为四层。前三层提供可复用的设计决策，第四层定义组件如何消费这些决策。各层目录结构不需要同构，只需建立合法、明确、可追溯的引用关系。

| 层级 | 主要解决的问题 | 组织逻辑 | 示例 |
| --- | --- | --- | --- |
| 01 Primitive | 系统拥有哪些基础设计值 | 按基础属性、色系、数值组织 | `color/blue/55` |
| 02 Profile | 不同设备配置采用哪些基础值 | 按适配参数、尺度组织 | `size/control/md` |
| 03 Semantic | 某个设计值承担什么使用意图 | 按语义角色组织 | `color/text/primary` |
| 04 Component | 某个组件的哪个部件需要什么属性 | 组件 → Intent → Appearance → State → Content → Slot → 属性 | `button/intent/primary/appearance/filled/state/default/content/text-only/slot/label/color` |

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
10. 共享值的覆盖顺序为 `base/size → base/intent → base/appearance → base/state → base/content → 组合覆盖集合（overrides）`；越靠后优先级越高。
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
40. 多行文本输入继续属于公共 `Input` 家族，通过 `mode=multiline` 表达结构差异；不得仅因底层适配 QTextEdit 而新增独立的公共 Token 家族或公共组件类别。
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
51. `focus-ring` 在当前 V2 设计系统中全局禁用，不得由 State、Appearance、Profile 或产品配置重新启用；其 Slot 仅用于保留结构兼容性。
52. `outlined` 的 `error` State 必须保留 `border` 并使用错误语义色，例如 `border/error`；该状态不得启用 `focus-ring`。
53. `error` 具有最高 Border 颜色优先级；`error + focused`、`error + hover` 等叠加情形必须使用 `border/error`，不得被其他 State 颜色覆盖。
54. Input 的六个交互 State 必须由 Registry 独立登记视觉契约、交互契约和优先级；不得要求通过逐状态人工确认才能生成，也不得由生成器隐式推断状态关系。
55. `disabled` 与 `error` 均锁定编辑、点击和提交交互，并采用禁用视觉契约；`error` 的 `border/error` 仍具有最高 Border 优先级，不得被普通 `border/disabled` 替换。
56. `clear` 仅在已配置且输入值非空时显示；空值不显示；`disabled`、`error`、`loading` 时保留已显示的 Slot 但必须 disabled。
57. Input 功能 Slot 的适用规则由 `mode` 固定登记，但实际显示内容必须随当前 State、输入值和 Loading 状态动态解析；`password-toggle` 仅属于 `password`，`search-icon` 仅属于 `search`，不得在其他 `mode` 中自由混用。
58. `password-toggle` 在所属 `mode` 下进入 `disabled`、`error` 时保留但必须 disabled；`search-icon` 始终为非交互装饰 Slot，只随当前 State 更新视觉表现；二者进入 `loading` 时隐藏并由 `spinner` 替换。
59. Input trailing 区的 Slot 顺序为 `clear → password-toggle/search-icon`；Loading 时为 `clear → spinner`，且 `spinner` 必须位于最右侧。
60. `search-icon` 是纯装饰性图标，不得触发搜索、提交或任何点击行为；键盘 Enter 与搜索请求接口也不得由该 Slot 承担。
61. `password-toggle` 只负责密码的显示/隐藏切换，不得新增 `visibility`、Reveal State 或其他公共轴；切换结果由该 Slot 的运行时行为控制。
62. `search` 模式的搜索请求可由开发侧 API 或键盘 Enter 触发，但不得由 `search-icon` Slot 触发。
63. `multiline` 模式下，Enter 只插入换行，不触发提交或搜索请求；该行为由运行时控制，不新增设计侧 State 或 Token 轴。
64. Select 统一支持 `selection=single|multiple` 两种选择模型，保持一个公共 `select` 组件、Registry 条目和 Recipe 边界；不得拆分为 `SingleSelect`、`MultiSelect` 两个公共组件。
65. Select 使用独立的 `selection=single|multiple` 与 `searchable=false|true` 两个维度，四种组合全部合法；`searchable=true` 只增加选项筛选输入能力，不改变 Select 的选择结果语义，也不得生成新的公共组件类别。
66. Select 的 State 轴为 `default`、`hover`、`focused`、`open`、`error`、`disabled`、`loading`；`focused` 与 `open` 可同时成立，`selected` 不得作为 State 轴，`loading` 必须由 Select 自身 Recipe 定义。
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
77. Select 的 `clear` 是可选功能 Slot；有选择值且已配置时显示，无选择值时隐藏。`single` 可通过 `clear` 清空当前值；`multiple` 的批量清空不通过触发器右侧的 `clear` 执行，而只能在 `popup` 内执行；`disabled`、`error`、`loading` 时相关清空操作按 Select 的锁定规则处理。
78. Select trailing 区的 Slot 顺序按选择模型区分：`single` 非 Loading 时为 `clear → dropdown-indicator`；`multiple` 不在触发器 trailing 区提供直接清空的 `clear`，仅显示 `dropdown-indicator`，批量清空放在 `popup` 首行；Loading 时 `dropdown-indicator` 由 `spinner` 替换，`spinner` 必须位于最右侧。
79. Select 的 `single` 使用 `content` 显示选中值；`multiple` 使用 `tag` 表示每个选中值，Tag 位于 content 区域；`tag` 不得用于单选模式。
80. Select 的 `multiple` 支持在 `popup` 内通过 `option-item` 逐个取消正常选项，或通过浮层首行的批量清空操作一次性清除全部选择；异常的禁用 Tag 与对应 `option-item` 不支持单独移除，不新增公共 State 轴或公共组件类别。
81. `multiple` 模式的 `option-item` 复用 `checkbox + label` 结构；每个已选 `option-item` 与 content 区的一个 `tag` 一一对应，Tag 与 checkbox 的选中状态必须双向同步。
82. `single` 模式的 `option-item` 不复用 Radio；它是可选择的单项，由 Select 保证同一时刻最多一个选项被选中，不新增 Radio 结构或公共组件类别。
83. `single` 模式的已选 `option-item` 使用内部 `selected-indicator` 表示，默认可呈现为右侧 `checkmark`；该标记不注册为独立公共组件或新的 Select State 轴。
84. `option-item` 必须拥有独立的 `default`、`hover`、`focused`、`selected`、`disabled` 状态契约；不得继承 Select 父组件的 State Token。父组件状态只能约束其可交互性，不能替代其内部视觉状态。
85. `option-item` 的 `selected` 只属于选项内部选择表现，不属于 Select 公共 State 轴；Select 的选择结果由 `selection` 模型和运行时数据控制。
86. `disabled` 的 `option-item` 必须继续显示在 `popup` 中，使用自身的 `disabled` 视觉 Token，并禁止点击、键盘选择、选中或取消选中；用户尝试操作该项时，由业务层触发可配置的 `warning` 或 `info` 事件，事件不得改变选择结果。
87. 已选中的 `option-item` 被业务侧标记为 `disabled` 时，必须区分选择模型处理：
    - `single`：不得自动清除当前值；Select 父组件进入 `error` 状态并使用错误色；原先选中的 `option-item` 使用自身的 `disabled` 视觉且不可再次选中；其他未禁用的 `option-item` 仍可选择；`clear` 保持可用，用户可通过清空 Select 排除该异常值。用户选择其他正常选项后，Select 自动清除 `error` 状态并恢复正常视觉。只有整个 Select 被业务整体标记为 `disabled` 时，才锁定 Select 的全部交互。
    - `multiple`：不得自动清除异常值；只要仍存在任意一个已选中且被禁用的异常项，Select 父组件就保持 `error` 状态并使用错误色；对应 Tag 保留但使用禁用色；对应 `option-item` 使用自身的 `disabled` 视觉；Checkbox 保持“已选中 + disabled”状态；该项及其 Tag 均不可单独移除。其他正常 Tag 和未禁用的 `option-item` 仍可正常操作；用户只能在 `popup` 内执行一次性清空，直到所有异常选中项被清除后，Select 才自动清除 `error` 状态并恢复正常视觉。
88. `multiple` Select 的 `popup` 首行提供 `select-all` 与 `unselect-all` 批量操作；首行必须固定在浮层顶部，并通过分隔线与下方的 `option-item` 子项区域隔开。`select-all` 只选中当前可用的选项并跳过 `disabled` 项；`unselect-all` 可一次性清除全部当前选择，包括已选中后被业务标记为 `disabled` 的异常项。批量操作不得改变 `option-item` 自身的 `disabled` 视觉和交互契约。
