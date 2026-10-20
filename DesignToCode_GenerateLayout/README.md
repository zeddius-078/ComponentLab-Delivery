# DesignToCode_GenerateLayout

## 项目定位

本项目把同一设计版本的 HTML 与 Pixso DSL 转换为 Qt 页面文件。它消费 `DesignToCode_GenerateComponent` 发布的公共 SDK，负责页面结构、布局、实例属性、页面 QSS 和页面资源。

| 目录 | 内容 |
| --- | --- |
| `Source` | 采集、适配、布局转换、页面编译、构建和校验源码 |
| `docs` | 规则、固定输入、样例、教程和验证代码 |
| `build` | 经过构建发布的稳定工具和固定运行材料 |
| `output` | 当前执行产生的缓存、页面包和日志 |

## 输入与输出

```text
Pixso DSL + D2C HTML + layoutReference
       -> capture manifest
       -> browser adapter
       -> component / layout compiler
       -> .ui + .h + .cpp + .qss + .qrc
       -> uic / consumer build / runtime
```

HTML 提供布局和文字测量，DSL 提供设计身份、节点关系、属性、状态和 Token 引用。capture manifest 保存二者的版本、路径和哈希。正式页面还需要布局证据、字体、资源和已发布 SDK。

组件源码、公共 QSS、Token 和工厂由 GenerateComponent 维护。ComponentLab 使用同一 SDK 展示状态。GenerateLayout 只生成页面文件和页面级资源。

## 入口

```powershell
pwsh -File Source/Workspace.ps1 -Action doctor
pwsh -File Source/Workspace.ps1 -Action build -Target all
pwsh -File Source/Workspace.ps1 -Action verify
```

正式页面：

```powershell
pwsh -File Source/HtmlDsl/Generate.ps1 `
  -CaptureManifest '<capture.json>' `
  -MappingDirectory '<映射目录>' `
  -Font '<字体文件>' `
  -Batch '<批次>' -Page '<页面名>'
```

页面包写入 `output/delivery/<批次>/<页面>`。临时数据写入 `output/work`，报告写入 `output/logs`。稳定工具经过构建后位于 `build/tools/layout`。

## 阅读顺序

1. [Skill 执行合同](docs/mainSkill/generate-layout-pipeline/SKILL.md)
2. [布局转换规则](docs/designRules/layout/布局转换器通用化规范.md)
3. [组件库 Profile](docs/designRules/library-profiles.md)
4. [上手指南](docs/tutorial/布局生成上手指南.md)
5. [验证代码说明](docs/validation/README.md)

Python 命令使用已安装的 Python 3.9。依赖路径由当前进程参数或环境变量提供；脚本不会修改系统环境。
