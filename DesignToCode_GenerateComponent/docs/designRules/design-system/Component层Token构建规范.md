# Component 层 Token 构建规范

本文定义 Component 层 Token 的来源、命名、组合、Slot、状态和生成边界。机器可读源位于 `Source/ComponentKit/design-assets/DesignSystem/source`。

## 1. Token 层次

| 层 | 文件或目录 | 内容 |
| --- | --- | --- |
| Primitive | `primitive.json` | 颜色、长度、字号、圆角、阴影等原子值 |
| Profile | `profile.json` | 宽度、平台和适配参数 |
| Theme | `theme.json` | 明暗和表面环境 |
| Semantic | `semantic.json` | 产品语义，例如 primary、danger、surface |
| Composite | typography、gradient、shadow、motion | 可复用复合值 |
| Component | `component-recipes/*.json` | 组件的稀疏视觉决策 |
| Content | `content/*.json` | 文本、术语和国际化内容 |

Registry、Contract、Recipe、Binding 和 Runtime Mapping 分别保存身份/组合、结构行为、视觉决策、设计绑定和 Qt 映射。生成目录不是维护源。

## 2. 组件轴

公共轴必须使用稳定英文值，具体组件通过 Registry 登记合法组合：

```text
Intent      primary / secondary / danger / success
Appearance  solid / outlined / dashed / ghost
State       default / hover / pressed / focused / disabled / loading / error
Size        xs / sm / md / lg / xl
Content     text / icon-only / text-with-icon
```

组件可增加自己的轴，但必须在 Registry 和 Contract 中说明值域、默认值和互斥关系。`dark` 属于 Theme/Surface 上下文；`icon-only` 属于 Content 结构；`loading` 属于 State。

## 3. Slot 定位

最终渲染属性必须指向明确 Slot。常用 Slot 包括：

```text
container / content / label / icon / leading / trailing
border / focus-ring / spinner / placeholder / popup
```

Slot 表示组件内部结构位置。它不创建新的公共组件，也不创建新的状态轴。

## 4. Button

Button 的合法组合由 Registry 计算。Recipe 只登记与共享 Token 不同的决策；完整矩阵由编译器展开。

`loading` 状态由 State 规则控制内容可见性和交互锁定。加载指示器使用 `spinner` Slot；原有图标和文本是否保留由组件合同明确决定。

## 5. Input

Input 的公共 mode 为：

```text
text / password / search / multiline / resource-picker
```

公共 appearance 为 `outlined`、`filled`、`dashed`。当前完整视觉合同集中在 `outlined`，其他 appearance 由 Registry 保留结构定义，未定义的视觉值不生成。

`placeholder` 由 value-present 条件控制；`clear`、`password-toggle`、`search-icon`、`browse-action` 和 `spinner` 属于 trailing Slot。多行输入不使用内部 clear Slot。

## 6. Select

Select 拥有自己的选择模型、popup、option-item、键盘导航和 open 条件。`searchable` 表示选项筛选能力，不能把 Select 变成普通 Input。

option-item 的选中、悬停、禁用和焦点状态由 option-item 合同登记。父 Select 的 loading、error 和 disabled 只约束交互条件，不能替代 option-item 的内部视觉定义。

## 7. 绑定和引用

绑定记录至少包含组件、轴值、Slot、属性、Token 路径、来源和适用 Profile。引用采用显式路径；生成器不根据相似名称隐式推断。

```json
{
  "component": "button",
  "axes": {"intent": "primary", "appearance": "solid", "state": "default"},
  "slot": "container",
  "property": "background",
  "token": "semantic.color.action.primary.default",
  "source": "source/figma-bindings/button.json"
}
```

## 8. 校验与输出

校验应报告级别、来源文件、JSON 路径、组件、轴值、Slot 和属性。生成链输出 Token、QSS、组件头文件、实现文件、映射资源和 manifest。

```powershell
py -3.9 -B Source/PrepareBuild.py
pwsh -File Source/Publish-ComponentSDK.ps1 -QtRoot <Qt5目录>
py -3.9 -B Source/SDKPackage.py verify --sdk build/sdk --configuration Release --project-root .
```

## 9. 发布边界

生成目录和 SDK 是发布结果，不作为下一次编辑入口。ComponentLab 读取 SDK 展示组件；GenerateLayout 读取 SDK 装配页面。任何新增公共 API 都要同时更新 Contract、映射、头文件、实现、QSS 和消费测试。

## 10. Shadow Composite

Shadow Composite 的基础色统一为 `#000000`，通过 `semantic.color.shadow.base` 引用 `primitive.color.neutral.gray.100`，不得复用文字颜色。几何参数 `offsetX`、`offsetY`、`blur`、`spread` 统一使用 `px`；`spread` 允许保留负值，不得取绝对值或改写为其他参数。`opacity` 独立使用百分比单位，不复制为颜色 Token。Figma 原始候选值必须保留；无法精确映射到 Primitive/Profile 时保留候选值并阻断激活，不得静默近似。

正向 `offsetX`、`offsetY`、`blur` 优先复用或补充 Primitive dimension；负 `spread`（当前为 `-2/-4/-10/-12px`）保留在 Shadow Composite 的候选几何记录中，不扩充通用 Primitive 负尺寸 Token。

为承载 Figma 阴影证据，Primitive 层补充 `opacity/4`、`opacity/6`、`opacity/25`、`opacity/35`；已有的 `opacity/5`、`opacity/10`、`opacity/15`、`opacity/20`、`opacity/30` 直接复用。Shadow 候选记录保留原始百分比作为证据，正式激活时必须解析到对应 Primitive Token。

Figma Shadow/Elevation 1–7 的候选值保存在 `source/shadow.json`，组件只引用 `shadow.elevation-1` 至 `shadow.elevation-7` 预设，不为每个组件或状态复制阴影变量。Focused 复用对应 Rest 阴影并使用公共 Focus Ring；Disabled 在当前 Figma 证据中不产生阴影。
