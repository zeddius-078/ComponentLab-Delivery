# ReferencePage 标准实施方案

ReferencePage 展示开发仓库可直接接手的 `.ui + .h + .cpp` 页面结构，并验证已发布组件如何进入 Designer XML。

| 文件 | 作用 |
| --- | --- |
| `Source/Qt/ReferencePage/ReferencePage.ui` | 声明布局和 SDK 中的自定义控件类 |
| `ReferencePage.h` | 暴露页面字段、设值接口和业务信号 |
| `ReferencePage.cpp` | 初始化选项、连接信号、切换 busy/error 等运行态 |
| `ReferencePage.source.json` | 记录来源类型、组件对象和布局 Token 的已解析值 |
| `tests/test_reference_contract.py` | 检查 XML 类名、objectName、来源与布局绑定 |
| `tests/ReferencePageTests.cpp` | 检查四个 Profile、交互状态和高 DPI |

## 组件进入 .ui

`.ui` 的 customwidgets 节点声明 SDK 公共类。直接具备 `QWidget* parent` 构造的组件可直接使用；需要 Profile 参数的组件通过 SDK 的 `designer_widgets::*` 薄适配类创建。页面源码不复制组件算法。

## 构建

```powershell
pwsh -File Source/Import-GeneratedComponentSDK.ps1 -Package '<GeneratedComponentSDK目录>'
pwsh -File Source/Qt/ReferencePage/Build.ps1 -Test -Deploy
```

构建读取 `build/sdk`。ReferencePage 不包含 Token 生成参数；更新样式时重新发布并导入 SDK。
