# 转换流水线实现

| 目录或文件 | 作用 |
| --- | --- |
| `capture` | 采集、D2C 返回落地、capture manifest 创建与验证 |
| `compiler` | 设计树、组件初始化、场景装配、Qt 文件和资源编译 |
| `engines/component_converter` | 组件身份和属性转换 |
| `engines/layout_converter` | Box、Grid、Form 等布局转换 |
| `runtime` | JSON、Widget 注册和 C++ 运行支持 |
| `Generate-Pages.ps1` | 页面生成步骤编排 |
| `RunPageAudit.py` | 固定顺序、失败继续的页面生成与结果汇总入口 |

`pipeline/runtime` 是源码；项目 `build/runtime` 是固定 Python、Node、Playwright 和浏览器依赖。采集输入来自 `docs/fixtures` 或一次性 `output/work`，生成页面进入 `output/delivery`。

清单校验、HTML 适配、组件映射、布局编译、uic、消费者编译和视觉检查分别记录结果。任一阶段失败时保留具体错误，不通过修改哈希或丢弃节点绕过。

## 固定顺序自动执行

从项目根目录运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Source/HtmlDsl/RunPageAudit.ps1 `
  -Url 'https://pixso.cn/app/design/<fileKey>?item-id=<rootId>'
```

默认执行顺序为：环境与 MCP 检查、DSL、D2C、布局证据、页面落地、资源扫描、最终导入、manifest 校验、字体解析、adapt、structure-only 编译、`uic` 与结果汇总。strict 编译、strict `uic` 和正式验收默认记为 `NOT_RUN`，不会创建 delivery；仅传入 `-GenerationMode strict` 才会追加它们。普通步骤失败只记录并继续；缺少前置输入时下游标记 `SKIP`；工具完整性或路径安全失败时停止受影响分支。结果写入 `output/work/audit/<批次>/result.json`，简要报告写入同目录 `REPORT.md`，原始日志写入 `output/logs/audit/<批次>/run.log`。默认完成即返回 0；需要 CI 失败码时加 `-FailOnError`。

默认结构路径适合没有临场推理能力的执行者：不得修改 DSL、注入变量模式或改用手写发射器。它不求值 Token，因此变量模式缺失记为 `NOT_APPLICABLE`；缺失数值 gap/四向 padding 按 `0` 输出，并在 trace 的 `defaultedMetricFields` 留痕。组件身份、目标类、布局模式和 Grid/Form 定位仍为硬门禁。普通 Frame 由 Qt `QWidget` 承载，不需要 contract-only 组件库登记该框架类。

结构容器分为 Auto Layout 与静态几何两类：已声明 `HORIZONTAL`、`VERTICAL`、`GRID`、`FORM` 的 Frame 使用对应 Qt Layout；`NONE` 或缺失 `stackMode` 的 Frame 仅在所有直接子节点都提供有限 `left/top/width/height` 时使用 `absolute-geometry`，按原始 `childNode` 顺序生成嵌套 QWidget 与 `.ui` geometry，并记录 `trace.staticLayouts`。静态子节点缺几何时失败 `STATIC_GEOMETRY_REQUIRED`；strict 不启用静态几何策略。

```powershell
# 默认：结构布局和默认组件外观
powershell -NoProfile -ExecutionPolicy Bypass -File Source/HtmlDsl/RunPageAudit.ps1 -Url '<Pixso链接>'

# 显式：完整组件合同与正式交付验收
powershell -NoProfile -ExecutionPolicy Bypass -File Source/HtmlDsl/RunPageAudit.ps1 -Url '<Pixso链接>' -GenerationMode strict
```
