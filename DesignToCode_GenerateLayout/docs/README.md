# 布局生成文档

GenerateLayout 负责页面结构，不负责创建组件。组件类、公开属性和 Token 由 GenerateComponent 发布，页面生成链通过 SDK 注册表创建实例。

本轮唯一执行与交付标准见 [Gen_layout 最小改造执行计划](Gen_layout最小改造执行计划.md)。以下区分正式四件套和旧 QUiLoader 预览；不把二者写成两套目标架构。

`Source/HtmlDsl/Generate.ps1` 默认生成正式交付，要求明确采集清单、Compact 映射、字体、批次和页面名。历史模式必须传 `-Preview`；两种模式不能混用参数。

## 数据流

| 阶段 | 输入 | 处理 | 输出与完成条件 |
| --- | --- | --- | --- |
| 设计采集 | 同版本 Pixso HTML 与 DSL | 保存原始字节、节点 ID 和 SHA-256 | 两类输入均可追溯 |
| 双源合并 | HTML 计算布局；DSL 组件身份与实例属性 | 按 Node ID 合并，拒绝缺失或重复身份 | 平台中立设计树 |
| Qt 编译 | 设计树、映射规则、SDK 注册表 | 生成布局、类型明确访问器及公开参数调用 | 正式 `.ui/.h/.cpp/.qss`；追溯在包外 |
| 运行装配 | 四件套、组件 SDK、业务侧代码 | `uic` 编译结构，公开接口连接行为 | 不依赖生成器、设计树或追溯日志 |
| 验证 | 来源指纹、生成物、离线包 | 检查结构、行为、哈希和依赖 | 单路线报告通过 |

## 目录

| 位置 | 内容 |
| --- | --- |
| `Source/HtmlDsl/inputs` | D2C 导出的 HTML 与页面资源 |
| `Source/HtmlDsl/pipeline/design-inputs` | Pixso MCP 返回的原始 DSL 与核对文件 |
| `Source/HtmlDsl/pipeline/mapping` | 设计组件、布局字段、工程命名和扩展规则 |
| `Source/HtmlDsl/pipeline/engines` | 组件与布局转换器 |
| `Source/HtmlDsl/pipeline/compiler` | `.ui`、QSS、清单和设计树编译器 |
| `Source/HtmlDsl/pipeline/runtime` | SDK 工厂的页面侧接入层 |
| `Source/HtmlDsl/generated` | 可复现的静态生成结果 |
| `Source/Qt` | 页面运行器和案例 |
| `Source/Validation` | 自动检查与测试源码 |

详细规则见 [设计到布局生成说明](DesignMapping/Guide.md)。

## 工程命名与布局节点

`pipeline/mapping/engineering-names.json` 保存设计 ID 与工程对象名，例如 `19:47 → workOrderInput`。新增页面登记稳定的 lowerCamelCase 名称；未登记时只接受可安全转换的 ASCII 设计名。中文未映射、保留字和重名必须失败，不追加节点 ID 凑名。正式 XML 不保存设计追溯字段，映射记录在包外。旧预览的追溯字段仅供回归，不进入正式包。

Frame 有视觉、独立对齐或尺寸边界职责时，保留原设计的 `QWidget + QLayout`；能够等价合并的纯排列节点才转为嵌套布局。正式模式通过 Qt 对齐实现 start/center/end，不造 Spacer 或包装层。业务使用生成头文件的类型明确访问器；`UiPage::widget(id)` 属于旧预览，不是正式业务接口。

布局尺寸按映射表转换为 Qt 的 Fixed、Hug 或 Fill。未知尺寸枚举以及尚未验证的“主轴 Fill + 非起点对齐”组合会终止生成并给出错误代码。修改布局规则后运行 `Source/Qt/V2/Verify.ps1`，同时核对静态 XML、节点几何和离线包。

## 构建

```powershell
pwsh -File Source/Import-GeneratedComponentSDK.ps1 -Package '<GeneratedComponentSDK目录>'
pwsh -File Source/Qt/V2/Build.ps1 -Test
pwsh -File Source/Qt/V2/Verify.ps1
```

`GenerateLayout` 不包含组件生成开关和 Token 编译目标。未映射组件会中止生成并输出具体节点与缺失映射。
