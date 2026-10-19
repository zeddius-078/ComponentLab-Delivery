# 2026-10-20 目录解耦记录

## 结论与边界

本次只整理维护输入、生成输出、示例和历史档案的位置，并调整生成入口；不修改组件视觉行为或 Token 值模型。Source 现保留 400 个维护文件，包含实现、生成工具、Token 源、契约、模板和运行所需的原始资源。生成代码也是构建产物，不能因为扩展名是 `.cpp` 或 `.h` 就继续放在 Source。

| 内容 | 当前位置 | 删除策略 |
| --- | --- | --- |
| 维护实现与权威输入 | `Source` | 不可删除 |
| 规范、历史审计、示例与消费测试 | `docs` | 不可删除 |
| 构建副本与全部新产物 | `build` | 可再生成 |
| 整理前构建目录与缓存 | `待删除-build-20261020` | 当前仍保存通过消费测试的旧 SDK；建议兼容性问题解决前保留 |

本归档的 `Source` 子目录含 451 个迁移前生成文件，仅用于追溯，不参与新构建。根目录原 `reports` 和 `.archive` 已分别归入 `docs/archive/reports` 与 `docs/archive/previous-root-archive`。示例迁至 `docs/examples/MappingWorkbench`，消费测试迁至 `docs/validation/SDKConsumerTest`。

## 构建方式

在项目根执行 `python -B Source/PrepareBuild.py`，将维护输入复制到 `build/work`，再在副本中生成 Token、原生组件和组件目录。执行 `Source/Publish-ComponentSDK.ps1` 完成编译及打包。输出路径受限于 build；不要直接修改副本，也不要从 Source 直接调用写文件生成器。

PrepareBuild 明确复制三个 docs 输入：组件追溯清单、Figma 结构审计和设计预算审计。后两者是历史验证证据，不是 Token 数值来源。保留 docs 是删除 build 后能够重建的前提。

## 验证结果

- 从空 build 准备及 SDK 编译成功；新 SDK 的功能验收未通过，不能视为可直接消费的稳定包。
- 目录边界测试 4/4 通过，Token 测试 39/39 通过，示例静态检查 13/13 通过。
- 项目结构检查通过，182 项 QRC 资源可解析；源码 SDK 打包成功。未验证该源码包在另一台机器上的完整编译。
- 完整发布与测试期间核对 Source/docs 文件 SHA-256，未发现运行回写。机器级清单与日志在 `build/reports`，可删除后重新生成。
- 两个并行任务已停止推进；最终验证不与其生成或发布并行。

### 未解决的功能问题

当前 SDK 的 `sdk_header_and_instance` 测试失败：

```text
Missing composite component layout token:
component.navigation-bar.base.state.default.slot.container.height
```

`Source/ComponentKit/runtime/component/Z_DesignComponents.cpp` 中导航栏仍读取该旧字段；当前 `component-recipes/navigation-bar.json` 使用 `utility-row`、`navigation-row`、`log-bar` 分区高度。旧 SDK 对同一消费测试通过，迁移前归档的运行时 Token 也包含旧字段。因此当前失败可定位为维护端 Token 结构与 C++ 消费契约不一致，而不是简单的资源路径丢失。

本次没有补旧字段、硬编码回退值或改写导航栏实现来掩盖失败；也未遍历修复其他组件，故不能断言仅有这一个不兼容点。下一步应单独对齐组件运行时与新 Token Contract 后重建，再重新执行消费测试。

## 回退与复测

旧构建目录未物理删除，历史输出也已保留。不要将历史生成物重新放回 Source。若仅需复测旧 SDK，可向 `docs/validation/SDKConsumerTest/Build.ps1` 的 `-SdkRoot` 指定 `待删除-build-20261020/sdk`，并为 `-BuildDirectory` 指定 build 下的独立目录。

所有构建和测试结果都应区分：目录边界通过、编译通过、运行时功能通过。前两者不能替代最后一项。
