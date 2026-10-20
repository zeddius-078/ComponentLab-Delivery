# DesignToCode_GenerateComponent

本项目维护组件与 Token 的生产工具，发布供 ComponentLab、GenerateLayout 和业务工程消费的 GeneratedComponentSDK。目录登记、设计绑定、源码存在、构造成功和视觉验收是不同状态，不能由组件数量推断“全部可用”。

## 目录与入口

| 路径 | 内容 |
| --- | --- |
| Source/ComponentKit | 手工维护的组件实现、生成工具、Token 源、合同、模板和资源 |
| Source/PrepareBuild.py | 准备 build/work 副本；默认执行生成，--copy-only 仅复制维护输入 |
| docs | 现行规范、映射工作台、独立消费测试与历史档案 |
| build/work/Source/ComponentKit | Token、原生组件 h/cpp、QSS、catalog 等生成物及编译输入副本 |
| build/sdk | 当前组件 SDK 发布目录 |
| build/resource | 源码交付包等可再生产物 |
| build/cache、build/reports | 编译缓存与当次测试结果 |

维护修改回到 Source 或 docs，不直接编辑 build/work。Source 不允许生成回写；build 可重建，不保存唯一历史记录。构建脚本本身留在 Source，不随 build 删除。

Token 权威源为 `Source/ComponentKit/design-assets/DesignSystem/source`。七层、Theme/Profile 分离及稀疏 Recipe 规则见 [Token 规范](docs/designRules/design-system/Component层Token构建规范.md)。公共目录、原生能力与 Figma 库存分别位于 component-definitions，不把旧 Figma 变体数量当作规范模型。

## 发布与接入

先读取工作区 AGENTS.md；使用已有安装环境，不自主安装或升级依赖。需要组件或 Token 发布时在本项目根执行：

```powershell
pwsh -File Source/Publish-ComponentSDK.ps1
pwsh -File docs/validation/SDKConsumerTest/Build.ps1
python -B Source/CheckProject.py
```

两个消费项目校验后直接引用本项目 build/sdk，不保存 SDK 副本。业务侧公共入口是 `MyDesignSystem/MyDesignSystem.h`，链接目标为 `GeneratedComponentSDK::Components`。映射工作台位于 docs/examples/MappingWorkbench；页面解析与独立库命名合同归 GenerateLayout。

## 当前限制

- 不能据文件生成或静态库编译通过宣称所有组件可实例化；组件仍须通过独立消费与运行测试。
- Python 构建脚本以 3.9 为兼容目标。两个组件生成器采用 UTF-8 write_bytes 保持 LF；对应测试为 docs/validation/test_python39_writes.py。接口回归不等于真实 Python 3.9 实机通过。
- analysis-system 是布局项目的 planned 类名合同，不属于本项目已发布组件。
- PrepareBuild 显式读取两份 docs/archive 审计作只读验证；它们不是 Token 值来源。不得随意挪动或删除。
- 旧 build 已隔离为待删除目录；本次文档工作不删除、不重新发布 SDK，也不恢复挂起修复。

使用步骤见 [SDK 上手指南](docs/tutorial/组件SDK上手指南.md)，完整入口见 [文档中心](docs/README.md)，历史记录见 [归档索引](docs/archive/README.md)。
