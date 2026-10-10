# DesignEvidence

当前权威工程数据 → [canonical/index.json](packages/DesignTest-2-145/canonical/index.json)；当前权威原始证据 → [current.json](packages/DesignTest-2-145/current.json) 指向的 run；历史证据 → `archive/`。

工程从 index.artifacts 按语义读取合同，不读取 Raw 文件、固定日期或采集序号。Raw 原字节只用于归一化和显式审计。capture、normalize、verify 分别负责采集、标准化和离线校验；README 不决定版本或当前 capture。

## 使用方式

在本目录执行。复采前在 Pixso 桌面端打开并激活目标文件，选择新的空 run 目录；本轮迁移没有执行现场复采。

```powershell
python -B tools/capture.py --file-key 4S0Ktm81WnvzuJmZHvRuOg --page-id 2:145 --variant-id 3:5686 --output packages/DesignTest-2-145/runs/<新capture-id>
python -B tools/normalize.py --manifest packages/DesignTest-2-145/runs/<新capture-id>/manifest.json --output packages/DesignTest-2-145/canonical
```

Normalize 只在完整验证后发布 Canonical 与 current。相同 Raw 输入得到相同文件字节；失败不覆盖上一有效合同。

从机器记录选择原始证据并审计：

```powershell
$package = 'packages/DesignTest-2-145'
$current = Get-Content -Raw -LiteralPath "$package/current.json" | ConvertFrom-Json
python -B tools/verify.py --index "$package/canonical/index.json" --manifest "$package/$($current.sourceManifest)" --current "$package/current.json"
python -B -m unittest discover -s tests -p test_contract.py
```

verify 不连接 Pixso。只需要工程合同校验时省略 --manifest 和 --current，仍会检查 Schema、artifact 哈希和跨文件关系。工具只用 Python 标准库；Schema 是标准 draft-2020-12，可另用兼容校验器检查。

## 交付文档

- [01 现状审计](docs/01_DesignEvidence现状审计.md)
- [02 目标结构与职责](docs/02_DesignEvidence目标结构与职责.md)
- [03 Canonical 字段说明](docs/03_CanonicalDesignContract字段说明.md)
- [04 迁移方案与实施记录](docs/04_DesignEvidence迁移方案.md)
- [05 工程消费接口](docs/05_工程消费接口说明.md)
- [迁移前清单、哈希与最终位置](docs/migration-inventory.json)

实际页面绑定使用 32 个变量；补充 Button 变体使用其中 7 个。263 次绑定分为页面 256 次、补充变体 7 次；实际使用变量共 256 条逐模式终值。旧 39 个声明变量和 312 条终值继续保存在归档索引，不作为工程实际引用计数。
