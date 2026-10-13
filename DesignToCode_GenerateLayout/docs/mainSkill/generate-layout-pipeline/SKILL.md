---
name: generate-layout-pipeline
description: 在 G:/ComponentLab-Delivery/DesignToCode_GenerateLayout 中维护 Pixso HTML+DSL 采集、节点合并、工程命名、Qt .ui/.h/.cpp/.qss 生成、GeneratedComponentSDK 装配和页面验收。用户要求从设计页生成复杂布局、补页面映射或验证正式页面包时使用。
---

# Generate Layout Pipeline

## 唯一职责

本项目读取页面设计数据，生成 Qt 页面结构并装配已发布组件。它不定义组件、不编译 Token，也不复制 `Z_*` 源码。

## 开始前必须读取

1. `G:/ComponentLab-Delivery/DesignToCode_GenerateLayout/README.md`
2. `docs/tutorial/布局生成上手指南.md`
3. `docs/mainSkill/generate-layout-pipeline/references/设计到布局生成说明.md`
4. 涉及 `.ui` 时读取 `docs/designRules/layout/Qt-UI布局规则.md`
5. 涉及分层时读取 `docs/designRules/architecture/Qt功能分层.md`
6. 运行 `Source/Verify-GeneratedComponentSDK.ps1`，检查工作区唯一权威 SDK 与设计采集 manifest。
7. 运行 `git status --short`，保留已有修改。

## 输入与输出

| 类型 | 必要内容 | 责任 |
| --- | --- | --- |
| HTML | 同版页面 DOM、文字与浏览器测量结果 | 提供位置、尺寸和文本测量 |
| DSL | 原生节点 ID、父子关系、组件身份、实例属性、Token 绑定 | 提供设计语义和结构身份 |
| Capture manifest | 来源 URL、文件/页面 ID、采集时间、输入哈希 | 证明两类输入可追溯且同版 |
| Mapping | 组件映射、工程对象名、布局字段和异常规则 | 把设计语义编译为 Qt 结构 |
| SDK | GenerateComponent 发布包与 manifest | 提供可实例化 `Z_*` 组件 |
| 输出 | `.ui/.h/.cpp/.qss`，必要时 QRC | 供业务工程通过 uic 与 CMake 接入 |

## 关键目录

| 路径 | 职责 |
| --- | --- |
| `Source/HtmlDsl/inputs` | D2C HTML 和页面资源输入 |
| `Source/HtmlDsl/pipeline/design-inputs` | Pixso MCP 原始 DSL 与核对文件 |
| `Source/HtmlDsl/pipeline/capture` | DSL 采集与输入哈希验证 |
| `Source/HtmlDsl/pipeline/mapping` | 组件、布局、工程命名和扩展规则 |
| `Source/HtmlDsl/pipeline/engines` | 节点、组件和布局转换逻辑 |
| `Source/HtmlDsl/pipeline/compiler` | `.ui/.h/.cpp/.qss` 编译器 |
| `Source/Qt` | ReferencePage、V1/V2 页面运行与回归案例 |
| `Source/Validation` | 来源、结构、SDK 和隔离消费检查 |
| `build/delivery` | 正式页面包 |
| `build/demo/page_examples` | 页面效果演示 |

本项目的脚本、编译器和校验程序统称为“布局生成工具链 / Layout Generator Toolchain”。它们通过命令行和文件合同工作，不构成供业务工程链接的 SDK。唯一组件 SDK 位于 `../DesignToCode_GenerateComponent/build/sdk`。

## 数据合并规则

1. HTML 和 DSL 必须来自同一个设计版本。
2. 使用设计节点 ID 合并，不按显示文字、图层顺序或截图位置猜测身份。
3. HTML 提供实测几何；DSL 提供组件、状态、Token 和层级语义。
4. 缺失、重复或冲突节点必须失败并输出明确错误。
5. 设计原始值、标准化值和 Qt 输出值分别记录，便于回溯转换。

## Qt 文件分工

| 文件 | 承担内容 |
| --- | --- |
| `.ui` | QWidget 层级、Qt Layout、objectName、静态属性和提升组件声明 |
| `.h` | 页面类型和业务需要的类型明确访问器 |
| `.cpp` | 页面初始化、信号连接、运行时状态和非静态装配 |
| `.qss` | 页面级静态视觉；组件公共样式来自 SDK |
| `.qrc` | 页面专属图片、图标和字体资源 |

SDK 组件内部对象不展开到页面 XML。业务逻辑、数据查询和权限判断不写入 `.ui` 或生成器映射。

## 页面生成流程

1. 固定 Pixso 文件、页面、节点和设计版本。
2. 采集 HTML、DSL 与资源，生成 capture manifest 和 SHA-256。
3. 验证 HTML 与 DSL 的版本及节点 ID 一致性。
4. 载入 SDK manifest 和公开组件注册表。
5. 按 Node ID 合并为平台中立设计树。
6. 解析 Auto Layout、尺寸模式、间距、对齐和容器边界。
7. 用组件映射创建已发布 `Z_*` 类；缺少映射时停止。
8. 用 `engineering-names.json` 生成稳定 lowerCamelCase objectName。
9. 输出 `.ui/.h/.cpp/.qss`，必要时输出 QRC。
10. 运行结构、uic、构建、交互和隔离消费验证。

## 工程命名

- 已登记节点使用 `engineering-names.json` 中的稳定名称。
- 未登记节点只接受可确定转换的 ASCII 名称。
- 中文未映射、C++ 保留字、空名和重名直接失败。
- 不把节点 ID 拼进业务对象名来掩盖冲突。
- 设计追溯信息保存在包外报告，不污染正式 `.ui`。

## 组件缺失处理

输出设计节点 ID、设计组件身份、实例属性和当前 SDK 版本。回到 GenerateComponent 建立合同、实现组件并发布新 SDK；本项目重新导入后再生成。禁止在页面编译器中创建临时 QWidget 替代正式组件。

## 验证

```powershell
python -B Source/CheckProject.py
python -B Source/Validation/check_delivery.py --formal-batch '<批次>'
pwsh -File Source/Validation/formal/Verify-RelocatedConsumer.ps1 -Batch '<批次>' -SdkRoot '<SDK目录>'
```

需要检查 V2 回归时执行：

```powershell
pwsh -File Source/Qt/V2/Build.ps1 -Test
pwsh -File Source/Qt/V2/Verify.ps1
```

完成条件：输入可追溯且同版；设计树无未解释节点冲突；正式四件套可由 uic 和业务式 CMake 工程编译；组件全部来自已校验的权威 SDK；隔离验证不依赖兄弟项目源码；验证结果写入 `build/reports`。
