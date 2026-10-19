---
name: design-to-qt-workspace
description: 管理 G:/ComponentLab-Delivery 三项目工作区的职责分流、跨项目依赖、文档治理和端到端验证。用户提出组件生成、Token 编译、ComponentLab 展示、Pixso HTML+DSL 页面生成、GeneratedComponentSDK 发布或三项目架构调整时使用。
---

# Design To Qt Workspace

## 目标

把任务分配到唯一责任项目，保持组件生产、组件验收和页面装配三条链路解耦。每次修改都需要给出输入、处理、输出和实际验证结果。

## 开始前

1. 读取 `G:/ComponentLab-Delivery/docs/designRules/architecture/项目总体架构.md`。
2. 读取目标项目根目录 `README.md`。
3. 读取目标项目的 `docs/mainSkill/*/SKILL.md`。
4. 运行 `git status --short`，保留用户已有改动。
5. 检查任务是否同时触及多个项目；跨项目修改必须按“发布 SDK → 校验 SDK → 直接引用”顺序执行。

## 任务分流

| 用户目标 | 责任项目 | 允许修改 | 主要验证 |
| --- | --- | --- | --- |
| 新增或修复 `Z_*` 组件 | `DesignToCode_GenerateComponent` | 组件合同、Token、QSS、`.h/.cpp`、工厂、SDK | SDK 发布、独立消费测试 |
| 修改 Token 解析或 Profile | `DesignToCode_GenerateComponent` | `Source/ComponentKit/design-assets` | Token 检查、SDK 发布 |
| 展示组件外观、状态和对象树 | `ComponentLab` | Demo、展示控制器、测试 | Demo 构建、项目检查 |
| 从设计页生成布局 | `DesignToCode_GenerateLayout` | HTML+DSL 合并、映射、编译器、页面案例 | 页面结构、独立消费、Demo |
| 修改跨项目职责或依赖方向 | 工作区文档及受影响项目 | README、Skill、校验/发布接口 | 三项目边界检查 |

## 固定依赖方向

```text
设计证据 + sToken + Qt 5.15.2
                 |
                 v
DesignToCode_GenerateComponent
                 |
                 v
       GeneratedComponentSDK
            /           \
           v             v
 ComponentLab      GenerateLayout
                         |
                         v
                    业务 Qt 工程
```

ComponentLab 与 GenerateLayout 是并列消费者。两者不得复制组件实现、Token 编译器或 GenerateComponent 的私有源码。

全工作区只有一个 SDK 产品：`DesignToCode_GenerateComponent/build/sdk`。GenerateLayout 的 HTML+DSL 编译器、脚本和校验程序统一归类为布局生成工具链，不使用 SDK 名称；ComponentLab 只输出展示 Demo。

## 跨项目执行顺序

1. 在 GenerateComponent 修改设计映射、Token 或组件实现。
2. 运行 `Source/Publish-ComponentSDK.ps1` 生成 `build/sdk`。
3. 运行 `Source/SDKConsumerTest/Build.ps1`，确认开发者只引用公共头和库即可创建组件。
4. 在消费项目运行 `Source/Verify-GeneratedComponentSDK.ps1`，校验 manifest、发布者和 SHA-256；构建直接引用同一发布目录。
5. 分别重建 ComponentLab 或运行 GenerateLayout 页面生成工具链。
6. 报告每一步实际执行结果；未经执行的检查标记为“未验证”。

## 技术边界

| 层 | 承担内容 |
| --- | --- |
| Pixso/Figma 设计数据 | 组件身份、实例属性、状态、变量绑定、页面节点 |
| Token 编译 | Base、Semantic、Component、Profile 到运行时 Token 与 QSS |
| `Z_*` C++ 组件 | Qt 原生能力提升、设计属性 API、状态与绘制 |
| ComponentLab | 已发布组件的可视化展示与验收 |
| HTML + DSL | HTML 提供布局测量，DSL 提供节点身份、组件属性和 Token 引用 |
| `.ui/.h/.cpp/.qss` | 页面结构、公开访问器、运行时连接和页面样式 |

主线技术栈为 Qt 5.15.2、C++17、MSVC。Python、PowerShell 和 JavaScript 只用于采集、生成、检查和构建工具，不进入组件运行时。

## 文档规则

- `docs/tutorial`：第一次使用和按步骤操作。
- `docs/mainSkill`：AI Skill、API 参考和工具接口。
- `docs/designRules`：架构、设计原则、Token、组件与布局规则。
- `build/reports`：测试结果、阶段记录、迁移记录和视觉证据。
- 源码目录只保留运行、构建或可复现生成所需说明。

新增文档前先确认现有文档能否承载。每个项目 `docs/README.md` 只做索引，不复制详细规则。

## 验证与交付

至少执行与改动直接相关的检查：

```powershell
python -B <项目>/Source/CheckProject.py
```

组件变更还需 SDK 发布与独立消费测试；展示变更需 ComponentLab 构建；页面变更需 `check_delivery.py` 和隔离消费验证。最终说明修改文件、依赖变化、实际测试、未验证项和后续入口。
