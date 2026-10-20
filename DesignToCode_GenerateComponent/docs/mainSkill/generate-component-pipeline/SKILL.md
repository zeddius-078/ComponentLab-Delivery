---
name: generate-component-pipeline
description: 维护设计组件、Token、Qt 映射、Z_* 实现和 GeneratedComponentSDK 发布流程。
---

# Generate Component Pipeline

## 开始前

1. 读取工作区 `AGENTS.md`、本项目 README 和[组件 SDK 上手指南](../../tutorial/组件SDK上手指南.md)。
2. 执行 `git status --short`。
3. 检查 Python 3.9、Qt 5.15.2、MSVC、CMake、Ninja 和所需 Node 工具是否已存在。
4. 根据任务读取 Token 规范、映射指南或 Qt 属性参考。

## 维护源

| 内容 | 路径 |
| --- | --- |
| Token、主题和 Profile | `Source/ComponentKit/design-assets/DesignSystem/source` |
| Component Registry | `Source/ComponentKit/design-assets/DesignSystem/source/component-registry.json` |
| Contract | `source/component-contracts` |
| Recipe | `source/component-recipes` |
| 设计绑定 | `source/figma-bindings` |
| Qt 映射 | `Source/Qt/MappingWorkbench` |
| 组件实现 | `Source/ComponentKit/runtime`、`component-definitions` |

`output` 中的生成副本和 `build` 中的 SDK/工具均由 Source 发布，不作为手工编辑入口。

## 组件生成顺序

```text
设计身份与变量
→ Registry / Contract / Recipe
→ Token 与 QSS
→ Z_* 组件源码
→ 编译与独立消费测试
→ manifest 与 SDK 发布
```

每个公开组件至少要有设计身份、合法属性和状态、Token 绑定、公开头文件、实现、QSS、工厂注册和消费测试。contract-only/planned 只记录目标合同，不进入可运行 SDK。

## 执行

```powershell
pwsh -File Source/Publish-ComponentSDK.ps1 -QtRoot <Qt5目录>
py -3.9 -B Source/SDKPackage.py verify --sdk build/sdk --configuration Release --project-root .
```

发布器使用 `output/work` 暂存，校验通过后更新 `build/sdk`。切换 SDK 时使用新的 CMake 工作目录，避免缓存绑定旧包。

## 验证

验证顺序为：源边界、Token 生成、C++ 编译、manifest、独立 SDK 消费、ComponentLab 消费和视觉展示。只报告已经执行的阶段。

## Agent 约束

- 组件实现和 Token 只在本项目维护。
- ComponentLab 不作为组件源码来源。
- GenerateLayout 不作为第二个组件生成器。
- 任何新 API 先写入 Contract、映射和公开头文件，再接入生成器。
