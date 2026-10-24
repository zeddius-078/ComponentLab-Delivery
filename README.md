# ComponentLab Delivery

这是一个包含三个 Qt 项目的工作区：组件生产、组件展示和页面生成。根目录工具负责协调它们的检查、构建、验证和打包。

当前上传为开发源码快照，包含自研工具完整源码和固定输入，不包含 SDK、浏览器等运行二进制。完整离线包尚未作为本次快照附件发布；使用端不能仅凭 clone 执行生成，也不应自行安装依赖。待交付材料的版本、位置、清单指纹和阻塞项见[工具与交付依赖](docs/designRules/architecture/工具与交付依赖.md#github-源码与运行资产领取)。

## 先看哪里

| 目标 | 入口 |
| --- | --- |
| 了解三个项目的关系 | [docs/README.md](docs/README.md) |
| 第一次配置开发机 | [docs/tutorial/三项目上手指南.md](docs/tutorial/三项目上手指南.md) |
| 使用已有离线包 | [docs/tutorial/离线交付.md](docs/tutorial/离线交付.md) |
| AI Agent 执行跨项目任务 | [docs/mainSkill/design-to-qt-workspace/SKILL.md](docs/mainSkill/design-to-qt-workspace/SKILL.md) |
| Agent 执行设备基线 / SKU | [agent-execution-capability-profile.json](docs/designRules/architecture/agent-execution-capability-profile.json) |
| 工作区构建入口 | [Source/Workspace.ps1](Source/Workspace.ps1) |
| 原生 Vue 导出件转 Qt 页面 | [第二路径说明](DesignToCode_GenerateLayout/docs/Pixso原生DSM-D2C到Qt第二路径说明.md) |

## 三个项目

| 项目 | 工作内容 | 主要消费者 |
| --- | --- | --- |
| [DesignToCode_GenerateComponent](DesignToCode_GenerateComponent/README.md) | 维护组件身份、Token、合同和 `Z_*` 实现，发布唯一权威 `GeneratedComponentSDK` | ComponentLab、GenerateLayout、业务 Qt 工程 |
| [ComponentLab](ComponentLab/README.md) | 消费 SDK，展示组件外观、状态和对象树，执行展示回归 | 设计评审和组件验收 |
| [DesignToCode_GenerateLayout](DesignToCode_GenerateLayout/README.md) | HTML＋DSL 与原生 Vue 导出静态转换分别维护，消费 SDK 生成 Qt 页面文件 | 业务 Qt 工程 |

依赖关系固定为：

```text
DesignToCode_GenerateComponent
        │  GeneratedComponentSDK
        ├──────────────→ ComponentLab
        └──────────────→ DesignToCode_GenerateLayout
                                  │
                                  └──→ .ui / .h / .cpp / .qss / .qrc
```

## 目录职责

三个项目均使用以下目录；每个项目的 README 说明具体入口。

| 目录 | 内容 | 处理方式 |
| --- | --- | --- |
| `Source/` | 可维护源码、脚本、配置和编译器实现 | 修改从这里开始 |
| `docs/` | 现行规则、教程、固定输入、示例和验证源码 | 作为维护输入保留 |
| `build/` | 受控 SDK、稳定工具、固定运行依赖和发布状态 | 由发布流程更新；不作为普通缓存删除 |
| `output/` | 编译工作区、预览、日志和页面交付批次 | 按批次或工作区清理 |

根目录的 `Source/` 是工作区编排层，不属于第四个业务项目。它只负责调用三个项目已有入口，不复制组件或布局实现。

## 受控 Agent 执行基线

当前生成与编码输出的受控执行基线是 **DeepSeek V4 Flash 256K Non-Think**，SKU 为 `DS-V4F-BUILDUNK-NT-C256K-TEXT-AGENT-PROVIDERUNK`。完整能力画像、适用范围和不可自主决定的事项以 [agent-execution-capability-profile.json](docs/designRules/architecture/agent-execution-capability-profile.json) 为唯一事实源。

该基线适合读取、提取、规则明确的配置或代码修改，以及有自动校验兜底的批处理；不承担跨模块架构决策、未测试的长链自主修复、缺失数据补全或把推断写成事实。生成任务必须提供冻结的输入、路径、模式、组件库 Profile、映射和输出批次；失败时保存证据并停止，不改写 DSL、不临时新增解析器/发射器，也不自动切换 SDK、Profile 或环境。新发布工具的验收记录会绑定该 SKU 与画像哈希；旧副本若没有该标签，只能视为未标注旧副本，不能宣称符合此基线。

## 开发机入口

按阶段使用已经安装的 Python 3.9、Qt 5.15.2、MSVC、CMake、Ninja、Node 和 Playwright。工具路径通过参数或当前进程环境变量提供，工作区不会安装或升级依赖。以下是工作区开发编排，不是所有转换任务的必经重建：已有 SDK 的原生 Vue 静态转换走第二路径独立入口，不需要运行 Vue 应用、Playwright 或浏览器，也不自动重建组件。

```powershell
pwsh -File Source/Workspace.ps1 -Action doctor
pwsh -File Source/Workspace.ps1 -Action build -Target all
pwsh -File Source/Workspace.ps1 -Action verify
```

SDK 由 GenerateComponent 生成后放在 `DesignToCode_GenerateComponent/build/sdk`。ComponentLab 和 GenerateLayout 读取该目录中的 manifest、公开头文件、静态库和映射资源。

## 离线使用

离线使用需要完整交付包。运行包自带解析器、SDK、Python、Node、Playwright、匹配浏览器、Qt/VC 运行库和许可清单。使用方式见[离线交付](docs/tutorial/离线交付.md)。

原生 Vue 第二路径当前从维护源码运行，解析依赖尚未纳入受控工具包；不能据上述离线包定义推断它已经获得离线发布或新机器验收。

## 事实口径

- `Source` 是维护源码的事实源。
- `docs` 中的固定输入必须带来源和版本信息。
- `build` 中的 SDK 和稳定工具只有在对应清单与验证记录通过后才可交付。
- `output` 中的日志和过程文件用于本次执行，不作为新的源码输入。
- 结构、编译、运行、视觉和新机器验收分别记录，任一结果只代表对应阶段。
