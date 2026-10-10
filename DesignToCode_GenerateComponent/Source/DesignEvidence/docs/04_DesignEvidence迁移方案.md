# DesignEvidence 迁移方案与实施记录

迁移按“盘点 → 生成 → 消费者切换 → 拆工具 → 归档 → 最终验证”进行。旧 Raw 在 Canonical 与消费者回归通过前保持原位置。

1. 保存迁移前 21 个文件的路径、字节数、SHA-256、职责与读取者。
2. 原字节复制当前 live Raw 到 runs/import-20261003/raw，保留旧聚合文件；生成新的 Raw Manifest，把三次聚合调用逐项定位。历史参数恢复来源显式标记，单次时间不补造。
3. 由 Raw Manifest 构建九个 Canonical 文件，重算实际绑定口径。工程变量来源明确采用 variables-full，两份重复主体只保留一次投影。
4. Button 适配器接受 index 和既有规则；生成真实 Variant key 的 Qt contract，不再接受 Raw snapshot。Qt 校验接受 pixsoCanonical，并移除旧 pixsoNormalized 正式路径。
5. 将原 capture 工具原样归档；正式 tools/capture、normalize、verify 分责。Figma generic snapshot 工具只处理 Figma，取消旧 Pixso DSL 分支。
6. 全部有关回归通过后，把当前原响应移入 run 原字节路径，旧 snapshot、probe、derived、旧 Button 输入与旧工具移入 archive；迁移清单记录对应最终路径。
7. 归档后运行完整 Raw 溯源验证与 Canonical-only 工程回归。

## 发布和回退

Normalize 在包内临时目录生成并进行 Schema、合同关系与 Raw 重现验证。通过后切换 canonical 目录，并以临时文件 replace 发布 current.json；发布异常恢复旧 canonical 和 current。相同输出为 no-op，不制造重复历史副本。

目录与 current 是两个文件系统操作，不能宣称跨文件事务原子性。消费者若遇到发布窗口中文件缺失或哈希不匹配，应失败并重新打开 index，不能回退读取旧 Raw。进程被强制终止留下 `.canonical-previous` 时工具拒绝自动覆盖，先恢复上一版目录和归档中的 current，再重试。

旧 Canonical 发布快照保存在 archive/canonical-history；完整迁移可依据 migration-inventory.json 的原路径与最终路径反向移动文件。归档是保留证据，不是删除证据。恢复旧工程消费者代码需同时恢复其对应测试和 Qt 来源校验，不能只切 JSON 路径。

## 验收证据

证据合同回归覆盖确定性、全部 Raw 哈希、单次调用定位、重复主体、身份、绑定、Alias、模式、路径边界、provenance，以及模拟采集的原字节响应/SSE/网络失败审计。Button 测试覆盖 Canonical 隔离消费、原生 key、评审哈希和旧输入拒绝。

Qt 四套主题回归新增 Canonical→Python→Qt→预览完整路径，在测试临时目录模拟哈希评审；测试结果不是用户人工评审记录。正式生成器仍要求先检查 proof，再提供它的 SHA-256。

本轮不重新采集，不自动发布 SDK，不修改 Token 数值或组件运行逻辑。最终命令结果另记在本文末尾。

## 最终执行结果

| 检查 | 实际结果 |
|---|---|
| tools/verify.py --index --manifest --current | passed；Raw、合同与权威记录一致 |
| 重复运行 Normalize | 九个文件逐字节相同 |
| 迁移前文件完整性 | 原证据包 21 个 + 旧 Button 5 个，共 26 个全部哈希一致 |
| Python 证据合同回归 | 18 项通过 |
| Python Button 消费回归 | 6 项通过，包括只提供 Canonical 的隔离输入 |
| MappingWorkbench Build.ps1 -Test | 构建成功；10 个 CTest 套件全部通过，包含四套主题的 Canonical 集成回归 |
| 本轮维护 Python 语法、交付文档链接与代码块 | passed |
| CheckProject.py | FAIL：原有 docs/Qt表格设计声明与原生映射.md 含跨项目 Markdown 链接，违反该检查的单项目复制边界；该文件未在本轮改动 |

项目总检查的失败已单独向用户询问是否扩展修复范围；本轮尚未修改该无关链接。证据治理、归档完整性和工程消费回归均已通过，但不能将项目总检查报告为通过。

最终工程读取路径是 Canonical index；旧 Pixso snapshot 转换、旧编号 Raw 读取与旧独立采集入口均已退出正式消费者。原始响应未被人工修订，旧派生文件仍保存在 archive。
