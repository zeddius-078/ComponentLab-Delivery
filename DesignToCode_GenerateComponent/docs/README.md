# 组件生成说明

本目录解释从设计系统组件到 Qt 组件 SDK 的映射和发布。

## 当前内容入口

Figma 基础设计资产与 Qt 合同的逐项审计进展，以现有 [Figma 状态账本](DesignSystem/figma-core-rebuild-state.json)及对应 Figma 实际节点为准。先读账本的 `claimStatus.currentVerified` 和 `pendingValidations`；`claimStatus.historicalConfirmedCheckpoints` 与 `entities.globalAudit` 的旧数量只是历史检查点，不是当前覆盖率。账本记录设计身份、证据和开放项，不替代 `Component Registry`、手写 sToken、SDK Manifest 或实际 Qt 源码。`DesignSystem` 目录下 2026-10-07 的[审计基线](DesignSystem/Figma核心设计系统审计基线.md)和[实施记录](DesignSystem/Figma核心设计系统实施记录.md)同样是阶段快照。设计合同审批与 Qt 运行验证必须分别取得证据，不能互相替代。

早期[组件基线与原生组合映射纲领](DesignMapping/一阶段组件基线与原生组合映射纲领.md)保留为历史需求证据，不再定义当前设计风格或完成标准。当前目标是以 DSM 四层 Token 与 Figma 原件建立可追溯的 Qt 5.15.2 基础设计合同，明确 Native、Design System、Composite 与 Engineering Contract 边界；本阶段不据此宣称 gen-comp、gen-layout 或 Qt 运行全链路完成。

| 文档 | 内容 |
| --- | --- |
| [一阶段组件基线与原生组合映射纲领](DesignMapping/一阶段组件基线与原生组合映射纲领.md) | 早期需求与原生组合约束的历史记录；现阶段以状态账本和当前用户要求为准 |
| [组件映射指南](DesignMapping/Guide.md) | 当前 DSM 身份与 Qt API 边界；其中保存的 Pixso 示例仅用于历史回归，不是现行 Figma 资产证据 |
| [组件完成度历史对照](组件完成度与Ant对照.md) | 早期 `Z_*` 身份和验证口径的历史记录；不可代替当前 Registry、Token 与 Figma 回读 |
| [Qt 功能分层](<Qt 功能分层.md>) | `.ui`、QSS、QWidget、Controller 的职责 |
| [可视化总览](DesignMapping/Overview.html) | 映射链路和产物关系 |

## 数据流

| 阶段 | 输入 | 执行动作 | 输出 |
| --- | --- | --- | --- |
| 设计组件识别 | 组件 ID、组件集、属性和状态 | 与登记的设计身份匹配 | 标准化组件契约 |
| Token 编译 | 基础、语义、组件 Token | 解析引用、校验类型、生成 Profile | QSS、catalog、manifest |
| Qt 组件构建 | 组件契约、Token 产物、C++ 实现 | 编译组件、主题和资源 | `GeneratedComponentSDK.lib` |
| SDK 发布 | 二进制、公共头、映射文件 | 计算哈希并写发布清单 | `build/sdk` |
| 开发接入测试 | SDK 路径、平台总头文件 | 只引用 SDK，创建真实组件实例并运行 | `Source/SDKConsumerTest` |

设计组件示例：`ComponentLab/Button` 的设计类型映射到 `commonWidget::Z_PushButton` 的项目接口。Loading 属于设计系统运行时扩展，不能仅凭 Figma 预览或变体名称认定为 Qt 原生可写属性；颜色、尺寸、材质也须分别核对组件 Token、生成资源与实际 Qt 消费。

## 全量 Qt 能力目录

| 类别 | 数量 | 进入设计组件集 | 工程落点 |
| --- | ---: | --- | --- |
| 具体可视控件 | 59 | 各有独立设计身份；不等于 59 个一对一页面／组件集或完整 API 已映射 | `Z_*` 包装类、`ComponentFactory`、SDK |
| 抽象基类 | 5 | 否 | 继承属性与状态合同 |
| Layout | 7 | 否 | GenerateLayout 的结构映射输入 |
| Model | 5 | 否 | Controller / Model 数据合同 |
| 行为类型 | 2 | 否 | Action 与运行时行为合同 |
| 值与平台适配 | 5 | 否 | Token、`.ui` 或平台参数 |
| 应用运行时 | 3 | 否 | 应用入口能力登记 |

能力目录来自 [qt-native-catalog.json](../Source/Qt/MappingWorkbench/mapping/qt-native-catalog.json)。元对象报告由 Qt 5.15.2 实机扫描生成，位置为 `build/reports/Workbench/qt-native-meta-report.json`。当前设计身份、命名和 Token 以 DSM 的 Figma 原件、手写 sToken 及现有 Registry/合同核实；原生 API 以 Qt 5.15.2 为准。目录内的旧参考来源只作迁移溯源，不能补写 DSM 设计值、Qt API 或完成状态。

上述 86 个 Qt 类型是能力目录，不是 86 个 Figma 公共视觉组件的配额；27 个非可视类型作为工程合同管理。设计系统扩展组件和复合组件另按其实际 Qt 基类、SDK 类及组成关系登记，不伪装为 Qt 5.15.2 原生控件。

```powershell
pwsh -File Source/Publish-ComponentSDK.ps1
```

发布清单位于 `build/sdk/manifest/component-sdk.json`。其中 `owner`、权限字段和文件哈希用于消费项目的导入校验。映射工作台位于 `build/demo/component_mapping_workbench`，不进入 SDK。
