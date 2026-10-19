# GenerateLayout 文档

现行文档只描述当前规则和入口；历史版本、过程记录及过去的测试结论统一见 [归档索引](archive/README.md)。归档不是当前验收报告。

| 分区 | 用途 | 入口 |
| --- | --- | --- |
| tutorial | 操作与验证 | [上手指南](tutorial/布局生成上手指南.md) |
| mainSkill | AI 执行边界与流程 | [Main Skill](mainSkill/generate-layout-pipeline/SKILL.md) |
| designRules | 当前规范 | [Qt 功能分层](designRules/architecture/Qt功能分层.md) |
| archive | 历史记录，只读追溯 | [历史归档](archive/README.md) |

遵循工作区 AGENTS.md：不自主安装或升级已有环境。Source 保存维护源码和工具，docs 保存规范、固定输入、示例及验证源码，build 保存可再生成果。构建不回写 Source/docs；永久证据不只留在可删除的 build。

## 专项规范

- [多组件库与前缀切换](designRules/library-profiles.md)
- [设计到布局生成说明](mainSkill/generate-layout-pipeline/references/设计到布局生成说明.md)
- [布局生成执行与验收规则](designRules/workflow/布局生成执行与验收规则.md)
- [Qt 页面交付边界](mainSkill/generate-layout-pipeline/references/page-delivery/Qt页面交付边界.md)
- [布局转换器通用化规范](designRules/layout/布局转换器通用化规范.md)
- [Qt UI 布局规则](designRules/layout/Qt-UI布局规则.md)

固定输入和映射在 docs/fixtures/layout，模板与样例在 docs/examples，测试在 docs/validation。LibraryProfile 选择目标库；默认 token-sdk，新 analysis-system 仅是 planned 类名合同。正式与预览分开验收，不能由 uic 通过推断目标实现存在。

旧采集清单必须重新核对哈希；导航栏 Token 消费契约问题仍按用户决定挂起。历史测试数字不作为当前结论。
