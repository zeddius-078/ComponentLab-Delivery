# ComponentLab 文档

## 阅读入口

| 文档 | 内容 |
| --- | --- |
| [组件展示上手指南](tutorial/组件展示上手指南.md) | 校验 SDK、构建展示程序和运行回归 |
| [ComponentLab Gallery Skill](mainSkill/componentlab-gallery/SKILL.md) | AI Agent 执行展示任务时的文件边界和验证流程 |
| [Token 变量集使用](designRules/token/Token变量集使用.md) | 展示页如何选择和读取 Token Profile |

## 项目边界

ComponentLab 读取 `DesignToCode_GenerateComponent/build/sdk` 的公开包。页面、控制器和测试属于展示侧；组件源码、Token 源和 SDK 发布属于 GenerateComponent。

结果写入 `output/logs` 或明确的展示交付目录。历史版本通过 Git 提交查看。
