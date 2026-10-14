# GenerateLayout 文档

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
