# ComponentLab 的 Token 变量集使用

ComponentLab 消费 GenerateComponent 已编译的 Token 结果，用于比较 Profile 与外部变量集的视觉效果。

| 能力 | 输入 | 运行方式 | 验收 |
| --- | --- | --- | --- |
| Profile 切换 | SDK 内嵌的 `light/dark × comfortable/compact` | 界面右上角选择 | 页面重启后四类组件使用同一 Profile |
| 外部变量集 | GenerateComponent 输出的完整 generated 目录 | `component_lab.exe --token-dir <目录> --profile <名称>` | manifest、QSS、catalog 均可读取 |
| 状态检查 | 组件 API 与演示操作 | 各组件“分组与状态”页 | default、hover、pressed、disabled、loading 等按组件能力呈现 |
| 焦点反馈 | SDK 的焦点配置与运行参数 | `--focus-indicator off|keyboard|always` | 模式与组件颜色状态可同时辨识 |

Token 的设计、映射、类型校验、QSS 编译和组件构建位于 [GenerateComponent 项目](../../../DesignToCode_GenerateComponent/README.md)。ComponentLab 只加载发布结果并展示，不修改发布包。
