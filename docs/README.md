# 文档中心

本目录说明 `ComponentLab`、`DesignToCode_GenerateComponent` 和 `DesignToCode_GenerateLayout` 的协作关系。`DesignToCode_GenerateComponent/build/sdk` 是全工作区唯一权威组件 SDK；两个消费项目校验后直接引用。

| 分区 | 用途 | 入口 |
| --- | --- | --- |
| `tutorial` | 第一次阅读、构建顺序和常用命令 | [三项目上手指南](tutorial/三项目上手指南.md) |
| `mainSkill` | AI 执行任务时使用的职责、流程与验证约束 | [工作区 Skill](mainSkill/design-to-qt-workspace/SKILL.md) |
| `designRules` | 跨项目架构、依赖方向和交付原则 | [三项目总体架构](designRules/architecture/项目总体架构.md) |

## 三个项目

| 项目 | 唯一职责 | 主要输出 |
| --- | --- | --- |
| [DesignToCode_GenerateComponent](../DesignToCode_GenerateComponent/README.md) | 设计身份、属性、状态和 Token 到 `Z_*` Qt 组件的映射与构建 | 唯一权威 `build/sdk` |
| [ComponentLab](../ComponentLab/README.md) | 校验并展示已发布组件，检查外观、状态、Profile 和对象树 | `build/demo/component_gallery`；不发布 SDK |
| [DesignToCode_GenerateLayout](../DesignToCode_GenerateLayout/README.md) | 合并同版本 HTML + DSL，按所选组件库合同映射并生成 Qt 页面 | 布局生成工具链与 `.ui/.h/.cpp/.qss/.qrc` 页面包 |

详细规则只在所属项目的 `docs` 中维护。所有历史记录、阶段报告和已替代说明统一进入所属项目的 `docs/archive`；`build/reports` 仅存当次可再生结果。需要长期保留的结果先归档再清理 build。工作区旧版说明见 [归档索引](archive/README.md)。

执行遵循工作区 AGENTS.md 的固定环境原则。独立组件库可声明未实现类，但 contract-only/planned 不等于已发布组件；当前多库与前缀能力见 GenerateLayout 文档。
