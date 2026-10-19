---
name: generate-layout-library-switch
description: Use when switching the target component library of DesignToCode_GenerateLayout output (token-adapted Z Design System / open-source Qt / self-use library), replacing the component mapping table by component name (batch rename or per-component override), or running the two point-UI demos. Covers library-profiles.json, --library-profile, and the review / review-qt point views.
---

# GenerateLayout 组件库切换

## 目标

在不修改设计契约 `Source/HtmlDsl/pipeline/mapping/component-mapping.json` 的前提下，
切换生成页面引用的组件库：

- `token-sdk`：Z Design System（`commonWidget::Z_*` + `factory.apply` + 组合 API）
- `qt-native`：开源 Qt（`Q*` 内置类，公开 Qt API 直接序列化进 `.ui` XML 属性）
- `custom-library`（示例）：自用库（`MyOrg::*`，渐进式适配）

## 按组件名替换映射

只编辑 `Source/HtmlDsl/pipeline/mapping/library-profiles.json`：

1. 批量改名：`classRenames`（正则，顺序应用）
2. 单组件覆盖：`componentOverrides`（键 = `component-mapping.json` 的 `designComponentName`；
   优先级高于批量改名）

覆盖示例见 `docs/designRules/library-profiles.md`。

## 生成命令（复用现有编译器，禁止新建脚本）

```powershell
# 先构建编译器
pwsh -File Source/Qt/V2/Build.ps1 -CompileOnly

# 指定库配置生成（默认 token-sdk）
& build/cache/V2-html-dsl/v2_ui_compiler.exe `
  --raw Source/HtmlDsl/pipeline/design-inputs/review-page/canonical.raw.json `
  --mapping Source/HtmlDsl/pipeline/mapping `
  --extensions Source/HtmlDsl/pipeline/mapping/extensions.json `
  --font build/demo/page_examples/V2/data/html-dsl/HarmonyOS_Sans_SC_Regular.ttf `
  --output build/demo/page_examples/V2/data/html-dsl/review-qt `
  --strict-layout --library-profile qt-native
```

正式交付路径：`Generate.ps1` 已支持 `-LibraryProfile`；**预览路径
`Generate.ps1 -Preview` 不支持**（`Generate-Pages.ps1` 未透传该参数，见下）。

## 两个点位 UI

```powershell
Run-HTML-DSL.cmd --page review     # Z Design System 映射
Run-HTML-DSL.cmd --page review-qt  # Qt 原生映射
```

## 验证

```powershell
python -B Source/Validation/layout/test_library_profiles.py
pwsh -File Source/Qt/V2/Build.ps1 -Test   # 含 pointUiLibrarySwitchLoadsBothMappings
```

## 已记录限制（不要绕过）

1. 预览管线不接收 `-LibraryProfile`（`Generate-Pages.ps1` 未透传）。
2. 预览采集清单 `inputs/result-page/capture.json` 的 `htmlSha256` 与 `page.html` 不一致
   （`CAPTURE_HASH_MISMATCH: htmlSha256`），完整预览/正式管线当前跑不通。
3. qt-native 下 SelectOption 失败关闭（`SELECT_OPTION_REQUIRES_TOKEN_SDK`）。
4. 无公开 Qt 等价物的类失败关闭（`UNSUPPORTED_QT_NATIVE_CLASS: <类名>`），必须显式覆盖或移除。
5. qt-native 几何基于公开 Qt 控件测量，像素级视觉对等不在其契约内。
