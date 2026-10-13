# DesignEvidence 目标结构与职责

Raw 负责审计，Canonical 负责工程消费，Archive 保存历史。工程入口固定为 `packages/DesignTest-2-145/canonical/index.json`。

```text
DesignEvidence/
├─ README.md
├─ docs/                         五份交付文档与迁移清单
├─ schemas/                      11 个 version 1 JSON Schema
├─ tools/
│  ├─ capture.py                 只采集，保存原响应、请求和调用清单
│  ├─ normalize.py               只生成确定性 Canonical 并发布 current
│  ├─ verify.py                  只离线验证，不连接 Pixso
│  └─ contract_io.py             小型共享 JSON、哈希、路径和 Schema I/O
├─ tests/                        证据合同与采集回归
├─ packages/DesignTest-2-145/
│  ├─ current.json               机器可读权威 capture 和 index 哈希
│  ├─ canonical/                 九个稳定工程接口文件
│  └─ runs/<capture-id>/
│     ├─ raw/                    原字节响应及历史聚合响应
│     └─ manifest.json           语义角色、单次调用与三层哈希
└─ archive/DesignTest-2-145/      旧快照、探测、派生输出及历史工具
```

| 层/入口 | 职责 | 允许的读取者 |
|---|---|---|
| Raw | 保留返回、参数、时间证据、原生 ID 和哈希 | normalize、审计 verify、问题定位 |
| Canonical | 身份、结构、组件、变体、实际变量与溯源合同 | 生成器、映射适配器、工程校验 |
| Archive | 历史记录与明确回退 | 人工审计、迁移恢复 |
| current.json | 当前 capture 与 Canonical 一致性声明 | 发布/审计管理工具 |
| canonical/index.json | Schema、身份、artifact路径/哈希、计数和状态 | 工程唯一入口 |

Raw 不写归一化字段。历史文件移动前后逐字节哈希一致。采集序号只用于 call ID，不决定工程语义。现有聚合 Raw 文件保留原字节，新 Manifest 提供内部 JSON Pointer；未来一调用一响应文件。

新采集若为 SSE，保存原传输流及解析出的 JSON-RPC envelope，后者仅为传输提取表示；原流哈希仍可复核。网络失败只记录客户侧诊断，不伪造 Pixso error 响应。

工具不承担 Qt Mapping。共享 I/O 模块不提供插件系统或复杂框架；内置 Schema 校验器仅支持本包实际采用的标准 JSON Schema 关键字，遇到未知关键字明确失败。
