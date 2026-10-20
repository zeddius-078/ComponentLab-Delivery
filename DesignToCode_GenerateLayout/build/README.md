# 受控布局工具与运行依赖

本目录保存已经发布的工具及固定运行材料。普通 clean 只处理 `output`，不处理这里的稳定资产。此 README 是目录说明；运行资产通过受控发布提供。

| 内容 | 用途 | 更新方式 |
| --- | --- | --- |
| `tools/layout` | 布局工具、Qt 运行材料、脚本和资源的运行副本 | 从 Source 构建、测试、校验后事务发布 |
| `tools/support` | 工作区共享校验和生命周期支持工具副本 | 由工作区维护实现受控发布 |
| `runtime/python` | 固定 Python 3.9 运行材料 | 使用批准的已有材料，不自动升级 |
| `runtime/node`、`runtime/node_modules` | Node 及 Playwright 等依赖 | 版本与文件清单控制 |
| `runtime/browser` | 匹配的浏览器发行材料 | 保持固定版本及哈希 |
| `runtime/licenses` | 运行材料许可证 | 随对应材料维护，交付时一并检查 |
| `runtime/runtime-manifest.json` | 运行依赖配置与文件校验清单 | 发布过程生成，哈希由发布流程维护 |
| `.state` | 发布锁、恢复状态与受控上一版本 | 由生命周期工具管理 |

组件 SDK 默认来自兄弟项目 `DesignToCode_GenerateComponent/build/sdk`，本项目不维护第二套权威 SDK。外部 SDK 为只读输入。

## 运行与维护

从本项目根检查已发布工具：

```powershell
.\build\tools\layout\Run.cmd doctor --work '<已有的独立工作目录>'
```

`doctor` 不代表页面生成或视觉验收通过。实际生成参数见[上手指南](../docs/tutorial/布局生成上手指南.md)。普通运行不需要重新构建 SDK 或安装 VS/CMake；源码构建仍需固定开发环境。

脚本修改回到 [Source](../Source/README.md)。工具副本由发布流程更新。输出进入工作目录的 `output/work`、`output/delivery` 和 `output/logs`。Git 检出后的工具可用性由 `doctor` 检查，首次开发构建由工作区入口协调。

[返回项目入口](../README.md)
