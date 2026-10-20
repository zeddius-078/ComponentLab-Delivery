# 转换流水线实现

| 目录或文件 | 作用 |
| --- | --- |
| `capture` | 采集、D2C 返回落地、capture manifest 创建与验证 |
| `compiler` | 设计树、组件初始化、场景装配、Qt 文件和资源编译 |
| `engines/component_converter` | 组件身份和属性转换 |
| `engines/layout_converter` | Box、Grid、Form 等布局转换 |
| `runtime` | JSON、Widget 注册和 C++ 运行支持 |
| `Generate-Pages.ps1` | 页面生成步骤编排 |

`pipeline/runtime` 是源码；项目 `build/runtime` 是固定 Python、Node、Playwright 和浏览器依赖。采集输入来自 `docs/fixtures` 或一次性 `output/work`，生成页面进入 `output/delivery`。

清单校验、HTML 适配、组件映射、布局编译、uic、消费者编译和视觉检查分别记录结果。任一阶段失败时保留具体错误，不通过修改哈希或丢弃节点绕过。
