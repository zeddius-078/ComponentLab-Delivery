---
name: design-to-qt-workspace
description: 管理 ComponentLab-Delivery 三项目的职责分流、Token 与 SDK 依赖、多组件库映射、文档归档和验证。用于跨项目任务、目录边界检查或设计到 Qt 链路协作。
---

# Design To Qt Workspace

## 开始前

1. 读取工作区 AGENTS.md、[总体架构](../../designRules/architecture/项目总体架构.md)及目标项目 README。
2. 读取责任项目的 Main Skill；涉及具体 API 再读其 references。
3. 检查已有修改与并行任务，保留用户改动。只处理授权范围，不自动恢复挂起的问题。

## 分流

| 任务 | 责任项目 | 验证边界 |
| --- | --- | --- |
| Token、组件实现、工厂、SDK 发布 | DesignToCode_GenerateComponent | 生成检查、构建、独立消费者 |
| 已发布组件展示与交互 | ComponentLab | 展示构建、项目检查、运行验证 |
| HTML+DSL 解析、布局、目标组件集声明 | DesignToCode_GenerateLayout | 输入校验、映射/结构测试、相应交付模式 |
| 文档及历史整理 | 所属项目 docs | 路径、链接、事实与 Skill 校验 |

组件库定义、组件生成和布局生成分别维护。独立目标库 JSON 可声明 planned 类，不能注册为已实现 SDK。默认 token-sdk 与新库配置分离；前缀切换使用 classPrefix，不批量改写源设计和历史样例。

## 硬边界

- Source 保存实现与构建工具；docs 保存现行规范、固定输入、样例、测试和档案；build 只保存可再生产物。
- 阶段记录、旧说明、迁移证据统一 docs/archive。build/reports 仅是当次可再生报告，永久证据必须归档。
- GenerateComponent 在 build/work 生成，不回写 Source。保留 PrepareBuild.py 显式读取的只读审计路径。
- 两个消费者直接校验并引用 GenerateComponent/build/sdk，不复制 SDK 或私有源码。
- 当前七层由 v2-source-manifest.json 定义；Theme 与 Profile 分开，Content 不承担视觉或运行时规则。
- 一个统一组件集内的公共身份一对一；不同目标库是明确选择的映射上下文，不混合成一条多目标映射。
- 环境固定：不安装、升级、降级、补包或修改全局配置；缺依赖先报告用户。Python 目标 3.9，较新解释器通过不等于 3.9 实测。
- 导航栏缺失 Token 的消费契约问题按用户决定挂起；未经要求不恢复修复。

## 执行顺序

组件实现或 Token 变化才执行 SDK 发布、docs/validation/SDKConsumerTest/Build.ps1、消费端 Source/Verify-GeneratedComponentSDK.ps1，然后重建对应消费者。文档或库命名合同更新不触发无关发布。

正式页面必须有同版 HTML/DSL、哈希、独立布局证据、显式字体与映射；输出 ui/h/cpp/qss/qrc。contract-only 库只做预览，不进入正式交付或运行验收。

## 验证与交付

按改动选检查，不声称未执行的验收：
- GenerateComponent、GenerateLayout：各自 `python -B Source/CheckProject.py`。
- ComponentLab：`python -B Source/delivery/tools/CheckProject.py`。
- Skill 修改：使用 skill-creator 的 quick_validate.py；缺依赖不安装。
- 文档修改：检查本地链接、命令路径、当前/历史分区，保留归档原文。

说明修改范围、当前能力、实际测试、失败/未测项及归档入口。历史通过不能替代当前版本通过，登记数不能替代实例化与视觉验收。
