# 工作区文档中心

这里记录三个项目的共同规则、执行入口和交付边界。项目内部的技术细节放在对应项目的 `docs/` 中。

## 阅读顺序

| 顺序 | 文档 | 用途 |
| --- | --- | --- |
| 1 | [三项目上手指南](tutorial/三项目上手指南.md) | 了解首次检出、环境检查和构建顺序 |
| 2 | [项目总体架构](designRules/architecture/项目总体架构.md) | 查看职责、数据流和 SDK 依赖 |
| 3 | [工具与交付依赖](designRules/architecture/工具与交付依赖.md) | 核对开发工具、运行依赖和许可清单 |
| 4 | [离线交付](tutorial/离线交付.md) | 使用完整离线包 |
| 5 | [工作区 Skill](mainSkill/design-to-qt-workspace/SKILL.md) | 执行跨项目任务时的操作规则 |
| 6 | [项目进展时间线](ComponentLab-Delivery项目进展时间线.md) | 管理层视角的阶段成果与当前重点 |

## 项目入口

| 项目 | 文档入口 | 输出 |
| --- | --- | --- |
| [GenerateComponent](../DesignToCode_GenerateComponent/README.md) | [组件文档](../DesignToCode_GenerateComponent/docs/README.md) | `build/sdk` 中的 `GeneratedComponentSDK` |
| [ComponentLab](../ComponentLab/README.md) | [展示文档](../ComponentLab/docs/README.md) | 组件展示程序和验证结果 |
| [GenerateLayout](../DesignToCode_GenerateLayout/README.md) | [布局文档](../DesignToCode_GenerateLayout/docs/README.md) | 布局生成工具与页面文件包 |

GenerateLayout 的 HTML＋DSL 与原生 Vue 导出件分别走独立入口；原生 Vue 任务以[第二路径说明](../DesignToCode_GenerateLayout/docs/Pixso原生DSM-D2C到Qt第二路径说明.md)为当前命令和验收边界，不将第一路径的浏览器环境或正式交付状态套用到第二路径。

## 工作区目录

`Source/` 保存工作区脚本，`docs/` 保存共同规则，三个项目各自拥有同名目录。项目的 `build/` 保存受控发布资产，`output/` 保存执行期间的工作文件、日志和交付批次。

历史过程通过 Git 提交查看。当前文档只描述可执行的现行流程；验证结果随对应版本或 output 日志保存。
