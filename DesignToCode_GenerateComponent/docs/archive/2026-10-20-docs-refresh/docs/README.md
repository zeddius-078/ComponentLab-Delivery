# GenerateComponent 文档

环境维护遵循工作区 `AGENTS.md`：上位机已匹配环境不得自主安装或升级；遇到不兼容先修源码，需要改环境必须询问用户。Python 构建脚本以 3.9 为兼容目标。两个组件生成器已将 `Path.write_text(newline=...)` 改为 UTF-8 `write_bytes`，保持 LF 输出；对应回归测试位于 `docs/validation/test_python39_writes.py`。本机没有登记 Python 3.9，接口兼容回归不能替代公司上位机的 3.9 实测。

| 分区 | 内容 | 入口 |
| --- | --- | --- |
| `tutorial` | 环境、SDK 发布、验证和开发者接入 | [组件 SDK 上手指南](tutorial/组件SDK上手指南.md) |
| `mainSkill` | AI 处理设计证据、映射、Token、组件源码和 SDK 时使用的完整流程 | [GenerateComponent Skill](mainSkill/generate-component-pipeline/SKILL.md) |
| `designRules` | 技术分层、映射原则、设计系统基线和组件规则 | [Qt 功能分层](designRules/architecture/Qt功能分层.md) |

## 参考入口

- [设计组件映射指南](mainSkill/generate-component-pipeline/references/设计组件映射指南.md)
- [Qt 组件类与主属性参考](mainSkill/generate-component-pipeline/references/Qt组件类与主属性参考.md)
- [Figma V2 重绑定计划](archive/reports/figma-rebind-plan.json)
- [Component 层 Token 构建规范](designRules/design-system/Component层Token构建规范.md)
- [七层架构审计与刷新报告（2026-10-19）](archive/reports/2026-10-19-seven-layer-architecture-refresh.md)

历史验收、交接与 Figma 重绑定记录集中在 `docs/archive`；构建期机器报告在 `build/reports`。历史审计中的数值不是本轮重新测量结果。当前规则以维护源、实际实现和本次测试为准。

## 协同交接入口

- [变更情况与阅读指引（2026-10-18，历史归档）](archive/reports/archive/2026-10-18/2026-10-18-collaboration-handoff.md) —— 历史协同记录
- [版本变更摘要（2026-10-18，历史归档）](archive/reports/archive/2026-10-18/2026-10-18-change-report.md)

## 目录与生成规则

- `Source`：维护实现、生成工具、Token 原始层、Registry/Contract/Recipe、设计输入和资源；不允许生成器回写。
- `docs`：规范、示例、消费测试及历史档案，不能作为临时目录删除。
- `build`：可整体删除。`work` 是维护输入的构建副本加生成输出，不是第二套源码；`cache` 保存编译缓存，`sdk` 保存发布包，`resource` 保存源码交付包，`reports` 保存本轮验证结果。

统一执行 `Source/Publish-ComponentSDK.ps1`，准备构建副本后生成、检查、编译并发布 SDK。只准备构建输入可运行 `python -B Source/PrepareBuild.py`。不要直接编辑 build/work；维护更改应回到 Source，再重新执行发布入口。直接在 Source 调用会写文件的生成器将明确拒绝。

2026-10-20 的迁移前输出保存在 `archive/2026-10-20-source-generated`，只用于追溯；生成链不读取这些旧产物。两份设计审计是例外的只读验证输入，已显式登记在 PrepareBuild.py，不参与 Token 值生成。旧 build 已标为项目根的 `待删除-build-20261020`，可由用户删除。

本次整理与验证见 [目录解耦记录](archive/2026-10-20-source-generated/README.md)。目录边界和编译已通过，但新 SDK 消费测试发现导航栏 Token 契约不一致；兼容性修复前建议保留待删除目录内的旧 SDK。
