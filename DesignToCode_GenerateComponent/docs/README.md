# GenerateComponent 文档

现行文档只描述当前规则和入口；历史版本、过程记录及过去的测试结论统一见 [归档索引](archive/README.md)。归档不是当前验收报告。

| 分区 | 用途 | 入口 |
| --- | --- | --- |
| tutorial | 操作与验证 | [上手指南](tutorial/组件SDK上手指南.md) |
| mainSkill | AI 执行边界与流程 | [Main Skill](mainSkill/generate-component-pipeline/SKILL.md) |
| designRules | 当前规范 | [Qt 功能分层](designRules/architecture/Qt功能分层.md) |
| archive | 历史记录，只读追溯 | [历史归档](archive/README.md) |

遵循工作区 AGENTS.md：不自主安装或升级已有环境。Source 保存维护源码和工具，docs 保存规范、固定输入、示例及验证源码，build 保存可再生成果。构建不回写 Source/docs；永久证据不只留在可删除的 build。

## 专项规范

- [Component 层 Token 构建规范](designRules/design-system/Component层Token构建规范.md)
- [Figma 统一组件融合规则](designRules/design-system/Figma统一组件融合规则.md)
- [设计组件映射指南](mainSkill/generate-component-pipeline/references/设计组件映射指南.md)
- [Qt 组件类与主属性参考](mainSkill/generate-component-pipeline/references/Qt组件类与主属性参考.md)

Token 源位于 Source/ComponentKit/design-assets/DesignSystem/source。PrepareBuild 在 build/work 准备副本后生成；--copy-only 才是只复制输入。SDK 消费测试位于 docs/validation/SDKConsumerTest，映射工作台位于 docs/examples/MappingWorkbench。

Python 3.9 是兼容目标；接口检查不能替代真实 3.9 构建与消费实测。当前文档不宣称全量组件验收通过。
