---
name: generate-layout-library-switch
description: Use when selecting a target component library, changing classPrefix or verifying planned class mappings for GenerateLayout. For capture-to-UI generation or missing build dependencies, use workspace-build-and-layout first.
---

# 组件库切换入口

先读取工作区 AGENTS.md 和 `DesignToCode_GenerateLayout/docs/mainSkill/generate-layout-pipeline/SKILL.md`，再读 `docs/designRules/library-profiles.md`（相对 GenerateLayout 项目）。

当前配置在 `docs/fixtures/layout/mapping/library-profiles.json`；默认 token-sdk。analysis-system 独立 JSON 的 classPrefix 改变目标类名与自动头文件名，仍为 contract-only/planned，不得声明已实现。布局与 Spacer 保持 Qt XML。

正式和预览入口均支持 `-LibraryProfile`；不要使用已删除的 Source/Qt、Source/Validation 或 Source/HtmlDsl/pipeline/mapping 路径。首次缺 SDK/编译器走工作区 Workspace.ps1，不能临时构造伪组件。专项检查使用 Python 3.9：

```powershell
py -3.9 -B -m unittest discover -s docs/validation/layout -p test_library_profiles.py
py -3.9 -B -m unittest discover -s docs/validation/layout -p test_component_libraries.py
```

在 GenerateLayout 根执行。专项命名通过不等于 capture/adapt/compile/visual 全链通过。
