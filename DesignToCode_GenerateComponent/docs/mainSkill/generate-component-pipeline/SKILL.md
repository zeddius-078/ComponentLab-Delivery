---
name: generate-component-pipeline
description: 在 DesignToCode_GenerateComponent 维护组件身份、Qt API 映射、七层 Token、QSS、Z_* 实现及 GeneratedComponentSDK。用于组件或 Token 修改、生成边界检查、SDK 发布和独立消费验证。
---

# Generate Component Pipeline

## 开始前

读取工作区 AGENTS.md、项目 README、[上手指南](../../tutorial/组件SDK上手指南.md)及 [设计组件映射指南](references/设计组件映射指南.md)。涉及 Token 先读 [构建规范](../../designRules/design-system/Component层Token构建规范.md)及 source/v2-source-manifest.json；涉及 Qt 属性再读 [Qt 参考](references/Qt组件类与主属性参考.md)。检查现有修改，不覆盖并行任务。

## 维护源与生成物

| 位置 | 职责 |
| --- | --- |
| Source/ComponentKit/design-assets/DesignSystem/source | 七层 Token 及 Registry/Contract/Recipe/Binding/Runtime Mapping |
| Source/ComponentKit/component-definitions | Native 与统一公共目录的维护定义 |
| Source/ComponentKit/runtime | 手工实现、状态、绘制、资源和工厂 |
| Source/ComponentKit/MyDesignSystem.h | 公共总头维护入口 |
| docs/examples/MappingWorkbench | 维护映射输入与演示源码 |
| docs/validation/SDKConsumerTest | 独立消费测试源码 |
| build/work/Source/ComponentKit | PrepareBuild 准备的副本及所有生成源码/Token/QSS |
| build/sdk | 当前组件 SDK 发布物 |
| docs/archive | 历史记录；不可充当运行时实现 |

Source 和 docs 是维护输入，生成执行不得回写。生成原生 h/cpp、聚合头、catalog 和 QSS 只进入 build/work。PrepareBuild 显式复制追溯清单及两份归档审计；这些审计只做验证，不供给 Token 值。不得为清理文档移动这些依赖。

## 组件与 Token 规则

- 公共身份一对一，Qt 基类、私有 anatomy、Variant 不另算公共身份；布局辅助项与资产不进 QWidget Factory。
- 可用组件至少具备合同、实际实现、公开 API、Token/状态、工厂入口、公共头/安装导出及独立消费证据。JSON 登记或 h/cpp 生成不等于运行完成。
- 七层为 Primitive / Profile / Theme / Semantic / Composite / Component / Content。Theme 与 Profile 分开；Recipe 是稀疏视觉源，生成 component.json 与组合展开不是维护源。
- 内容文本与视觉 Token 分离；布局结构归 GenerateLayout；业务数据与生命周期归 Controller/Model。
- Figma ID、版本、输入哈希与映射分开记录；截图和节点数量不能证明合法组件模型，未验证项保持未验证。
- Analysis 等独立目标库只在 GenerateLayout 声明，contract-only/planned 不写进本项目已实现 Registry，不生成假实现。

## 执行

1. 确认任务属于合同、Token、实现还是映射；只修改对应维护源。
2. 新增组件时确认 Qt 基类、属性值域、状态、事件、资源、工厂及导出。
3. `python -B Source/PrepareBuild.py --copy-only` 只准备副本；不加该参数会执行 Token 与组件生成。
4. 需要发布时运行 `pwsh -File Source/Publish-ComponentSDK.ps1`，再运行 `pwsh -File docs/validation/SDKConsumerTest/Build.ps1`。
5. 消费端校验并直接引用 SDK，不复制、不引用生产方私有源码。

环境不可自主安装或升级。Python 构建脚本目标 3.9；两个 Python 组件生成器采用 UTF-8 write_bytes 保持 LF。禁止改回仅较新版本支持的 write_text(newline=...)；兼容接口测试不等于真实 3.9 实测。

## 按范围验证

```powershell
python -B Source/CheckProject.py
python -B -m unittest discover -s docs/validation -p test_python39_writes.py -v
```

Token 变更在 build/work 副本中运行 scripts/v2/tests；映射变更使用 docs/examples/MappingWorkbench/Build.ps1。完整发布需构建及独立消费，不以文件生成通过代替。

文档整理与无关改动不得自行重发 SDK 或宣称全量组件可运行。当次报告写 build/reports；需长期保留的结果冻结到 docs/archive，现行文档不追加流水账。
