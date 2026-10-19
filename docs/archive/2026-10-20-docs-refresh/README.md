# 2026-10-20 文档整理前快照

范围：工作区的 6 个现行说明、指导、Main Skill、参考文档及相关图片。原文按所属项目的原相对路径保存；复制时逐文件比对 SHA-256，未改写内容。

这些文件包含旧路径、阶段数量、测试结论和过时限制，不再作为当前操作依据。现行说明见 [文档中心](../../README.md)。固定构建输入与生产源码没有因本次整理而迁移；历史记录保留，不等于认可旧结论仍有效。

整理内容：Source/docs/build 职责；七层 Token 与 Theme/Profile 分离；SDK 直接消费；多组件库及 classPrefix；contract-only 边界；固定安装环境与 Python 3.9 兼容目标；导航栏 Token 问题保持挂起。

## 本次整理验证记录

- [快照清单](snapshot-manifest.json)登记四个归档位置共 65 个原文件的路径、字节数与 SHA-256；归档说明另用独立索引，避免覆盖旧项目 README。
- 更新 4 份 Main Skill；quick_validate.py 在现有 Python 3.14 中以 `-X utf8 -B` 执行，4/4 通过。首次使用系统默认 GBK 读取中文失败，仅切换本次命令的 UTF-8 参数后通过，未安装或改动环境。
- 活动文档本地链接检查：39 份 Docs/项目 README、80 个本地链接，无失效目标。
- GenerateComponent 与 GenerateLayout 项目检查通过。前者输出的 registeredRuntimeComponents / engineeringGaps 是静态登记检查，不代表实际构造、运行或视觉验收。
- ComponentLab 项目检查失败：`demo V2 component map is stale`。现有演示包映射与当前 SDK 不一致；不在文档任务中重建发布。
- `git diff --check` 对本次文档范围通过，仅提示既有 CRLF/LF 归一化警告。
- GenerateLayout 的文档导航检查增加 docs/archive 排除，与 GenerateComponent 一致；归档保留旧路径，不重写原文以迁就当前链接检查。未改页面编译、组件实现或 Token 生成逻辑。
- 未执行 SDK 发布、完整构建、Figma 写入、目标类运行或真实 Python 3.9 测试；不据文档检查声称这些验收通过。
