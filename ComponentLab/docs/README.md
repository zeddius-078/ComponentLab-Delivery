# ComponentLab 文档

现行文档只描述当前规则和入口；历史版本、过程记录及过去的测试结论统一见 [归档索引](archive/README.md)。归档不是当前验收报告。

| 分区 | 用途 | 入口 |
| --- | --- | --- |
| tutorial | 操作与验证 | [上手指南](tutorial/组件展示上手指南.md) |
| mainSkill | AI 执行边界与流程 | [Main Skill](mainSkill/componentlab-gallery/SKILL.md) |
| designRules | 当前规范 | [Token 使用](designRules/token/Token变量集使用.md) |
| archive | 历史记录，只读追溯 | [历史归档](archive/README.md) |

遵循工作区 AGENTS.md：不自主安装或升级已有环境。Source 保存维护源码和工具，docs 保存规范、固定输入、示例及验证源码，build 保存可再生成果。构建不回写 Source/docs；永久证据不只留在可删除的 build。

组件实现缺口回到 GenerateComponent，不用演示伪实现补齐。新增独立目标库仍为 contract-only 时不得宣传为本项目可运行组件。
