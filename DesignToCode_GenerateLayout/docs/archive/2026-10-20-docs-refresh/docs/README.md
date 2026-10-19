# GenerateLayout 文档

目录边界：`Source` 只保存解析器、布局生成器与工具源码；`docs` 保存不可丢弃的规范、固定输入、样例及验证源码；`build` 只保存可再生结果。构建脚本本身仍在 Source，不能随 build 一起删除。

2026-10-20 迁移验证：已隔离旧 build，新建空 build 后成功构建解析器并通过两组 C++ 测试。Pixso 文件 `4S0Ktm81WnvzuJmZHvRuOg` 的 `27:450` 只读连接成功；该节点的已保存 HTML/DSL 通过适配、严格解析与 uic 检查，不代表重新采集后的视觉验收。采集和解析执行前后比较 Source/docs 共 407 个文件，无文件增删或哈希变化。

待处理问题：两份历史采集清单的 HTML 哈希不匹配；正式合同测试 37 项中 29 项通过，7 项仍依赖未重新生成的 `build/cache/formal-inputs/result/hybrid-result.json`，1 项 Profile 断言未通过。完整演示运行及视觉验收尚未完成，不应将目录迁移通过写成全部交付通过。旧 build 已重命名为项目根的 `待删除-build-20261020`，交由用户删除，当前工具不依赖该目录。

复查入口：`python -B Source/CheckProject.py`、`python -B docs/validation/test_output_boundary.py`；本次执行证据位于可再生的 `build/reports/source-boundary/verification.json`。

| 分区 | 内容 | 入口 |
| --- | --- | --- |
| `tutorial` | SDK 导入、HTML + DSL 正式生成和验证 | [布局生成上手指南](tutorial/布局生成上手指南.md) |
| `mainSkill` | AI 采集、合并、映射、生成页面和验收时使用的完整流程 | [GenerateLayout Skill](mainSkill/generate-layout-pipeline/SKILL.md) |
| `designRules` | Qt 布局、`.ui`、页面交付和执行门禁 | [布局转换器通用化规范](designRules/layout/布局转换器通用化规范.md) |

## 参考入口

- [设计到布局生成说明](mainSkill/generate-layout-pipeline/references/设计到布局生成说明.md)
- [Qt 页面交付边界](mainSkill/generate-layout-pipeline/references/page-delivery/Qt页面交付边界.md)
- [Qt 功能分层](designRules/architecture/Qt功能分层.md)
- [布局生成执行与验收规则](designRules/workflow/布局生成执行与验收规则.md)
- [Qt UI 布局规则](designRules/layout/Qt-UI布局规则.md)
- [布局转换器通用化规范](designRules/layout/布局转换器通用化规范.md)

组件类、Token、QSS 和工厂由 GenerateComponent 发布。本项目只通过导入后的 SDK 装配页面。

组件 Token 的历史架构审计位于工作区兄弟项目 `DesignToCode_GenerateComponent/docs/archive/reports/2026-10-19-seven-layer-architecture-refresh.md`；当前状态需以维护源与本轮验证为准。该跨项目资料不复制到本项目，历史记录也不放在本项目 `Source/` 或 `docs/` 顶层。
