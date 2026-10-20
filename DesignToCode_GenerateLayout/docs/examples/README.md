# 固定样例

本目录保存可重复使用的 Qt 示例工程、模板、字体和技术夹具。执行输出写入项目 `output`。

| 路径 | 内容 |
| --- | --- |
| `Qt` | V2 页面运行与回归工作台 |
| `offline-capture` | 手写 HTML、DSL 和布局规格的链路样例，来源标记为 `synthetic-example` |
| `resources` | 示例字体等固定资源 |
| `styles` | 结构预览的初始样式 |
| `templates` | 示例工程和验证模板 |

运行入口：

```powershell
pwsh -File Source/Build.ps1 -Test
pwsh -File docs/examples/Qt/V2/Build.ps1 -Test
```

样例通过只说明样例输入和对应阶段通过。真实 Pixso 页面、完整组件覆盖和视觉签核需要独立证据。

[返回文档索引](../README.md)
