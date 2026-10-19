# Token 原始数据 · 待优化结构清单（2026-10-18）

> 数据来源：`docs/archive/design-evidence/raw-tokens/raw-extract/`（四个原始 JSON，由交接文档拆分）。
> 本清单只做**结构与引用审计**，不修改原始数据。转换为 canonical V2 源时按本清单逐项处理。

## A. Primitive 层（primitive.json）

| # | 位置 | 问题 | 建议 |
|---|---|---|---|
| P1 | `Color.functional.bule` | 拼写错误（bule→blue），且其内容与 `red` 组完全重复（red-5..red-100 全套） | 删除；blue 组已有正确内容 |
| P2 | `Color.functional.yellow` / `orange` / `red` 的 `*-42` 与 `*-8` 键 | yellow-42/orange-42/red-42 三处都是 `#F76E7B`（粉红），yellow-8/orange-8 也是 `#FBE4E5`（粉色）——显然复制粘贴错值 | 按各自色系梯度重定：yellow-42 应位于 yellow-40(#EEC771) 与 yellow-45(#EBBF5B) 之间；orange-42 在 orange-40(#F3A275) 与 45(#F09563) 之间；red-42 在 red-40(#F2766B) 与 45(#EE655B) 之间 |
| P3 | `Color.functional.yellow` 键序 | yellow-42 插在 yellow-35 与 yellow-40 之间（乱序） | 重排 |
| P4 | `Color.data_series."Color 2"` | 键名含空格、无系列语义、仅一个白色值 | 重命名为 data-1..data-N 序列并补全 |
| P5 | `Color.highlight.purple-68/74/84` | 与 purple-65..75 主梯度不连续（#8F85DB/#8076CE/#7267C1 属另一套紫色，偏灰） | 裁决：并入主梯度重排，或移入历史色组 |
| P6 | `Color.cosmic` | 与 functional/neutral 大面积重合（blue-cosmic=blue-55、yellow-nebula=yellow-55、orange-prome=orange-60、red-flare=red-60） | 裁决：别名指向或删除 |
| P7 | `dimension` 键序 | dimension-30 排在 32 之后、整体乱序；存在 22/26/72/80/96/108/128/256/800/1200 等低频值 | 排序；统计引用面后决定保留/裁剪 |
| P8 | `dimension` vs `spacing` vs `sizing` | 三套数值体系重叠（dimension-0..64 ≡ spacing-0..64；sizing.control-*/icon-* ≡ dimension 部分值），语义层引用时混用 | 统一为单一 primitive 主干 + 语义别名 |
| P9 | `line_width` 键命名 | 100/125/150…（数值×100）与 none/050/075 两套命名混编；"无线条"描述错贴在 100/125/150 上（应为 1/1.25/1.5px） | 统一 `w-0.5` 风格或 DTCG 命名；修正描述 |
| P10 | `opacity` | 键为数字字符串（"0".."100"），值 0-100，单位未标注（百分比？）；语义层 state.hover=5 的含义需明确 | 标注单位或转 0-1 系数 |
| P11 | `font_size` / `line_height` | 键为数字字符串；font_size 与 line_height 数值重叠易混 | 保持数值键但注明单位 px |
| P12 | `paragraph_spacing` | 四项全为 0（占位） | 补值或删除 |
| P13 | `radius.full` | 999px 魔法值 | 改为明确的 9999 常量或语义化 |

## B. Profile 层（profile-2560x1440.json / profile-1920x1080.json）

| # | 位置 | 问题 | 建议 |
|---|---|---|---|
| F1 | `typography.fontsize` | 04/05/07/08（及 2560 版 10）引用 `{line_height/*}`——**字号槽引用行高 primitive**，疑似错位（如 fontsize/04 应指 18-20px 字号却指向行高 24） | 逐项核对设计稿，改为 `{font_size/*}` |
| F2 | `typography.fontsize."10"` | 两套 profile 引用不一致（2560 版 → font_size/44；1920 版 → line_height/40） | 统一 |
| F3 | `typography.letterspecing` | 拼写错误（letterspecing→letter-spacing） | 修正 |
| F4 | `temp` 段 | 已确认是垃圾数据，不属于公共 Token 或产品布局事实 | **关闭：不迁移、不映射、不校验、不发布；仅可作为 Raw 归档中的废弃内容处理** |
| F5 | `size.visualization.v-navigation-width` | 引用 dimension-2560/1920——值等于整个视口宽度，语义可疑 | 核对设计意图 |
| F6 | `layout.grid.column-count` | 引用 `{dimension/dimension-24}`——24 列数借用了 24px 的 dimension 值（类型耦合） | 列数应为无单位 number，独立定义 |
| F7 | 两套 profile 的 spacing/padding 阶梯 | 1920 版 s02=4、s03=8（压缩阶梯），2560 版 s02=8、s03=12——阶梯错位方式不同 | 核对是否设计意图（紧凑/宽屏分级） |
| F8 | `layout.viewport` 分辨率 | 1920×1080 与 2560×1440 两套，与 canonical Standard/Wide 的对应关系待确认 | 与 Figma 集合模式对齐后命名 |

## C. Semantic 层（semantic.json）

| # | 位置 | 问题 | 建议 |
|---|---|---|---|
| S1 | 引用风格混用 | `{Color/...}`、`{dimension/...}`、`{typography/...}`（语义自引用）、`{spacing/spacing-8}`、`{layout/spacing/s03}`、`{radius/md-4}` 多种前缀并存 | 统一引用 canonical primitive/profile 路径 |
| S2 | `color.status.warning` | 引用 `{Color/functional/yellow}`——yellow 是**包含子键的对象**，非叶值，引用非法 | 改为 `{Color/functional/yellow/yellow-55}` |
| S3 | `color.background.hover-dark/pressed-dark` | dark 变体混入浅色语义集 | 拆分为 Light/Dark 模式覆盖 |
| S4 | 描述批量错贴 | "禁用状态文字" 出现在 text.active-brand / waring / border.primary 等；"页面主要文字" 出现在 border.primary；"弱化信息" 出现在 border.quaternary | 逐项重写描述 |
| S5 | 拼写/大小写 | waring→warning、emphsis→emphasis、tooltip-cemera→camera、DefualtAuxiliary→DefaultAuxiliary、letterspecing→letter-spacing、SubTitle/Default/Navigation 大小写混用 | 统一 kebab-case |
| S6 | `color.status.waiting/process/process-soft` | 三者全部引用 gray-25，描述均为"高亮强调文本"——状态色未区分 | 按设计稿区分状态色 |
| S7 | `typography.fontsize.heading*` | 语义层 fontsize 引用 `{typography/fontsize/06}`——与 profile 层 `typography` 根重名，引用落在哪层不明确 | 明确分层：semantic.typography 引用 profile.typography |
| S8 | `spacing.section-gap/iconText/related/default/separated` | 混用 `{dimension/*}` 与 `{spacing/*}` 两套 primitive 引用 | 统一 |
| S9 | `layout.viewport.grid.marginInline` | 硬引用 `{dimension/dimension-20}`，绕过 spacing 阶梯 | 改引 `{layout/spacing/*}` |
| S10 | `opacity` 语义根 | semantic.opacity 与 primitive.opacity 同名根，引用 `{opacity/100}` 歧义；值语义（5%=hover 蒙层）未解耦 | 语义层改 percentage/0-1 命名或加注单位 |
| S11 | 缺 Dark 模式 | 无任何 dark 覆盖（除 background 的 hover-dark/pressed-dark 混入项） | 单独建 dark 语义集 |

## D. 跨层

| # | 问题 | 建议 |
|---|---|---|
| X1 | 引用路径斜杠 + 根键大小写不统一（`Color` vs `dimension` vs `typography`） | 转 canonical 点路径时统一 |
| X2 | semantic 的 `typography` 根与 profile 的 `typography` 根同名 | 分层引用明确化（见 S7） |
| X3 | profile.temp 与 semantic 无引用关系（死数据风险） | 迁出（见 F4） |

## 处理顺序建议

1. **先修 Primitive 数据错误**（P1-P3、P6、P9 描述）——基础数据错值会传染全链。
2. **再定 Profile**（F1-F8，含 temp 迁出与 Standard/Wide 对应确认）。
3. **最后重写 Semantic 引用**（S1-S10），引用修正依赖前两层稳定。
4. 全部完成后按《Component层Token构建规范》与 v2-token-schema.json 转入 canonical V2 源，并重跑 `export_tokens.py --check`。
