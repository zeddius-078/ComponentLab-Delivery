# Token 层对齐基线（V2）

> 更新时间：2026-10-19  
> 目的：把当前 raw-extract 中的原始 Token 与 V2 的 Primitive、Profile、Theme、Semantic 层建立可追溯关系。本文不覆盖原始 JSON，也不把生成产物重新变成设计源。

## 结论

当前先完成四层边界和最小引用链：

```text
Primitive → Profile → Theme → Semantic
                              ↓
                     后续 Composite / Component
```

本轮最低验收是 Primitive 与 Profile 参数对齐；Semantic 保留已有内容，但允许继续存在待清理引用。Component 层 Token 不在本轮补齐，也不从 `generated/component.json` 反向导入。

## 当前文件与状态

| 层 | 原始证据 | 当前规范化文档 | 状态 |
|---|---|---|---|
| Primitive | `primitive.json`，279 个叶 Token | `Source/ComponentKit/design-assets/DesignSystem/source/primitive.json`，303 个叶 Token | 已建立参数映射；raw 错值与重复值保留在待处理清单 |
| Profile | `profile-1920x1080.json`、`profile-2560x1440.json`，各 98 个叶 Token | `Source/ComponentKit/design-assets/DesignSystem/source/profile.json`，Standard/Wide 两个 Mode，共 142 个叶 Token | 已按 1920×1080 → Standard、2560×1440 → Wide 对齐；`temp` 明确判定为垃圾数据，不纳入治理 |
| Theme | 原始文件未独立拆出 | `theme-v2.json` / `Source/ComponentKit/design-assets/DesignSystem/source/theme.json`，18 个叶 Token | 已从 Semantic 的 dark 变体中拆出 |
| Semantic | `semantic.json`，257 个叶 Token | `Source/ComponentKit/design-assets/DesignSystem/source/semantic.json`，310 个叶 Token | 保留可用语义；下一阶段再清理命名与引用 |

## Primitive 对齐规则

Primitive 只保存可复用的原子值，不表达主题、分辨率、组件状态或业务语义。raw 的 `Color`、`dimension`、`radius`、`spacing`、`sizing`、`line_width`、排印和透明度参数，统一映射到 canonical 的小写点路径。

本轮不静默修正原始颜色或数字。`P1-P13` 仍是待裁决证据；只有进入 `source/primitive.json` 的值才可参与生成。

## Profile 对齐规则

两个 raw Profile 结构路径相同，但 90 个值存在差异。差异被解释为 Profile Mode 差异，而不是两个独立 Token 层：

| Raw 文件 | V2 Mode | 允许表达 |
|---|---|---|
| `profile-1920x1080.json` | `standard` | 标准分辨率、标准间距、标准排印 |
| `profile-2560x1440.json` | `wide` | 宽屏分辨率、宽屏间距、宽屏排印 |

Profile 只负责平台、分辨率和密度适配。`temp.*` 已明确判定为垃圾数据，不进入公共 Profile、Canonical、Figma 或组件生成；其存在不再构成对齐阻塞项。

## Theme 拆分规则

Theme 负责 `light/dark` 和表面环境。Semantic 不再用 `hover-dark`、`pressed-dark` 这种混合命名表达主题；语义角色引用 Theme，Theme 再引用 Primitive。`dark` 不得成为 Button Intent 或其他组件的公共轴值。

## Semantic 当前边界

Semantic 本轮只要求“有可用内容并能沿引用链解析”。优先级是：背景、文字、边框 → Theme；尺寸、间距、排印 → Profile；原子色值、尺寸和透明度 → Primitive。完整角色命名、错误引用和历史拼写在下一轮处理。

## 不能做的事

- 不把 `raw-extract` 中的原始拼写或错误值直接发布到 Figma。
- 不把 `generated/component.json`、组合展开结果或 运行时 Token 作为 Primitive/Profile/Theme/Semantic 源。
- 不因 1920 与 2560 的差异复制第二套 Profile 层；使用同一组 Profile 参数和 Mode 覆盖。
- 不为 `temp.*` 建立迁移、映射或补值任务；该组仅作为原始归档中的废弃内容处理。
- 不在 Semantic 中重新创建一套 dark 值；主题值统一回收到 Theme。

完整的机器可读索引见 [token-layer-alignment-v2.json](token-layer-alignment-v2.json)。结构问题仍见 [2026-10-18-raw-token-structure-issues.md](../../../../../reports/archive/2026-10-18/2026-10-18-raw-token-structure-issues.md)。
