# CanonicalDesignContract 字段说明

所有合同的 schemaVersion 为整数 1。JSON 使用 UTF-8、键排序、两空格缩进和结尾换行，禁止 NaN/Infinity。generatedAt 使用 Raw Manifest 固定记录的 UTC 时间，不使用 Normalize 的运行时间。

## 唯一入口与权威记录

| index 字段 | 含义 |
|---|---|
| schemaVersion / packageId | 合同版本和页面包身份 |
| provider / fileKey / pageId | Pixso 文件与页面身份 |
| captureId / generatedAt | 原始采集标识和可追溯的固定时间 |
| sourceManifest / sourceManifestSha256 | 相对包目录的 Raw Manifest 路径和原字节哈希 |
| artifacts | 每项的相对 canonical 路径、bytes、sha256、schema |
| counts | 页面与补充变体分口径统计，不沿用旧声明数 |
| validationStatus | 通过校验后为 passed；失败不能发布 |

current.json 保存 packageId、captureId、sourceManifest、sourceManifestSha256、canonicalIndex、canonicalIndexSha256 和 schemaVersion。它不取代工程入口，也不通过“最新日期”猜测 run。

## 文件职责与实体字段

| 文件 | 主字段/规则 |
|---|---|
| design-identity | provider/fileKey/pageId；componentSets(componentSetId,mappingKey)；variants(componentSetId,variantNodeId,nativeComponentKey)；instances(instanceNodeId,componentSetId,variantNodeId,nativeComponentKey,locator) |
| page-structure | boundsSpace=parent-local；rootLocators；nodes；独立 variantExamples。节点保留 locator/nodeId/parentLocator/children/type/name、必要 bounds/layout/text/visible 及组件引用 |
| components | 当前引用状态组的 componentSetId/designName/properties/variantNodeIds/mappingKey |
| component-variants | componentSetId/variantNodeId/name/properties/nativeComponentKey，824 项独立索引 |
| variables-used | variables 与 bindings。实际 32 个变量，页面 256 次、补充变体 7 次绑定 |
| variable-collections | collectionId/name/defaultModeId/modes(modeId,name)，仅引用及 Alias 依赖需要的四个集合 |
| provenance | sourceManifest/manifestSha256/calls/mappings，独立承担溯源 |
| validation | status 与 schema/identity/components/variables/alias/hash/provenance 机器检查状态 |

mappingKey 是本地稳定连接键 `fileKey:componentSetId`，绝不表示 Pixso 原生 key。状态组缺少原生 key 时不借用某个 Variant key。实例未明确返回 mainComponent 时 variantNodeId/nativeComponentKey 为 null，不按名称猜测。

没有原生 ID 的子节点 nodeId=null，locator 使用 callId + decoded-body JSON Pointer。它是本次证据内的结构定位符，不能替代跨采集原生 ID。children 和 parentLocator 使用同一定位规则。

源未返回 visible、几何或布局字段时保持缺失，不补默认零值。Token 引用仍以原引用字符串保留，并在 bindings 中提供可解释的拆分结果。propertyPath 为节点内 JSON Pointer。

变量字段为 variableId/name/collectionId/nativeVariableKey/resolvedType/scopes/modes。每个 mode 保存 modeId/modeName/rawPayload/aliasChain/finalValue/status。Alias 每跳保存原变量 ID、名称、collectionId、modeId、rawValue。

跨集合 Alias 仅按已返回的唯一、完全相同模式名连接；这是项目解析规则，不冒充 Pixso 自动模式选择。重复模式名、模式缺失、Alias 环、目标缺失或无法确认集合模式时明确失败，不使用默认模式降级。

## Provenance 与哈希

mappings 每条包含 artifact、Canonical pointer、rule 和 sources。每个 source 包含 callId 与 decoded-body bodyPointer；callId 连接到 calls 中的 tool/arguments/response.path/response.pointer 和哈希。对象级映射覆盖其子字段；逐模式解析列出每一跳变量和集合来源。常量字段有明确规则和包级 Manifest 来源。

| 哈希 | 判定对象 |
|---|---|
| files[].sha256 | Raw 文件原字节 |
| calls[].response.sha256 | 单次 JSON-RPC response 对象的确定性 JSON |
| calls[].payloadSha256 | 完整 MCP result，包含附加说明块 |
| calls[].bodySha256 | 唯一 JSON 内容块解析后的确定性主体 |
| artifacts[].sha256 | Canonical 文件原字节 |

Raw 响应内 pointer 与 decoded-body bodyPointer 属于不同层次，不能直接混用。所有路径受相对路径边界检查；工程读取器只展开 index.artifacts，绝不沿 sourceManifest 自动进入 Raw。
