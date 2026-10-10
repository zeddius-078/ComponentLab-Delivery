# Qt 设计到开发映射演示

三个项目按“组件产出、组件验收、页面装配”分工。组件和 Token 只有一个事实源。

当前第一阶段基线采用 [Figma → sToken → 状态账本基线计划](DesignToCode_GenerateComponent/docs/DesignSystem/Figma_sToken_状态账本基线计划_2026-10-11.md)：先治理 Figma 中设计师实际使用的资源结构，再把已确认的设计决策同步到 `stoken.json`，完成生成校验后更新状态账本。Git 用于版本发布和追溯，不替代设计、Token 或运行证据。

当前一阶段基线以[组件基线与原生组合映射纲领](DesignToCode_GenerateComponent/docs/DesignMapping/一阶段组件基线与原生组合映射纲领.md)为准：建设 Qt 原生组件能力与 Ant Design 设计组件交集范围内的完整映射，组合严格遵循原生能力，为 GenerateLayout 提供有效且一致的组件引用与配置。

| 项目 | 输入 | 允许执行 | 输出 |
| --- | --- | --- | --- |
| [DesignToCode_GenerateComponent](DesignToCode_GenerateComponent/README.md) | 设计系统的组件、属性、状态、Token | 建立设计组件到 Qt 类/API 的映射；编译 Token；构造并发布组件 | `GeneratedComponentSDK`、映射清单、组件工作台 |
| [ComponentLab](ComponentLab/README.md) | 已发布的 `GeneratedComponentSDK`、可替换 Token 变量集 | 展示 Button、Checkbox、Input、Select 的外观、状态和组合 | 组件演示程序、展示验证结果 |
| [DesignToCode_GenerateLayout](DesignToCode_GenerateLayout/README.md) | 同源 Pixso HTML + DSL、组件实例、已发布 SDK | 合并 HTML 布局与 DSL 组件身份；按映射引用组件；生成 `.ui` 并连接页面交互 | Qt 页面、布局清单、页面演示程序 |

```text
设计系统组件 / 属性 / 状态 / Token
                  |
                  v
DesignToCode_GenerateComponent
       | 发布 GeneratedComponentSDK
       +-----------------------+
       v                       v
ComponentLab          DesignToCode_GenerateLayout
外观与状态验收         设计稿解析、组件引用、布局装配
```

依赖通过显式发布和导入完成。两个消费项目的 `Source` 中均不保存组件实现或 Token 编译器。每个项目各有 `Source`、`docs`、`build`，能够单独构建和验证。

三个项目统一采用 Productized Library Structure（产品化库结构）：

| 路径 | 内容 | 开发侧用途 |
| --- | --- | --- |
| `build/sdk` | 头文件、静态库、Token 与映射资源、CMake 包 | 编译和集成 |
| `build/demo` | EXE、Qt DLL、启动脚本、组件画廊或页面案例 | 查看效果与验收 |
| `build/cache` | CMake、编译器和生成器缓存 | 可重建 |
| `build/reports` | 测试与发布结果 | 追溯验证 |

GenerateComponent 生成 `build/sdk`。ComponentLab 与 GenerateLayout 仅导入同一发布包；它们不会从本地源码构造组件。
