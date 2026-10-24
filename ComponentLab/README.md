# ComponentLab

ComponentLab 是组件展示与验收项目。它读取 GenerateComponent 发布的 `GeneratedComponentSDK`，用页面展示组件外观、状态、对象树和 Token Profile。

## 目录

| 目录 | 用途 |
| --- | --- |
| `Source/` | 展示窗口、页面控制器、测试和构建脚本 |
| `docs/` | 展示规则、教程和 Token 使用说明 |
| `build/` | 受控展示程序、支持工具和发布状态 |
| `output/` | 本次构建工作区、日志和展示包 |

组件实现、Token 编译和 SDK 发布由 `../DesignToCode_GenerateComponent` 负责。ComponentLab 只消费公开 SDK。

## 首次构建

在工作区根目录执行：

```powershell
pwsh -File Source/Workspace.ps1 -Action doctor
pwsh -File Source/Workspace.ps1 -Action build -Target gallery
```

直接在本项目构建时，先设置 `QT_ROOT`、`VS_ROOT`、`CMAKE_EXE`、`NINJA_ROOT` 和 `GENERATED_COMPONENT_SDK_ROOT`，再执行：

```powershell
pwsh -File Source/Verify-GeneratedComponentSDK.ps1
pwsh -File Source/delivery/tools/Build.ps1
```

## 展示内容

组件页按组件族展示外观和分组状态。注册信息页展示公开类名、设计身份、Token Profile、头文件和状态合同。ComponentLab 不通过展示页创建新组件，也不修改 SDK。

## 验收记录

结构、构造、交互、Token 变化和截图分别记录。展示程序启动成功只说明运行链路可用；组件是否适合业务接入，以独立 SDK 消费测试为准。

## 自动化输出基线

本项目的受控执行 SKU 为 `DS-V4F-BUILDUNK-NT-C256K-TEXT-AGENT-PROVIDERUNK`（DeepSeek V4 Flash 256K Non-Think）。自动化任务必须显式给定 SDK、组件族、Profile、场景和验证命令；缺失合同或展示差异记录并停止，不在展示层推断或补写组件实现。完整约束见工作区 [能力画像](../docs/designRules/architecture/agent-execution-capability-profile.json)。
