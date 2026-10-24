# DesignToCode_GenerateLayout

## 项目定位

本项目消费已发布的公共组件 SDK，生成 Qt 页面结构、布局、实例初始化、页面 QSS 和资源。当前有两条相互隔离的输入路径，不是任意设计稿或 Vue 工程的通用转换器。

| 输入与任务 | 入口 | 能力边界 |
| --- | --- | --- |
| 同版 HTML＋Pixso DSL＋layoutReference | `Source/HtmlDsl/Generate.ps1` | 浏览器适配与正式页面生成；遵守该路径完整输入合同 |
| Pixso 原生 Vue 导出 ZIP / SFC＋CSS＋资源 | `Source/pixso_native/generate_page.py` | 静态转换与 SDK 适配候选；详见[第二路径说明](docs/Pixso原生DSM-D2C到Qt第二路径说明.md) |

Vue 导出件不能直接传给 HTML/DSL 生成器。第二路径不调用第一路径编译器，也不要求启动 Vue 3、Vite、Playwright 或浏览器；但必须同时提供与导出件同批的 Pixso DSL，用于恢复设计身份、节点关系和可读命名。缺少 DSL 或根节点不匹配时拒绝生成。该路径仍依赖显式指定的语法解析模块及 SDK，其依赖尚未全部纳入受控交付，不承诺单项目压缩后换机即用。

第二路径目前可复现的是已核对附件到页面源码的转换。该入口直接消费本地 Vue 文件、映射和 SDK，不要求在线 DSM、MCP 或 ChatGPT。上游六族 DSM 配置仅通过本地配置检查，当前账号的带配置导出被企业权限限制；不能把本地页面交付解释为最新线上设计已经通过，也不要求上传受保密约束的资料。完整参数、复验条件和限制统一维护在第二路径说明中，不以 README 或 Skill 的更新代替实际验收。两条路径均属于项目必须保留的能力，但输入合同、生成器和验收结果彼此隔离；第二路径的“可用”不等于第一路径的 HTML＋DSL 正式页面合同已满足。

| 目录 | 内容 |
| --- | --- |
| `Source` | 采集、适配、布局转换、页面编译、构建和校验源码 |
| `docs` | 规则、固定输入、样例、教程和验证代码 |
| `build` | 经过构建发布的稳定工具和固定运行材料 |
| `output` | 当前执行产生的缓存、页面包和日志 |

## 第一路径输入与输出

```text
Pixso DSL + D2C HTML + layoutReference
       -> capture manifest
       -> browser adapter
       -> component / layout compiler
       -> .ui + .h + .cpp + .qss + .qrc
       -> uic / consumer build / runtime
```

HTML 提供布局和文字测量，DSL 提供设计身份、节点关系、属性、状态和 Token 引用。capture manifest 保存二者的版本、路径和哈希。正式页面还需要布局证据、字体、资源和已发布 SDK。

组件源码、公共 QSS、Token 和工厂由 GenerateComponent 维护。ComponentLab 使用同一 SDK 展示状态。GenerateLayout 只生成页面文件和页面级资源。

## 入口与执行目录

以下开发编排命令从 **GenerateLayout 项目根目录**执行；编排脚本实际位于上一级工作区的 Source，不在本项目 Source 中。环境缺项只报告，不安装或升级。

```powershell
pwsh -File ../Source/Workspace.ps1 -Action doctor -Target layout
pwsh -File ../Source/Workspace.ps1 -Action build -Target layout
pwsh -File ../Source/Workspace.ps1 -Action verify -Target layout
```

第一路径正式页面：

```powershell
pwsh -File Source/HtmlDsl/Generate.ps1 `
  -CaptureManifest '<capture.json>' `
  -MappingDirectory '<映射目录>' `
  -Font '<字体文件>' `
  -Batch '<批次>' -Page '<页面名>'
```

第一路径正式页面包写入 `output/delivery/<批次>/<页面>`；稳定工具发布在 `build/tools/layout`。第二路径目前直接从 Source 执行，生成与消费者验证写入 `output/work/pixso-native/<批次>`。最近保留的页面源码交付为 `output/delivery/pixso-native-20261022-03`，批次旁置说明、清单、追踪记录和截图；尚未接入上述稳定工具和自动交付入口。该批次是限定范围的页面源码包，不能把 `STAGED` 或编译通过等同于视觉验收或免安装工具发布。

## 回归与清理边界

正式回归按既有测试版本执行：V1 页面包、V2 Qt/离线运行验证，以及 Python 3.9 的 `docs/validation/check_delivery.py`。回归产生的中间文件和日志只允许进入 `output/work`、`output/logs`；成功交付写入明确批次的 `output/delivery`。`build` 不是普通缓存，包含受控工具、SDK 和运行材料，不得用普通清理删除。

完成回归后，仅使用以下入口清理可重建内容，不触碰 `build` 或未指定的交付批次：

```powershell
py -3.9 -B ../Source/ArtifactLifecycle.py clean --project (Get-Location)
py -3.9 -B ../Source/ArtifactLifecycle.py clean --project (Get-Location) --delivery <批次>
```

第一条清理 `output/work` 与 `output/logs`；第二条只清理明确指定的交付批次。清理失败时入口会把目标标记为待删除，而不会静默改动受控工具。

## 阅读顺序

1. [Skill 执行合同](docs/mainSkill/generate-layout-pipeline/SKILL.md)
2. [布局转换规则](docs/designRules/layout/布局转换器通用化规范.md)
3. [组件库 Profile](docs/designRules/library-profiles.md)
4. [上手指南](docs/tutorial/布局生成上手指南.md)
5. [验证代码说明](docs/validation/README.md)
6. 原生 Vue 附件任务直接读取[第二路径说明](docs/Pixso原生DSM-D2C到Qt第二路径说明.md)，不套用第一路径的采集步骤

Python 命令使用已安装的 Python 3.9。依赖路径由当前进程参数或环境变量提供；脚本不会修改系统环境。

## 自动化输出基线

本项目的受控执行 SKU 为 `DS-V4F-BUILDUNK-NT-C256K-TEXT-AGENT-PROVIDERUNK`（DeepSeek V4 Flash 256K Non-Think）。该设备只执行已冻结的输入、路线、`GenerationMode`、SDK、Profile、mapping、批次与验收命令；不能猜测缺失 DSL/几何/组件身份，不能注入设计值或新建解析器、`.ui` 发射器。失败必须保留证据并停止，详情见工作区 [能力画像](../docs/designRules/architecture/agent-execution-capability-profile.json)。
