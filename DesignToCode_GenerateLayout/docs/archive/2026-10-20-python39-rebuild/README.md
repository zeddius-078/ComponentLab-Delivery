# Python 3.9 构建刷新记录

日期：2026-10-20。使用已有 Python 3.9.13、Qt 5.15.2、MSVC、CMake、Ninja、Node 和 Playwright；没有安装更新环境。Node 显式使用 `C:\Program Files\nodejs\node.exe`，未修改全局 PATH。

Layout 不维护 Component 式的 build/work 源码副本：由 Source 直接构建布局解析器至 build/cache，再生成页面至 build/preview 或 build/delivery。两者都不允许向 Source 回写产物。

## 实测结果

| 检查 | 实际结果 |
| --- | --- |
| 权威组件 SDK | 本轮 Component 重建包清单哈希校验通过 |
| Source/Build.ps1 -Test | 22 个构建步骤完成；v2_ui_compiler 及测试程序已重建，后续 CTest 非全绿 |
| CTest layout_converter | 通过，16 个 Qt Test 项均通过（含初始化/清理） |
| CTest component_converter | 失败，8 项通过、2 项失败（含初始化/清理） |
| Python 顶层回归 | 11/11 通过：夹具完整性、项目独立性、输出边界 |
| Python 布局及组件库回归 | 19 项中 8 项通过、11 项失败；相关失败均出现 VARIABLE_MODE_VALUE_MISSING: 12:466/ |
| 两项目解释器选择检查 | 自动选择 3.9、指定路径、拒绝缺失/错误版本均通过 |
| HTML+DSL 预览生成 | 两份清单通过；HTML 适配分别通过 14/6 节点且几何差为 0；随后 review 页面编译失败，未形成完整页面产物 |
| Source/CheckProject.py | PASS；此检查不等于运行或视觉验收 |

## 失败与边界

1. SDK 导航栏缺失 `component.navigation-bar.base.state.default.slot.container.height`，同时阻断组件消费和组件转换中的语义组件测试。按原决定继续挂起，未修改 Token。
2. `resourceMappingFromRealDefinitions` 找不到 Leading 母版：测试筛选写死 `standard-light`，保存的原始设计使用旧 Profile 命名。本轮不修改采集证据来迎合测试。
3. 页面变量报 `VARIABLE_MODE_VALUE_MISSING: 12:466/`。已读源码显示 UiCompiler 将 runtimeProfile 传给 DesignTree/validateVariables；原始变量 12:466 所在集合 12:424 的模式名是 light-comfortable/light-compact/dark-comfortable/dark-compact，验证器按传入 Profile 名查找。设计模式与 SDK 模式的转换边界需要单独修复；本轮只记录，不绕过变量校验。
4. HTML 原始脚本有重复声明错误，适配报告保留了这些既有错误；适配器本身仍返回 PASS，不能将其等同于完整页面生成成功。
5. 维护输入基线含 446 个 Source/docs 文件，比较时这些文件保持原字节，但 Source 新增非本任务创建的 `Build._bom.ps1`。未覆盖、删除或执行该文件；已请求暂停并行修改。因此不宣称整个执行窗口的源码目录完全静止。

## 产物与证据

当前解析器：`build/cache/V2-html-dsl/v2_ui_compiler.exe`。运行日志：`build/reports/python39-refresh`；Qt XML 记录：`build/reports/component/qt-results.xml` 与 `build/reports/layout/qt-results.xml`；适配记录：`build/reports/v2/html-dsl-adaptation`。`evidence.zip` 冻结这些记录和输入基线。

正式页面包未生成；没有把失败后的空目录或旧页面称为本轮成功产物。未导出标记为验收通过的交付包，也未作视觉签核。本次完成的是解释器切换后的真实重建与问题暴露，不是所有历史问题清零。
