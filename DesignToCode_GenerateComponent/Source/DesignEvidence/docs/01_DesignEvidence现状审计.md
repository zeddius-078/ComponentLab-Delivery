# DesignEvidence 现状审计

本审计基于迁移前逐文件读取、源码搜索及 SHA-256 核验。原根目录为 `Source/DesignEvidence`，未建立第二套 PixsoDesignEvidence。

## 实际规模与来源

迁移前 21 个文件，共 15,807,114 字节（15.07 MiB）。live 批次 14 个文件，共 12,985,550 字节（12.38 MiB）。live Manifest 已登记的 13 个文件哈希全部匹配。

| 原路径（相对证据根目录） | 字节数 | 分类 | 职责 |
|---|---:|---|---|
| `capture.py` | 33,100 | Tool | MCP连接、采集、索引、Alias、验证混合工具 |
| `DesignTest-2-145/design-evidence.json` | 1,207,650 | Derived / Historical | 合并节点、组件、变量、解析值的派生索引 |
| `DesignTest-2-145/live-20261003/00-initialize.json` | 313 | Raw / Current legacy | MCP原始响应 |
| `DesignTest-2-145/live-20261003/01-tools-list.json` | 57,645 | Raw / Current legacy | MCP原始响应 |
| `DesignTest-2-145/live-20261003/02-file-identity.json` | 175 | Raw / Current legacy | MCP原始响应 |
| `DesignTest-2-145/live-20261003/03-page-dsl.json` | 547,568 | Raw / Current legacy | MCP原始响应 |
| `DesignTest-2-145/live-20261003/04-button-variant-dsl.json` | 157,462 | Raw / Current legacy | MCP原始响应 |
| `DesignTest-2-145/live-20261003/05-variable-sets.json` | 4,535,468 | Raw / Current legacy | MCP原始响应 |
| `DesignTest-2-145/live-20261003/06-variables.json` | 4,535,124 | Raw / Current legacy | MCP原始响应 |
| `DesignTest-2-145/live-20261003/07-all-components.json` | 972,347 | Raw / Current legacy | MCP原始响应 |
| `DesignTest-2-145/live-20261003/08-local-styles.json` | 137 | Raw / Current legacy | MCP原始响应 |
| `DesignTest-2-145/live-20261003/09-variable-collections.json` | 2,574 | Raw / Current legacy | MCP原始响应 |
| `DesignTest-2-145/live-20261003/10-state-group-variants.json` | 42,156 | Raw / Current legacy | MCP原始响应，三次调用聚合 |
| `DesignTest-2-145/live-20261003/11-state-group-dsl.json` | 226,835 | Raw / Current legacy | MCP原始响应，三次调用聚合 |
| `DesignTest-2-145/live-20261003/design-evidence.json` | 1,902,282 | Derived / Current legacy | 合并节点、组件、变量、解析值的派生索引 |
| `DesignTest-2-145/live-20261003/manifest.json` | 5,464 | Metadata / Current legacy | 采集/装配清单与哈希 |
| `DesignTest-2-145/manifest.json` | 1,132 | Metadata / Historical | 采集/装配清单与哈希 |
| `DesignTest-2-145/mcp-probe.json` | 60,386 | Historical / Diagnostic | 早期连接探测 |
| `DesignTest-2-145/page-2-145.source.json` | 1,178,430 | Historical / Preserved snapshot | 旧页面或变体快照 |
| `DesignTest-2-145/variant-1.source.json` | 335,095 | Historical / Preserved snapshot | 旧页面或变体快照 |
| `README.md` | 5,771 | Documentation | 说明与人工权威路径 |

完整 SHA-256、来源、读取者与最终位置见 `migration-inventory.json`。当前 live 的权威关系原先只在 README 中解释，缺少工程入口。

## 已确认的问题与统计纠正

- get_variable_sets 与 get_variables 的唯一 JSON 正文逐字相同；前者附带额外说明内容块，因此整个文件及 MCP payload 哈希不同，主体哈希相同。正文需去重，原响应不能合并冒充一次调用。
- 两个状态组聚合文件各包含三次调用，原 Manifest 未提供单次响应定位。新清单以 `/0/response`、`/1/response`、`/2/response` 独立定位，并分别计算响应、payload、主体哈希。
- 旧索引中的 900 个节点包含页面 898 个节点与补充 Button 变体 2 个节点。
- 39 是页面 refsIndex 声明变量数；实际页面绑定只涉及 32 个变量。补充变体的 7 个引用变量全部包含于这 32 个中，并不是另外 7 个。
- 263 次绑定 = 页面 256 次 + 补充变体 7 次；312 条旧终值包含未绑定的声明变量，实际使用变量的终值为 256 条。
- 页面状态组 11 个，实例 45 个，Variant 原生 key 824 个。全量可用变量 5,602 个，继续保留在 Raw，不进入页面合同。

## 真实消费者

源码和配置检索未发现工程直接读取此证据目录；原读取者为 capture.py 内部 build/verify。

MappingWorkbench 存在独立的 Pixso Button snapshot/variant 转换器和测试。它曾将 fileKey:componentSetId 拼接值写为 componentKey，并把 Variant ID 写为 instanceNodeId。本轮改为 Canonical-only 转换，并修正原生 key 与身份种类。

Qt 元数据目录与 ComponentKit Token 编译链使用各自合同；本轮不替换其 Token 权威源、不扩展全部组件转换能力。Figma 示例继续使用其独立输入。

## 证据边界

历史单次调用没有采集时间，保留 null。批次时间来自旧 Manifest，不能当作每次补充调用时间。initialize、tools/list、file identity 的参数由保留下来的采集脚本恢复，并明确标记 metadataSource；它们不是历史请求报文实录。新采集保存请求原文。

本轮没有重连 Pixso、没有现场复采，也没有验证真实设备运行。集合中文名称原样保留，未发现 U+FFFD 替换字符。
