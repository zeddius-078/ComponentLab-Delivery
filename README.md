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
| 工作区构建入口 | [Source/Workspace.ps1](Source/Workspace.ps1) |

## 三个项目

| 项目 | 工作内容 | 主要消费者 |
| --- | --- | --- |
| [DesignToCode_GenerateComponent](DesignToCode_GenerateComponent/README.md) | 维护组件身份、Token、合同和 `Z_*` 实现，发布唯一权威 `GeneratedComponentSDK` | ComponentLab、GenerateLayout、业务 Qt 工程 |
| [ComponentLab](ComponentLab/README.md) | 消费 SDK，展示组件外观、状态和对象树，执行展示回归 | 设计评审和组件验收 |
| [DesignToCode_GenerateLayout](DesignToCode_GenerateLayout/README.md) | 合并同版 HTML + DSL，解析布局并生成 Qt 页面文件 | 业务 Qt 工程 |

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

## 开发机入口

使用已经安装的 Python 3.9、Qt 5.15.2、MSVC、CMake、Ninja、Node 和 Playwright。工具路径通过参数或当前进程环境变量提供，工作区不会安装或升级依赖。

```powershell
pwsh -File Source/Workspace.ps1 -Action doctor
pwsh -File Source/Workspace.ps1 -Action build -Target all
pwsh -File Source/Workspace.ps1 -Action verify
```

SDK 由 GenerateComponent 生成后放在 `DesignToCode_GenerateComponent/build/sdk`。ComponentLab 和 GenerateLayout 读取该目录中的 manifest、公开头文件、静态库和映射资源。

## 离线使用

离线使用需要完整交付包。运行包自带解析器、SDK、Python、Node、Playwright、匹配浏览器、Qt/VC 运行库和许可清单。使用方式见[离线交付](docs/tutorial/离线交付.md)。

## 事实口径

- `Source` 是维护源码的事实源。
- `docs` 中的固定输入必须带来源和版本信息。
- `build` 中的 SDK 和稳定工具只有在对应清单与验证记录通过后才可交付。
- `output` 中的日志和过程文件用于本次执行，不作为新的源码输入。
- 结构、编译、运行、视觉和新机器验收分别记录，任一结果只代表对应阶段。
