---
name: generate-component-pipeline
description: 在 G:/ComponentLab-Delivery/DesignToCode_GenerateComponent 中维护设计证据、设计组件到 Qt API 映射、sToken 编译、QSS、Z_* C++ 组件、ComponentFactory 和 GeneratedComponentSDK。用户要求新增组件、补属性状态、刷新 Token、发布 SDK 或检查设计到组件链路时使用。
---

# Generate Component Pipeline

## 唯一职责

本项目是组件与 Token 的唯一产出方。它把经核实的设计合同和 Qt 5.15.2 能力转换为可被业务工程直接引用的 C++ 组件 SDK。

## 开始前必须读取

1. `G:/ComponentLab-Delivery/DesignToCode_GenerateComponent/README.md`
2. `docs/tutorial/组件SDK上手指南.md`
3. `docs/mainSkill/generate-component-pipeline/references/设计组件映射指南.md`
4. 涉及 Qt 属性时读取 `references/Qt组件类与主属性参考.md`
5. 涉及 Pixso 证据时读取 `references/design-evidence/DesignEvidence结构与职责.md` 和 `CanonicalDesignContract字段参考.md`
6. 运行 `git status --short`，识别并保留已有修改。

## 权威输入和输出

| 阶段 | 权威输入 | 处理 | 输出 |
| --- | --- | --- | --- |
| 设计证据 | Pixso/Figma 原生 ID、组件集、变体、变量绑定 | 采集、哈希、标准化、校验 | Canonical 设计合同 |
| Qt 能力 | Qt 5.15.2 元对象与项目能力目录 | 分类属性、状态、结构和行为 | Qt 能力目录与映射 |
| Token | `design-assets/tokens`、`stoken.json`、组件合同 | 类型校验、引用解析、Profile 编译 | Catalog、QSS、运行时 Token |
| 组件 | 组件定义、映射、Token 产物 | 生成或维护 `.h/.cpp`、绘制和状态 | `Z_*` C++ 组件与工厂 |
| 发布 | 公共头、库、资源、映射 | 构建、独立消费、哈希清单 | `build/sdk` |

设计截图只能支持视觉核对，不能代替节点 ID、变量绑定或运行测试。推断字段需要明确标记，不能写成原始设计事实。

## 关键目录

| 路径 | 职责 |
| --- | --- |
| `Source/DesignEvidence` | Pixso 原始证据、Canonical 合同、采集与校验工具 |
| `Source/Qt/MappingWorkbench/mapping` | 设计身份、属性、状态与 Qt 类/API 映射 |
| `Source/ComponentKit/design-assets` | Token 输入、组件合同、QSS 模板和编译工具 |
| `Source/ComponentKit/component-definitions` | Native、Design System、Composite 组件定义 |
| `Source/ComponentKit/runtime/component` | 手写或生成的 `Z_*` `.h/.cpp` 与工厂 |
| `Source/ComponentKit/MyDesignSystem.h` | SDK 公共总头文件 |
| `Source/SDKConsumerTest` | 只使用发布 SDK 的独立消费测试 |
| `build/sdk` | 唯一正式 SDK 发布目录 |
| `build/demo/component_mapping_workbench` | 映射关系演示，不进入 SDK |

## 组件完成合同

每个可视组件必须同时具备：

1. 唯一设计身份，例如 `ComponentLab/Button`。
2. 唯一开发类，例如 `commonWidget::Z_PushButton`。
3. 明确 Qt 基类及继承能力。
4. 属性、状态和默认值映射。
5. Token 绑定和 Profile 结果。
6. `.h`、`.cpp` 和对应 QSS；纯原生薄包装也需要独立源码。
7. `ComponentFactory` 或 Registry 构造入口。
8. `MyDesignSystem.h` 与 SDK 安装导出。
9. 最小构造、属性、状态、主题和独立消费测试。

