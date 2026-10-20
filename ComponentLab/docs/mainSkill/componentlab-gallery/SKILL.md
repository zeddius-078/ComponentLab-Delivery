---
name: componentlab-gallery
description: 维护 ComponentLab 的组件展示、状态切换、对象树和 SDK 消费验证。
---

# ComponentLab Gallery

## 项目职责

ComponentLab 消费 GenerateComponent 发布的 SDK，展示已发布组件。组件身份、Token、QSS、C++ 实现和公共接口由 GenerateComponent 维护。

## 开始前

1. 读取工作区 `AGENTS.md`、本项目 README 和[上手指南](../../tutorial/组件展示上手指南.md)。
2. 运行 `git status --short`。
3. 执行 `Source/Verify-GeneratedComponentSDK.ps1`。
4. 确认目标 Profile、组件族和验收场景。

## 关键目录

| 路径 | 作用 |
| --- | --- |
| `Source/delivery/demo` | 展示窗口、组件页和对象树页面 |
| `Source/presentation` | 展示侧控制器和状态编排 |
| `Source/delivery/tests` | 展示和消费回归 |
| `Source/delivery/tools` | 构建、部署、运行和打包脚本 |
| `docs` | 当前规则、教程和固定验证输入 |
| `build` | 受控展示程序和支持工具 |
| `output` | 本次执行工作区、日志和展示包 |

## 修改流程

### 展示一个新组件

1. 确认组件已出现在 SDK manifest 和公开头文件中。
2. 在 ComponentLab 注册展示页或展示卡片。
3. 使用公开接口创建组件，设置状态和 Profile。
4. 增加对象树和交互检查。
5. 构建并运行展示回归。

### 发现 SDK 缺口

记录缺少的头文件、类、Token、资源或运行时接口，转交 GenerateComponent。ComponentLab 保留可复现的展示失败信息，不在展示层补写组件实现。

## 展示约束

- 展示页按组件族组织，原生 Qt 类和 `Z_*` 组件按各自身份分栏。
- 每个组件展示外观、状态、对象树和注册信息。
- contract-only/planned 条目只能显示为映射或计划信息，不创建可运行组件卡片。
- Token Profile 的改变必须来自 SDK 提供的 Profile 选择，不在页面里写死颜色和尺寸。

## 验证

```powershell
pwsh -File Source/Verify-GeneratedComponentSDK.ps1
pwsh -File Source/delivery/tools/Build.ps1
pwsh -File Source/delivery/tools/Verify.ps1
```

只报告实际运行的检查。展示程序通过不替代 SDK 消费测试。
