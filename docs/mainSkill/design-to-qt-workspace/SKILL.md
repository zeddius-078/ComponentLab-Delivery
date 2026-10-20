---
name: design-to-qt-workspace
description: 管理 ComponentLab-Delivery 三个项目的任务分流、SDK 依赖、构建顺序和验证边界。
---

# Design To Qt Workspace

## 开始前

1. 读取工作区 `AGENTS.md`。
2. 读取目标项目的 README 和 Main Skill。
3. 执行 `git status --short`，保留已有修改。
4. 根据任务范围选择只读检查、项目构建或完整工作区流程。

## 任务分流

| 任务 | 责任项目 | 主要验证 |
| --- | --- | --- |
| Token、组件合同、`Z_*` 实现、SDK 发布 | GenerateComponent | Token 检查、SDK 构建、独立消费者 |
| 组件外观、状态、对象树展示 | ComponentLab | 展示构建、交互回归、SDK 消费 |
| HTML + DSL、布局解析、页面文件生成 | GenerateLayout | Capture、结构矩阵、uic、页面消费者 |
| 跨项目构建、环境检查、离线打包 | 根 `Source/Workspace.ps1` | 工作区级验证 |

## 数据边界

GenerateComponent 生产唯一权威 SDK，路径为 `DesignToCode_GenerateComponent/build/sdk`。ComponentLab 和 GenerateLayout 通过 manifest 和 SHA-256 校验后消费它们。两个消费项目不复制组件私有源码。

`Source/` 保存维护实现；`docs/` 保存规则、固定输入和验证源码；`build/` 保存受控工具、SDK 和运行依赖；`output/` 保存本次工作区、日志和交付批次。

## 执行顺序

```powershell
pwsh -File Source/Workspace.ps1 -Action doctor
pwsh -File Source/Workspace.ps1 -Action build -Target all
pwsh -File Source/Workspace.ps1 -Action verify
pwsh -File Source/Workspace.ps1 -Action package -ReleaseInputs <清单路径>
```

Python 使用已安装的 3.9。Qt、MSVC、CMake、Ninja、Node 和 Playwright 使用现有环境；缺少依赖时记录缺项，不自动安装或升级。

## 任务规则

- 组件身份、属性、状态和 Token 由 GenerateComponent 维护。
- ComponentLab 只展示和验证已发布组件。
- GenerateLayout 只使用 SDK 组件装配页面，不生成第二套组件实现。
- `contract-only/planned` 条目可以用于命名和映射测试，但不能写入已发布可运行组件清单。
- HTML 与 DSL 必须来自同一设计版本，并通过 capture manifest 校验。
- `.ui` 保存结构、布局、objectName 和可声明属性；页面 `.cpp` 负责运行时初始化；页面 `.qss` 负责页面级静态视觉；组件公共样式来自 SDK。

## 验收口径

分别报告：输入证据、适配、页面生成、uic、C++ 消费、运行行为、视觉核对和离线新机器运行。只记录实际执行的命令和结果。
