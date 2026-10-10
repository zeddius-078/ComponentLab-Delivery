# ComponentLab 使用说明

ComponentLab 回答两个问题：发布组件当前长什么样；属性、状态和 Token Profile 切换后如何表现。

## 使用

双击 `../build/demo/component_gallery/Run.cmd`。打开 `Z-DesignSystem` 的“组件图库”，在左侧组件树选择具体组件。右侧每个组件都有“外观”和“分组与状态”页，均展示属性操作、预览和实时对象树。Button、Checkbox、Input、Select 保留各自完整交互；其余组件提供文本、启用和默认/禁用状态的基础操作，便于逐项扩展。窗口类组件通过“打开组件”查看实际窗口。右上角切换亮暗模式和密度，焦点反馈可选关闭、仅键盘或始终显示。

| 可调整项 | 数据来源 | ComponentLab 的动作 |
| --- | --- | --- |
| Profile | SDK 内嵌的四套生成结果 | 重新启动演示并加载目标 Profile |
| 外部 Token 集 | GenerateComponent 产出的完整目录 | 通过 `--token-dir` 加载并展示 |
| 组件属性与状态 | SDK 公共 API | 在演示控件上调用，展示结果 |
| 页面组合 | `Source/delivery/demo` | 排列组件并记录交互状态 |

## 构建

```powershell
pwsh -File Source/Import-GeneratedComponentSDK.ps1 -Package '<GeneratedComponentSDK目录>'
pwsh -File Source/delivery/tools/Build.ps1
python -B Source/delivery/tools/CheckProject.py
```

`Build.ps1` 编译演示与展示测试，并部署离线运行包。它不编译 Token 或组件源码。组件更新流程为：GenerateComponent 发布 SDK，本项目导入 SDK，本项目重建并运行外观与状态测试。
