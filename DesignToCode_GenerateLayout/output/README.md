# 执行结果

`output` 保存本项目运行产生的临时数据、页面交付物和日志。源码、固定输入和稳定工具分别位于 `Source`、`docs` 和 `build`。

| 子目录 | 内容 | 处理方式 |
| --- | --- | --- |
| `work` | 编译缓存、临时采集、适配结果和预览 | 可由统一 clean 入口清理 |
| `delivery` | 按批次保存的页面包 | 按批次明确删除，生成时拒绝覆盖 |
| `logs` | 本次构建、测试和验收日志 | 可清理，运行工具不依赖旧日志 |

清理入口：

```powershell
pwsh -File ../Source/Workspace.ps1 -Action clean -Target layout -DryRun
```

确认范围后去掉 `-DryRun`。稳定工具和固定输入不在普通清理范围内。
