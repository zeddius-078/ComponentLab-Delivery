# Generate Layout Pipeline Skill

## 1. 任务边界

本 Skill 用于把同一设计版本的 HTML 与 Pixso DSL 转换为 Qt 页面文件。组件实现、Token、公共 QSS 和 SDK 由 `DesignToCode_GenerateComponent` 提供；页面生成器读取 SDK，不复制组件实现。

正式页面输出：`.ui`、`.h`、`.cpp`、页面 `.qss` 和 `.qrc`。输出可供业务 Qt 工程继续编译和装配。

## 2. 执行前读取

按以下顺序读取：

1. 工作区 `AGENTS.md`：固定环境和协作约束。
2. 项目 `README.md`：目录与入口。
3. `docs/designRules/layout/布局转换器通用化规范.md`：布局字段和值域。
4. `docs/designRules/library-profiles.md`：组件库选择。
5. `docs/tutorial/布局生成上手指南.md`：当前命令。
6. 目标页面的 capture manifest、HTML、DSL、layoutReference、字体和资源清单。

输入缺失时先记录缺失项，不能用固定样例代替目标页面。

## 3. 选择生成深度

| 深度 | 输入 | 输出 | 适用场景 |
| --- | --- | --- | --- |
| 结构预览 | 结构 DSL、初始样式、字体 | 结构 `.ui`、PNG、报告 | 先核对层级和位置 |
| 正式页面 | 同版 HTML + DSL、布局证据、映射、字体、SDK | 页面五件套 | 进入业务 Qt 工程 |

结构预览中的未知节点使用 QWidget 表达，适用于结构核对。正式页面要求组件身份和 SDK 合同完整。

## 4. 依赖与环境

使用已经安装的 Python 3.9、Qt 5.15.2、CMake、MSVC、Ninja、Node.js、Playwright 和已发布 SDK。路径通过当前进程参数或环境变量传入：`D2C_PYTHON_EXE`、`QT_ROOT`、`VS_ROOT`、`CMAKE_EXE`、`NINJA_ROOT`、`NODE_EXE`、`D2C_NODE_MODULES`、`GENERATED_COMPONENT_SDK_ROOT`。

依赖检查使用：

```powershell
pwsh -File ../Source/Workspace.ps1 -Action doctor
```

环境检查失败时停止该阶段并记录原因。

## 5. 输入清单

capture manifest 绑定 HTML、DSL、设计文件标识、根节点、字体、显式资源、布局证据和 SHA-256。正式模式必须包含独立 `layoutReference`；该文件来自设计节点布局属性读取。

创建与验证：

```powershell
py -3.9 -B Source/HtmlDsl/pipeline/capture/capture_manifest.py create `
  --output '<capture目录>/capture.json' `
  --html '<capture目录>/page.html' `
  --dsl '<capture目录>/canonical.raw.json' `
  --root-id '<根节点ID>' --file-key '<Pixso文件ID>' `
  --font-family '<字体族>' `
  --layout-reference '<布局证据.json>'
py -3.9 -B Source/HtmlDsl/pipeline/capture/capture_manifest.py verify `
  --manifest '<capture目录>/capture.json' --mode formal
```

路径按清单自身目录解析。原始响应按字节保存，导入器只下载清单中显式声明的资源。

## 6. 组件与布局映射

组件映射使用所选 Profile。稳定设计身份对应 SDK manifest 中的 canonical ID、Qt 类和公开属性。组件初始化生成器只调用合同中的 API。

布局分派如下：

```text
HORIZONTAL -> QHBoxLayout
VERTICAL   -> QVBoxLayout
GRID       -> QGridLayout
FORM       -> QFormLayout
```

Grid 需要行、列和跨度；Form 需要行号和角色。定位信息缺失时报告错误，不能由 left/top 推断。

## 7. 页面生成顺序

```text
capture verify
  -> browser adapt
  -> component mapping
  -> layout conversion
  -> .ui/.h/.cpp/.qss/.qrc
  -> uic
  -> consumer build
  -> runtime / visual review
```

SDK 页面构造顺序为 `setupUi`、页面 QRC 注册、ComponentFactory 应用运行时属性、页面 QSS 加载。公共组件样式由 SDK 管理，页面 QSS 只表达页面专属样式。

正式入口：

```powershell
pwsh -File Source/HtmlDsl/Generate.ps1 `
  -CaptureManifest '<capture.json>' `
  -MappingDirectory '<映射目录>' `
  -Font '<字体文件>' `
  -Batch '<批次>' -Page '<页面名>'
```

正式结果进入 `output/delivery/<批次>/<页面>`，临时数据进入 `output/work`，日志进入 `output/logs`。已有页面目录拒绝覆盖。

## 8. 验证

```powershell
py -3.9 -B docs/validation/check_delivery.py --formal-batch '<批次>'
pwsh -File docs/validation/formal/Verify-RelocatedConsumer.ps1 `
  -Batch '<批次>' -SdkRoot '<SDK目录>'
```

验证至少分为：输入清单、浏览器适配、生成文件、uic、消费者编译、运行状态和视觉比对。每一项单独记录，不能从上一项推导下一项。

## 9. 失败处理

| 错误 | 处理 |
| --- | --- |
| 哈希不匹配 | 重新获取同版输入并更新清单 |
| 缺少布局字段 | 回到设计节点数据补充定位信息 |
| 未登记组件 | 在 GenerateComponent 建立合同并发布 SDK |
| SDK manifest 不匹配 | 重新选择已发布 SDK 或重新构建 |
| uic 失败 | 修正 `.ui` 序列化，不修改报告掩盖错误 |
| 消费者编译失败 | 检查头文件、库、QRC 和 CMake 链接 |

结构预览可用于确定节点和布局，不能替代正式组件映射。`synthetic-example` 只表示手写技术样例，不能作为 Pixso 实测证据。
