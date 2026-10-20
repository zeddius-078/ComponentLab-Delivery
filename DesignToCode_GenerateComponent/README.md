# DesignToCode_GenerateComponent

这个项目负责把设计侧组件身份、属性、状态和 Token 编译为 Qt 组件，并发布供其他项目消费的 `GeneratedComponentSDK`。

## 目录

| 目录 | 作用 |
| --- | --- |
| `Source/ComponentKit` | Token 源、组件合同、Recipe、绑定、组件源码和生成工具 |
| `Source/Qt` | Qt 能力目录、映射工作台和原生组件登记 |
| `Source/PrepareBuild.py` | 将维护输入复制到 output 工作区 |
| `Source/Publish-ComponentSDK.ps1` | 生成、编译、校验和发布 SDK |
| `docs/` | 规则、教程、映射参考和验证代码 |
| `build/sdk` | 当前唯一权威 SDK |
| `build/tools` | 受控工具副本 |
| `output/` | 生成、编译、测试和发布过程文件 |

## Token 和组件链

维护源位于 `Source/ComponentKit/design-assets/DesignSystem/source`。Registry 登记组件身份和合法组合，Contract 登记结构与行为，Recipe 登记视觉决策，Binding 登记设计变量，Runtime Mapping 登记 Qt 类和 API。

生成器读取这些输入，产生 Token、QSS、头文件、实现文件和映射资源，再编译为 SDK。生成结果由 manifest 记录来源指纹、工具链、配置和文件哈希。

## 发布 SDK

工作区根目录执行：

```powershell
pwsh -File Source/Workspace.ps1 -Action build -Target component
```

直接执行时：

```powershell
pwsh -File Source/Publish-ComponentSDK.ps1 -QtRoot <Qt5目录>
py -3.9 -B Source/SDKPackage.py verify --sdk build/sdk --configuration Release --project-root .
```

发布器先在 `output/work` 完成暂存、Token 检查、编译和独立消费者测试，再更新 `build/sdk`。消费者通过 `MyDesignSystem/MyDesignSystem.h` 和 `GeneratedComponentSDK::Components` 接入。

## 当前能力口径

目录登记、设计绑定、源码存在、构造成功、独立消费和视觉验收分别记录。组件数量或 JSON 条目数量不代表组件已经完成全部验收。
