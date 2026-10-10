# Figma 核心设计系统实施记录

> Figma：`Design-Components-Build` / `w92BKW1EcutKmTXkwzNHbf`  
> 实施日期：2026-10-07

> **历史快照**：本文只记录 2026-10-07 的目录改造与首轮试点结果。之后新增或修正的组件资产、身份、Token 绑定和开放问题，应查看[现有状态账本](figma-core-rebuild-state.json)并回读 Figma 原节点；下文的“已完成”“最终目录”和“尚未完成”均不代表 2026-10-09 的实时全量状态，也不构成 Qt 运行验收。

## 已完成

1. 建立规范、控件、工程协同、参考资料四段式页签结构。
2. 新建 8 个核心治理内容页，明确事实源、Token 模型、组件合同、Figma→Qt 映射与发布门禁。
3. 新建 9 个控件分类页，覆盖鸿蒙目录中的导航、展示、操作、输入、选择、容器，并增加 Qt 桌面需要的数据视图、反馈、窗口与系统。
4. 将现有 Button、Checkbox、Select 提升到对应分类，并在画布左侧增加 canonical contract 说明。
5. 为 26 个现有组件集追加设计状态、工程状态、Design ID、Qt/SDK 候选身份及合同入口。
6. 将 Ant Design、Layout、Feedback 与历史 PPT 统一归入 `90.* Reference`，保留内容但移除隐式权威。
7. 所有新增治理/分类页面直接绑定现有 `QtLab / Runtime` 语义颜色，未复制或另建第二套 Token。

## 最终目录

| 范围 | 页签 |
|---|---|
| 总览 | `00 · Core Design System`、`01 · Start Here` |
| 通用基础 | `10 · 通用设计基础 / Foundations`、`11 · Tokens & Runtime Profiles` |
| 控件治理 | `20 · 控件总览 / Coverage`、`20.0 · Component Contract` |
| 控件分类 | `20.1` 导航、`20.2` 展示、`20.3` 操作、`20.4` 输入、`20.5` 选择、`20.6` 容器、`20.7` 数据视图、`20.8` 反馈、`20.9` 窗口与系统 |
| 现有组件 | `20.3.01 · Button`、`20.5.01 · Checkbox`、`20.5.02 · Select` |
| 工程协同 | `30 · Figma → Qt Contract`、`31 · Release & Evidence` |
| 参考归档 | `90.1` AntD Components、`90.2` Kickoff & Color、`90.3` AntD Layout、`90.4.*` AntD Feedback |

## 关键节点

| 内容 | Page ID | Root/Doc ID |
|---|---:|---:|
| Core cover | `320:2710` | `321:2` |
| Start Here | `320:2711` | `321:4335` |
| Foundations | `320:2713` | `321:4439` |
| Tokens | `320:2714` | `321:4550` |
| Coverage | `127:2710` | `322:2` |
| Component Contract | `320:2716` | `322:4355` |
| Figma→Qt Contract | `320:2718` | `322:4462` |
| Release & Evidence | `320:2719` | `322:4577` |
| Button contract doc | `172:2710` | `323:422` |
| Checkbox contract doc | `234:2717` | `323:4269` |
| Select contract doc | `242:2720` | `323:4295` |

分类页根节点：Navigation `328:2`、Display `328:4327`、Actions `328:4409`、Input `328:4493`、Selection `328:4575`、Container `329:2`、Data Views `329:4325`、Feedback `329:4407`、Window & System `329:4489`。

## 验证结果

- 17 个新增内容页根节点均为 1440px 宽。
- 17 个新增内容页及 3 个组件合同侧栏均无缺失字体。
- 所有横向 Auto Layout 已改为内容高度自适应，无已知叠压。
- 核心页每页有 69–91 个变量绑定节点；分类页每页有 62–66 个变量绑定节点。
- Button 5/5、Checkbox 2/2、Select 19/19 组件集已写入合同状态标记。
- 已截图复核 Cover、Coverage、Figma→Qt、Actions、Feedback 与 Select 合同侧栏。

## 尚未完成（不得误报为已发布）

1. Select 的 `Z_Select` / `Z_ComboBox` / 组合层身份决策。
2. 除 Button、Checkbox、Select 外的公共 Figma 组件资产及工程合同。
3. 59 个 Qt 可视类型的公共组件准入清单。
4. 每个组件的设计 Owner、工程 Owner、版本与双签记录。
5. C++/Qt Code Connect；当前 Figma Code Connect 没有适合的 C++/Qt 语言标签。