只登记 JSON、只生成 `.def`、只添加 QSS 或只在 Demo 中伪造控件，均不满足组件完成合同。

## 属性落点

| 属性类型 | 示例 | 工程落点 |
| --- | --- | --- |
| 设计变体 | `type`、`size`、`variant` | 设计组件属性 + C++ enum / setter |
| 实例内容 | `text`、`placeholderText`、`checked` | 设计实例属性 + `Q_PROPERTY` / setter |
| 视觉 Token | 颜色、字号、圆角、高度、阴影 | Token → Catalog/QSS/C++ 绘制 |
| 结构属性 | Layout、margin、stretch | GenerateLayout 的 DSL → `.ui` |
| 交互状态 | hover、pressed、focus、disabled | Qt 事件与状态机 + 状态 Token |
| 业务数据 | options、model、selection | Controller / Model；组件只暴露接口 |
| 只读结果 | `displayText`、`selectedText` | 运行时查询，不作为设计输入 |

## 修改流程

### 新增组件

1. 在设计证据或设计系统中确认身份、属性和值域。
2. 在 Qt 能力目录确认基类、可写属性、枚举和信号槽。
3. 更新映射注册表和组件定义。
4. 增加组件 Token 合同和 QSS 模板；无专用视觉规则时显式继承语义 Token。
5. 实现 `Z_* .h/.cpp`，公开稳定 API。
6. 注册工厂、Designer 适配和公共总头。
7. 生成 Token 与组件资源，检查生成漂移。
8. 发布 SDK，运行独立消费测试。
9. 由 ComponentLab 校验并直接引用 SDK 后展示，不在本项目复制演示逻辑。

### 修改 Token

1. 确认修改属于四层中的哪一层：`primitive`、`profile`、`semantic`、`component`（产品专属布局改 `DesignSystem/source/product/application-layout.json`，不是公共层）。
2. 修改唯一规范源 `DesignSystem/source/*.json` 和必要的组件合同（`design-assets/tokens/*-contract.json`）。Figma 调整先作为候选差异进入 `DesignSystem/config/legacy-map.json` 或冲突清单，不以设计文件/运行时文件双向覆盖。
3. 运行 `python -B design-assets/tools/export_tokens.py`，检查四个上下文（standard/wide × light/dark）的 Catalog、CSS、DTCG 与 Qt 运行时产物。
4. 运行 `python -B -m unittest discover -s DesignSystem/scripts/v2/tests`。
5. 搜索硬编码值，确认运行时使用生成结果。
6. 重新发布 SDK；消费项目通过导入脚本升级。旧 stoken.json 只是冻结设计快照与单向兼容产物，不再是写入入口。

### 修改设计映射

保留设计原生 ID、来源、采集时间和输入哈希。映射只描述设计身份到 Qt API 的可复现关系；无法验证的属性进入问题清单，不由模型补造默认值。

## 不得跨越的边界

- ComponentLab 负责展示，不参与组件生成。
- GenerateLayout 负责布局与页面装配，不维护第二套组件 setter 或实现。
- 主线运行时只使用 C++ Qt5；Python、PowerShell 和 JavaScript 是离线工具。
- Ant Design 可作为设计语义参考，不替代 Qt 元数据、项目设计证据或 Token 合同。
- 生成物由生成器更新；手工修改生成目录前先确认文件生命周期。

## 验证

```powershell
pwsh -File Source/Publish-ComponentSDK.ps1
pwsh -File Source/SDKConsumerTest/Build.ps1
pwsh -File Source/Qt/MappingWorkbench/Build.ps1 -GenerateTokens -Test -Deploy
python -B Source/CheckProject.py
```

完成条件：SDK manifest 与文件哈希完整；业务式消费者只包含 `MyDesignSystem/MyDesignSystem.h` 并链接 `GeneratedComponentSDK::Components` 即可创建组件；源码、QSS、Token、工厂和映射身份可互相追溯；测试结果记录在 `build/reports`。
