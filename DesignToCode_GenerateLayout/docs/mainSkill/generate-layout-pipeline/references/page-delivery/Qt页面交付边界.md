# Qt 页面交付现状与边界

## 页面类型与正式链

ReferencePage、V1 和旧 DSL 运行器用于维护和回归；当前正式生成链使用 HTML+DSL。

| 页面类型 | 结构来源 | 组件来源 | 运行时职责 |
| --- | --- | --- | --- |
| ReferencePage | 开发维护的 `.ui` | 导入的 `GeneratedComponentSDK` | 页面类连接信号、读取表单值 |
| V1 | DSL 转换结果 | SDK 公共组件 | 预览布局和组件映射 |
| V2 DSL | 原生 DSL 生成的 `.ui` | SDK 公共组件 | 加载页面并推进工单流程 |
| V2 HTML+DSL | HTML 几何与 DSL 身份共同生成的 `.ui` | SDK 公共组件 | 加载页面并推进工单流程 |
| 节点案例 | 固定设计证据生成的 `.ui` | SDK 公共组件 | 验证静态生成与运行状态 |

## 文件分工

| 文件 | 负责内容 |
| --- | --- |
| `.ui` | 控件层级、布局、objectName、可静态表达的属性 |
| 页面 `.h/.cpp` | 页面构造初始化、数据接口、信号连接、运行时状态和业务动作入口 |
| 路线 mapping JSON | 设计身份、布局规则、组件属性到 API 的映射 |
| SDK 公共头和静态库 | 组件构造、绘制、状态、Token 与 QSS |
| 页面 QSS | 页面壳与局部布局视觉；通过页面 QRC 注册并在页面构造函数中加载，组件自身样式由 SDK 提供 |
| 页面 QRC | 注册页面 QSS，并在存在页面专属静态资源时登记这些资源；业务 CMake 将其编译进页面目标 |

SDK 页面构造按 `setupUi → 创建工厂 → 注册页面 QRC → 应用组件属性 → 页面 QSS` 顺序执行；public-qt 路径使用 QWidget 与公开 Qt API。页面链遇到未映射组件时停止并记录节点身份。组件实现和 Token 更新在 GenerateComponent 完成，发布后在本项目校验并直接引用。contract-only 类名合同用于映射检查和预览；正式运行页面需要可验证 SDK。目标库切换规则见 [组件库配置](../../../../designRules/library-profiles.md)。
