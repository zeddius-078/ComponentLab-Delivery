# DesignTest 设计证据

本目录保存 Pixso 文件 `DesignTest` 的页面 `2:145` 设计证据。采集过程只读取设计，不修改 Pixso 文件，也不改动三个 C++ 项目的运行链路。

设计源：[DesignTest，页面 2:145](https://pixso.cn/app/design/4S0Ktm81WnvzuJmZHvRuOg?file_type=10&icon_type=1&page-id=2%3A145)。文件标识为 `4S0Ktm81WnvzuJmZHvRuOg`。

## 这次读到了什么

| 数据 | 实测结果 | 证据位置 |
| --- | ---: | --- |
| 页面上的展示容器 | 11 个组件集、45 个实例 | `03-page-dsl.json` |
| 真正的组件状态组 | 11 个，含属性定义 | `03-page-dsl.json`、`11-state-group-dsl.json` |
| 页面内的组件变体 | 824 个，均找到 Pixso 原生 `component_key` | `07-all-components.json`、`design-evidence.json` |
| 文件当前可用变量 | 19 个组、5,602 个变量 | `05-variable-sets.json`、`06-variables.json` |
| 本地变量集合及模式名 | 5 个集合 | `09-variable-collections.json` |
| 本页引用的变量 | 39 个，出现 263 次绑定 | `03-page-dsl.json`、`design-evidence.json` |
| 本页变量的逐模式终值 | 312 条，全部解析成功 | `design-evidence.json` |
| 本地样式 | 本次返回空对象 | `08-local-styles.json` |

`03-page-dsl.json` 内的 `variableMap=290`、`variableSetMap=4` 是该次紧凑 DSL 的来源统计。`get_variables` 单独查询的是**当前文件可用的全部变量**，因此得到 19 个组、5,602 个变量。两个数字口径不同。`07-all-components.json` 还有另一个页面的 3 个组件，本页索引只计入 824 个。

## 为什么先前判断为缺失

原有转换脚本只提取一个 Button 变体，并把 `fileKey:componentSetId` 本地拼接值写作 `componentKey`。页面 DSL 实际已经含有 8 个状态组的属性和变体引用。另 3 个状态组需要读取真正的状态组 ID，不能拿画板上的展示容器 ID 调 `get_variants`。本次采集把两层身份拆开了。

首次连接本机 Pixso MCP 时，服务返回 `example-server`，但 `tools/list` 不可用；随后工具列表恢复，新的原生采集完成。这个现象不能证明变量缺失。Pixso 的[本地 MCP 说明](https://pre.pixso.cn/developer/en/mcp/local-mcp.html)要求目标设计文件在桌面端保持打开并处于活动标签；该条件可作为再次遇到空工具列表时的检查项。

## 如何查一条映射

以 Button 为例：

```text
文件 fileKey             4S0Ktm81WnvzuJmZHvRuOg
展示容器节点              3:5616  (01 / Button)
组件状态组                3:10558 (ComponentLab/Button)
变体节点                  3:5686  (PRIMARY / M / default / solid / light-comfortable)
变体原生 component_key    5db2a212943f439833dd3b8d6f8957d687a6edb1
页面实例节点              3:10591
```

这些字段来自 Pixso，不以名称拼接生成身份。开发侧可用 `componentSetId + variantProperties` 找变体，用原生 `component_key` 校验设计组件，再映射到 `Z_PushButton`。页面实例 ID 用于追溯布局中的那一次使用。展示容器 ID、状态组 ID、变体 ID、实例 ID 分别保存。

Button 主色也可逐跳复核：`contract/button/palette/primary` (`3:5226`) → `project/color/action/primary` (`3:3513`) → `stoken/color/interactive/brand/default` (`3:3483`) → 基础色。普通 `light-comfortable` 的终值为 `#6235F7`，`teal/light-comfortable` 为 `#1D4840`。`design-evidence.json` 保留每个模式的原始值、别名链、模式 ID 和模式名。Pixso 官方[变量 API](https://pre.pixso.cn/developer/en/plugin-api/variables/variables.html)提供本地变量及集合读取能力。

## 文件怎么用

| 文件 | 用途 |
| --- | --- |
| `capture.py` | 只读 MCP 采集、索引生成与校验工具；不依赖 PySide2 |
| `DesignTest-2-145/live-20261003/00` 至 `11` 号 JSON | Pixso MCP 的 JSON-RPC 返回内容；`manifest.json` 记录工具名、参数和文件哈希 |
| `DesignTest-2-145/live-20261003/design-evidence.json` | 为开发阅读整理的节点树、11 个组件契约、824 个变体 key、变量绑定与终值 |
| `DesignTest-2-145/live-20261003/manifest.json` | 原生响应和索引的 SHA-256 清单 |
| `DesignTest-2-145/page-2-145.source.json`、`variant-1.source.json` | 先前留存的快照原样拷贝，仅供对照；以 `live-20261003` 为本次权威证据 |
| `DesignTest-2-145/mcp-probe.json` | 先前工具列表未就绪时的连接探测，解释采集前后的差异 |

`design-evidence.json` 是本地确定性整理结果。其 `definitionSource`、`nativeKeySource` 和 `modeNameSource` 字段指出相应原生响应。原始 JSON-RPC 文件可以独立复核；本地索引不冒充 Pixso 原始输出。

## 验证与复采

在 PowerShell 中执行（可把 `python` 换为本机 Python 3.10 绝对路径）：

```powershell
历史命令（已失效）：`capture.py` 已随旧 Pixso 链路归档，不应从当前工程重新执行。证据文件现位于 `docs/archive/design-evidence/legacy-pixso/archive/`。
```

需要重新采集时，在 Pixso 桌面端打开并激活 DesignTest，再给 `--output` 一个新的空目录，依次执行：

```powershell
python capture.py --fresh --output <新目录>
python capture.py --read-collections --output <新目录>
python capture.py --capture-state-groups --output <新目录>
python capture.py --capture-state-group-dsl --output <新目录>
python capture.py --build-live --output <新目录>
python capture.py --verify --output <新目录>
```

采集包可作为 `DesignToCode_GenerateComponent` 的输入。它目前没有自动改写 Qt 组件注册表、Token 编译器或 SDK；这些工程映射应以这份证据为依据单独实施和验证。
