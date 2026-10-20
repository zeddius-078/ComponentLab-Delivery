# 设计模式与运行时 Profile 分离修复

日期：2026-10-20。范围：修复 Layout 变量模式选择与母版筛选测试；保持 Python 3.9 和原始采集不变，不修改、生成或发布组件 SDK。

## 原因与修补

真实 CLI 在 review 输入稳定报 `VARIABLE_MODE_VALUE_MISSING: 12:466/`。该变量有有效模式值，但编译器将 SDK `standard-light` 传给 DesignTree 的变量校验；原始集合名为 `light-comfortable` 等设计名称。显式 mode ID 可成功，新名称的受控内存样例也成功，排除了原始值缺失这一解释。

- UiCompiler 向 DesignTree 传 `profileResolution.designProfile`；主题和组件仍使用 `runtimeProfile`。
- DesignTree 参数显式命名为 designProfile，避免调用方再混淆职责。
- 图标母版测试按现有 mapping.profileAliases 判断对应 SDK 上下文，不再硬编码原始母版必须叫 standard-light；仍严格限制内容模式、状态、外观与图标样式，并验证真实图标、可访问名称和点击事件。
- 新增真实 CLI 的 7 项回归。修改前 2 项失败、5 项通过；修改后全部通过。无效模式、缺值、歧义及未登记别名仍失败关闭。

## 验证结果

| 范围 | 结果 |
| --- | --- |
| Python 版本 | 已安装的 3.9.13；未安装或更新环境 |
| Layout 解析器编译 | 通过，build/cache/V2-html-dsl/v2_ui_compiler.exe 已更新 |
| CTest 组件、布局转换 | 2/2 测试组通过 |
| 布局、组件库、Profile CLI 回归 | 26/26 通过，含新增 7 项 |
| 固定输入完整性、独立性、输出边界 | 11/11 通过 |
| HTML+DSL 两页预览 | review 和 result-html 均生成成功，uic 校验通过 |
| V2 示例构建与运行回归 | 17/17 通过（含初始化/清理）；包含确认返回流程、运行状态更新、库切换、Profile 别名、确定性及重复构造销毁 |
| 项目检查、git diff --check | 通过 |

预览页面位于 `build/preview/html-dsl`；V2 示例及部署数据位于 `build/demo/page_examples/V2`。当次日志在 `build/reports/profile-mode-fix`；Qt 运行结果在 `build/reports/v2/html-dsl/qt-results.xml`。`evidence.zip` 冻结当次日志和测试记录。

## 事实边界

原始采集、清单哈希、Token 源、SDK 未被本轮改写。原 HTML 重复脚本声明警告仍在适配报告中，没有通过改原文掩盖。其他任务留下的组件实现改动和 Build._bom.ps1 未被覆盖、删除或作为修补来源。

本轮所用 SDK 已不同于上一轮失败时的外部状态；当前 Layout CTest 的语义组件用例也通过，但这不构成本轮修复了导航 Token 的证据。本轮未重新执行组件 SDK 独立消费发布流程，不追认旧 SDK 或全部组件已验收。

此次成功范围是固定案例的预览生成与 V2 自动运行回归，不是正式三页交付、离线部署或人工视觉签核。回退时仅撤销本次 UiCompiler/DesignTree 与相应测试和规范变更，再重建 Layout；不恢复损坏的原始采集，也不混入组件源码改动。
