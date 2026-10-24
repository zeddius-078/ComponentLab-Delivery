# GenerateLayout 文档

现行文档只描述当前规则和入口。历史版本通过 Git 提交追溯；机器报告写入可再生的 output/logs，不再维护 docs/archive 副本。固定夹具和回归测试不是历史垃圾。

| 分区 | 用途 | 入口 |
| --- | --- | --- |
| tutorial | 操作、输出要求与验证路径 | [操作指南索引](tutorial/README.md) |
| mainSkill | AI 执行边界与流程 | [技能入口说明](mainSkill/README.md) |
| designRules | 布局、组件、映射与交付规范 | [规范索引](designRules/README.md) |
| examples | 示例源码、模板、字体和手写技术样例 | [示例说明](examples/README.md) |
| fixtures | 长期维护的固定输入与映射配置 | [固定输入说明](fixtures/README.md) |
| validation | 可执行的检查与测试源码，不是测试报告 | [验证说明](validation/README.md) |

`docs` 同时承担说明文档和维护输入的职责。映射配置位于 `fixtures/layout/mapping`，并非生成器临时输出；固定输入与测试代码不能仅因位于 docs 就作为历史材料删除。执行日志应写入 `output/logs`，新采集和临时案例应先进入 `output/work`。

遵循工作区 AGENTS.md：不自主安装或升级已有环境，Python 显式使用已安装的 3.9。Source 保存唯一维护实现，docs 保存规范、固定输入、示例及验证源码，build 保存受控工具和固定运行依赖，不允许普通清理删除。编译、采集与预览写入 output/work，交付写入 output/delivery，检查写入 output/logs。开发仓库使用工作区 `Source/Workspace.ps1` 的 doctor/build/verify/package/clean；离线包使用自身 Run.cmd，不要求部署机重建 SDK。

## 专项规范

- [原生 Vue 导出到 Qt 第二路径](Pixso原生DSM-D2C到Qt第二路径说明.md)：独立入口、依赖、配置、命令和当前验收范围；不要求 Vue 应用或浏览器运行
- [多组件库与前缀切换](designRules/library-profiles.md)
- [设计到布局生成说明](mainSkill/generate-layout-pipeline/references/设计到布局生成说明.md)
- [布局生成执行与验收规则](designRules/workflow/布局生成执行与验收规则.md)
- [Qt 页面交付边界](mainSkill/generate-layout-pipeline/references/page-delivery/Qt页面交付边界.md)
- [布局转换器通用化规范](designRules/layout/布局转换器通用化规范.md)
- [Qt UI 布局规则](designRules/layout/Qt-UI布局规则.md)

第一路径固定输入和映射在 docs/fixtures/layout，通过 LibraryProfile 选库，默认 token-sdk；analysis-system 仅是 planned 类名合同。第二路径的维护映射位于 Source/pixso_native，通过显式 `--mapping`、`--sdk`、`--profile` 选择，不使用第一路径的 LibraryProfile 入口。模板与样例在 docs/examples，测试在 docs/validation。正式、适配候选和结构预览分开验收，不能由 uic 通过推断目标实现存在。

旧采集清单必须重新核对哈希；导航栏 Token 消费契约问题仍按用户决定挂起。历史测试数字不作为当前结论。
