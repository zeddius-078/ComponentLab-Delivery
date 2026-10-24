---
name: design-to-qt-workspace
description: 管理 ComponentLab-Delivery 三个项目的任务分流、SDK 依赖、构建顺序和验证边界。
---

# Design To Qt Workspace

## 开始前

1. 读取工作区 `AGENTS.md`。
2. 读取并核对 [受控执行设备基线](../../designRules/architecture/agent-execution-capability-profile.json)。当前 SKU 是 `DS-V4F-BUILDUNK-NT-C256K-TEXT-AGENT-PROVIDERUNK`；执行端不得根据名称推断额外能力。
3. 读取目标项目的 README 和 Main Skill。
4. 执行 `git status --short`，保留已有修改。
5. 根据任务范围选择只读检查、项目构建或完整工作区流程。

## 任务分流

| 任务 | 责任项目 | 主要验证 |
| --- | --- | --- |
| Token、组件合同、`Z_*` 实现、SDK 发布 | GenerateComponent | Token 检查、SDK 构建、独立消费者 |
| 组件外观、状态、对象树展示 | ComponentLab | 展示构建、交互回归、SDK 消费 |
| HTML + DSL、布局解析、页面文件生成 | GenerateLayout | Capture、结构矩阵、uic、页面消费者 |
| Pixso 原生 Vue 导出件、静态组件/布局映射 | GenerateLayout 的独立 pixso_native 路径 | 来源、静态解析、映射记录、uic、真实页面消费者 |
| 跨项目构建、环境检查、离线打包 | 根 `Source/Workspace.ps1` | 工作区级验证 |

## 数据边界

GenerateComponent 生产唯一权威 SDK，路径为 `DesignToCode_GenerateComponent/build/sdk`。ComponentLab 和 GenerateLayout 通过 manifest 和 SHA-256 校验后消费它们。两个消费项目不复制组件私有源码。

`Source/` 保存维护实现；`docs/` 保存规则、固定输入和验证源码；`build/` 保存受控工具、SDK 和运行依赖；`output/` 保存本次工作区、日志和交付批次。

## 执行顺序

以下命令是从工作区根目录执行的开发构建流程，不是只消费已有 SDK 的页面转换任务的必经步骤。原生 Vue 任务先读 [GenerateLayout Main Skill](../../../DesignToCode_GenerateLayout/docs/mainSkill/generate-layout-pipeline/SKILL.md) 与[第二路径说明](../../../DesignToCode_GenerateLayout/docs/Pixso原生DSM-D2C到Qt第二路径说明.md)；第二路径目前从 Source 执行，尚未接入工作区受控工具或离线包发布，不以第一路径替代。

```powershell
pwsh -File Source/Workspace.ps1 -Action doctor
pwsh -File Source/Workspace.ps1 -Action build -Target all
pwsh -File Source/Workspace.ps1 -Action verify
pwsh -File Source/Workspace.ps1 -Action package -ReleaseInputs '<清单路径>'
```

Python 使用已安装的 3.9。按阶段使用既有 Qt、MSVC、CMake、Ninja、Node 和解析/浏览器依赖；缺项记录，不安装或升级。原生 Vue 静态转换不要求 Vue 应用、Vite、Playwright 或 Chromium，但需要第二路径声明的现有解析模块，不能省略依赖后宣称换机可用。

## 任务规则

### 受控执行端限制

该工作区的默认输出设备是 DeepSeek V4 Flash 256K Non-Think。它只能执行**已冻结、可验证**的任务：任务规格必须明确输入文件或 URL、根节点、模式、SDK、Profile、映射、输出批次及验收命令。执行端不得自行改变这些选择，也不得用样例、历史输出或占位值替代缺失输入。

对于缺失 Token 模式、设计几何、组件身份、SDK API、工具或环境的情况，必须记录原始错误并停止受影响分支。禁止新建解析器或 `.ui` 发射器，禁止自动降级、切换 Profile/SDK，禁止修改 Source 以掩盖生成失败。构建/发布结果必须带有执行设备 SKU 与画像哈希；未带标签的受控工具为 `legacy-unlabeled`，不能作为此基线下的新验收结果。

- 组件身份、属性、状态和 Token 由 GenerateComponent 维护。
- ComponentLab 只展示和验证已发布组件。
- GenerateLayout 只使用 SDK 组件装配页面，不生成第二套组件实现。
- 组件视觉以组件专属 QSS（固定值）为运行事实源，禁止生造与 QSS 并行的运行时变量体系；更新设计直接编辑组件 QSS，新旧 QSS 通过 profile/版本/资源路径并存。
- `contract-only/planned` 条目可以用于命名和映射测试，但不能写入已发布可运行组件清单。
- 第一路径 HTML 与 DSL 必须来自同一设计版本，并通过 capture manifest 校验。第二路径核对原始导出、SFC/CSS/资源和旁置生成记录，不伪造 HTML、DSL 或同批关系。
- 第二路径按显式 export-first 配置允许属性/视觉/内容差异继续生成候选，差异及系统默认图标回退均逐项记录。STAGED、编译、运行、视觉和发布分别报告；策略不允许虚构 API、关闭路径安全或改写来源哈希。
- `.ui` 保存结构、布局、objectName 和可声明属性；页面 `.cpp` 负责运行时初始化；页面 `.qss` 负责页面级静态视觉；组件公共样式来自 SDK。

## 验收口径

分别报告：输入证据、适配、页面生成、uic、C++ 消费、运行行为、视觉核对和离线新机器运行。只记录实际执行的命令和结果。
