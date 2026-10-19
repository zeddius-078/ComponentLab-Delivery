# raw-extract · Token 原始数据存档（历史云档）

本目录保存从设计工具/交接文档中**原样提取**的 Token 原始数据（raw data）。
内容保真：未做任何值修改、键重命名或引用改写（原始拼写错误一并保留）。

> 本目录新增的 `token-layer-alignment-v2.*` 与 `theme-v2.json` 是“对齐基线文档”，不是对原始 JSON 的覆盖。原始文件继续作为证据保存；V2 规范化结果仍以 `Source/ComponentKit/design-assets/DesignSystem/source/` 下的 canonical 文件为唯一生成源。

> 注意：本目录**不参与 V2 管线**。`scripts/v2/model.py` 只读取
> `Source/ComponentKit/design-assets/DesignSystem/source/` 下的 canonical 文件；本目录与其隔离，改动不会
> 影响任何构建、发布或校验。

## 文件清单（由「新建 文本文档.txt」四段 JSON 拆分）

| 文件 | 层 | 来源段 | 说明 |
|---|---|---|---|
| `primitive.json` | Primitive | 第 4 段 | Color（highlight/neutral/cosmic/data_series/functional）、dimension、radius、spacing、sizing、line_width、opacity、boolean、font_family、font_weight、font_size、line_height、letter_spacing、paragraph_spacing |
| `profile-2560x1440.json` | Profile | 第 1 段 | size/layout/typography/temp（viewport 2560×1440，对应大屏/Wide 形态；`temp` 已判定为垃圾，不参与治理） |
| `profile-1920x1080.json` | Profile | 第 2 段 | size/layout/typography/temp（viewport 1920×1080，对应标准/Standard 形态；`temp` 已判定为垃圾，不参与治理） |
| `semantic.json` | Semantic | 第 3 段 | color/typography/border_width/opacity/radius/size/spacing/layout 角色，引用 `{Color/...}`、`{dimension/...}`、`{typography/...}` |
| `theme-v2.json` | Theme | V2 对齐快照 | 从当前 `source/theme.json` 提取的 Light/Dark 表面环境值；用于确认 Theme 已从 Semantic 中拆出 |
| `token-layer-alignment-v2.md` | 对齐说明 | 当前基线 | 记录 Primitive、Profile、Theme、Semantic 的来源、参数计数、命名归一化和待处理边界 |
| `token-layer-alignment-v2.json` | 对齐索引 | 当前基线 | 供脚本和评审读取的机器可读对齐状态 |

命名说明：两个 profile 文件按 viewport 分辨率命名（2560×1440 / 1920×1080），
避免在原始数据未确认前主观标注 Standard/Wide 语义。

## 引用语法提示（与 canonical V2 的差异）

- 原始数据使用 **斜杠路径** 引用：`{dimension/dimension-24}`、`{Color/neutral/gray-0}`。
- canonical V2 使用 **点路径**：`primitive.dimension.24`、`semantic.color.text.primary`。
- 根键大小写不统一：`Color`（大写）vs `dimension`（小写）。
- 结构性待优化清单见
  `reports/archive/2026-10-18/2026-10-18-raw-token-structure-issues.md`。
