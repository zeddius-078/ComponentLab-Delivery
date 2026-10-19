# 问题记录分析：表象 → 真问题 → 执行意见（2026-10-18）

> 约束遵守：不更新/安装环境；全部方案基于现有 Qt 5.15.2 (msvc2019_64) + MSVC 14.51 + Ninja；有执行问题先向用户提问。
> 关联现状：V2-only Token 体系（43 配方 + Registry + component_compile.py）、SDK 静态库 sole-publisher（`build/sdk`）、ComponentLab ctest 17/17 全绿、全部变更未提交 git。

## 0'. 用户答复与方案修订（2026-10-18 第二轮）

1. **cmake.txt**：运维方写的部署环境 CMake 描述文件，**不推荐轻易改动此结构**。→ ② 的 SDK 重构改为**非侵入式**：`build/sdk` 现有目录布局与 install 接口保持不动（运维消费面冻结）；"源包含"作为**新增的并行消费模式**（增量目录 + 增量 CMake 选项），不替换现有交付面。
2. **① 目标平台含 Linux**，.dll/.so 双形态是否立项由我方给建议。→ **建议：现在不立项**。原因：② 源包含是平台无关的（同一份 .cpp 在 MSVC/Linux 各自编译），交付路径上 .dll/.so 问题自然消解；只有"热更新不重启"成为硬需求时才需要插件化，届时用 QPluginLoader 天然双形态（Windows .dll / Linux .so，Qt 抽象掉差异），立项触发条件写死为"热更新需求确认"。
3. **⑪ 核心诉求澄清**：不是"弹窗"或"无边框"二选一，而是 **Qt 原生窗体框（含 OS 自绘的最小化/最大化/关闭按钮）与 UCD 设计稿对不上**，需要"样式可改"或"禁用原生顶部窗体后自绘"。→ 技术事实：QSS 无法重绘 OS 自绘的标题栏按钮（Windows 由 DWM 绘制、Linux 由窗口管理器绘制），**唯一可靠路径 = 禁用原生窗体 + 自绘标题栏**（Qt::FramelessWindowHint/CustomizeWindowHint + Z_TitleBar，与 ⑦ 无底色按钮联动）；QMessageBox 同理替换为自绘 Z_Dialog。Windows 拖拽/缩放需 nativeEvent(WM_NCHITTEST)，属标准做法，不引新环境。
4. **⑫ 穿梭框现实现**：`commonWidget::Z_Transfer`，源码在 SDK 运行时 `runtime/component/Z_FoundationComponents.{h,cpp}`（h:298 / cpp:783）。已确认其真问题：构造器 `Tokens tokens(profile)` 后 `Q_UNUSED(tokens)`——**零 token 消费**；面板固定 205×180、按钮固定 44×36、边距 14/12/8 全部硬编码；API 有 Disabled/Loading 状态但 Loading 无视觉接线。列表已用 QListView（非 QListWidget），改造面主要是"几何/色彩 token 化 + 状态视觉 + 按钮变体"。


## 0. 总体诊断：14 条记录 = 3 个真问题族

| 族 | 覆盖条目 | 真问题 |
|---|---|---|
| A. 交付/集成形态 | ① ② ③ ④ ⑧ ⑨ | 组件库"静态库唯一发布方"造成库/组件一致性漂移与资源散装；需要单一源码事实源 + 统一资源包 |
| B. 主题与变体机制 | ⑩ + ⑪（部分） | QSS 按 profile×组件碎片化预生成，品牌切换 = 重新编译；需要单文件聚合 QSS + 运行时替换 |
| C. 组件缺口 | ⑤ ⑥ ⑦ ⑫ 等待状态 | 缺的可视/布局/窗体/弹层组件都是"既有 token + 既有基件的组合"，不是新设计体系 |

---

## 1. 当前最大问题（A 族）

### ①"按照 .so 找组件"

- **表象**：按 .so 动态库发现/加载组件。
- **真问题**：组件发现与注册目前是编译期硬编码（ComponentFactory + registry.json + 静态 lib），消费者无法在**不重编译应用**的情况下增删组件。背后的真实诉求是"增量交付"和"组件实现独立演进"。注意 Windows 下对应物是 .dll（.so 是 ELF/Linux 产物），说明这条可能来自 Linux 侧协作方的表述。
- **与当前方案的关系**：Qt 5.15.2 自带 QPluginLoader/Q_DECLARE_INTERFACE 插件机制，无需新环境；但插件化会先放大"接口契约维护成本"，而契约漂移正是 ② 要解决的事。
- **执行意见**：**现在不立项（用户已确认目标平台含 Linux，双形态问题由我方裁决）**。理由：② 源包含是平台无关交付路径（同一份 .cpp 在 MSVC/Linux 各自编译），.so/.dll 分歧自然消解；只有"热更新不重启"成为硬需求时才插件化，届时 QPluginLoader 天然双形态（Windows .dll / Linux .so），且插件清单由现有 component-registry.json 派生，不另造注册表。立项触发条件：热更新需求书面确认。

### ②"不打包库，仅通过头文件引用组件，避免库/组件一致性冲突"

- **表象**：不再打包静态库，仅头文件引用。
- **真问题**：当前"sole-publisher"静态库模型下，组件/token 每次变更都必须"重发布 build/sdk → 消费者重链接"，漏跑就出现旧库对不上新源（本会话已实际踩过：缺失 token 键、旧类签名导致的未处理异常）。真实诉求是：**组件 C++ 源码与消费应用同一套编译配置、同一时间编译**，消灭"库漂移"。
- **与当前方案的关系**：SDK 内 C++ 源码（Z_*、ComponentFactory、运行时）本来就在仓库里；token/QSS/映射是发布期生成的确定性产物。把 C++ 部分改为**源包含**（source-inclusion）完全可行，且不需要任何新工具。
- **执行意见（P0，优先执行；非侵入式）**：
  1. SDK 包**增量**扩展为四段：`include/`（头）+ `src/`（.cpp，消费者直接编译）+ `resources/`（生成物：QSS×4、tokens_qt.json、映射、字体、SVG）+ `cmake/`（package config）。
  2. **运维消费面冻结**：`build/sdk` 现有目录布局、lib 名、install 接口保持不变（cmake.txt 描述的结构不动）；源包含是**新增并行模式**（新目录 + 新 CMake 选项），老路径照常可用。
  3. 消费者 CMake 两种模式可选：老（静态 lib）与新（add_subdirectory/FetchContent/target_sources）；`V2_COMPONENT_*_PATH` 宏在源包含模式改为构建期 `target_compile_definitions` 注入。
  4. 验证：ComponentLab 切到源包含后 ctest 17/17 保持绿色；SDKConsumerTest 两种模式各冒烟一次；`CheckProject.py` 两种 sdkMode 均可 PASS。

### ③"在 build 中加 resource 路径，包中含 ComponentLab 组件资源内容"

- **真问题**：运行时资源（QSS、tokens_qt.json、figma 资产、字体、映射）目前散落在 SDK resources 与 ComponentLab qrc 两处，交付布局没有单一 staging 目录。
- **执行意见**（P1，与 ② 同一批）：定标准目录 `build/sdk/resources/`（已存在）+ 新增聚合目标 `componentlab-resources`：发布时把 QSS×4、tokens、映射、字体、figma 资产拷贝进一个 `resource/` 子包；消费者只认一个前缀宏。CMake 用 custom command（copy 文件清单由编译阶段生成的 manifest 驱动），不引入新工具。

### ④"考虑组件整体打包"

- **真问题**：交付粒度 — 现在消费者要分别接线 lib/头/映射/qrc/QSS/字体六个面，任何一个漏接都失败。
- **执行意见**：与 ② 合并为同一动作。"整体打包"= ② 的源包含包 + ③ 的资源包 + 一个 `cmake/` 配置入口，最终一个 `--install` 前缀完整可用。不必单独立项。

### ⑧"cmake.txt"

- **表象**：仅一个文件名，无法判断是"已有的描述文件"还是"要产出的打包描述"。
- **执行意见**：**需要用户确认**。若是指协作方写的 CMake 打包描述：请提供文件内容/路径，我按 ②③④ 方案评审合并进 SDK package config；若指"想要一份 cmake.txt 说明"：并入 `cmake/ComponentSDKConfig.cmake.in`，不单列 txt。

### ⑨"业务侧会有一个总的资源文件用于索引 resource.qrc"

- **表象**：业务侧想用**一个**总资源文件索引全部组件资源。
- **真问题**：qrc 不能跨文件 include；今天每个消费者要自己维护 qrc 条目，组件资源一多就错漏。
- **执行意见**（P1）：发布期用 CMake/脚本**自动生成一个聚合 `component-resources.qrc`**（把资源清单全量罗列，含字体/SVG/静态资产）。业务侧仅 `add_resources(component-resources.qrc)` 一次。**注意区分**：QSS/tokens_qt.json 是"可替换变体"（见 ⑩），建议仍走外部文件路径而非嵌入 qrc，品牌切换才不用重编译业务应用；静态资产（字体/SVG）进 qrc。

---

## 2. 主题与变体机制（B 族）

### ⑩"所有组件变体在一整个 QSS 中编辑；切换品牌 token 时替代 QSS 变体"

- **表象**：单一 QSS 承载全部变体；换品牌 = 换 QSS 文件。
- **真问题**：当前主题 = 4 个预生成 QSS（standard/wide × light/dark）× 按组件分文件（Z_Button、Z_CheckBox、native 等），品牌/上下文切换要重跑编译器并重编译。真实诉求：**(a) 单一聚合 QSS 文件（每上下文一个）**；**(b) 品牌 = 一组 token 值 → 一个 QSS 变体文件，运行时整文件替换**。
- **与当前方案的关系**：V2 编译链已有全部能力：合同 → C++ 编译器 → 每上下文 QSS；缺的只是"聚合输出阶段"和"运行时替换加载器"。不涉及新环境。
- **执行意见**（P1）：
  1. 编译器链新增聚合阶段：`componentlab-all.{context}.qss`（把 button/checkbox/forms/native 各 QSS 按顺序拼接 + 头部版本注释），并随 `token-manifest.json` 记录"QSS ↔ token 上下文 ↔ profile"映射。
  2. 新增 `Z_ThemeManager::apply(context)`：读聚合 QSS → `qApp->setStyleSheet` + repolish；品牌切换 = 发布期生成新 QSS 集 → 业务侧换文件重启生效（**不重编译业务应用**）。
  3. 校验：d2c_check 增加聚合文件哈希比对；ComponentLab 冒烟切 4 上下文各跑一次。

### ⑪"弹窗和浮层都没有，qt 系统默认的系统 os 窗体无法取消显示"

- **表象**：缺弹窗/浮层组件；系统 OS 窗体无法去掉。
- **真问题（用户已澄清）**：Qt 原生窗体框（含 OS 自绘的最小化/最大化/关闭按钮）与 UCD 设计稿对不上，需要"样式可改"或"禁用原生顶部窗体后自绘"。技术事实：**QSS 无法重绘 OS 自绘的标题栏按钮**（Windows 由 DWM 绘制、Linux 由窗口管理器绘制），所以"样式可改"在原生窗体上不成立；唯一可靠路径是**禁用原生窗体 + 自绘标题栏**。
- **与当前方案的关系**：token 已就绪（dialog-button-group 家族 token 在账本有记录；titleBar/pageHeader 组件已被 CompositionTests 验证）；frameless 用 `Qt::FramelessWindowHint | Qt::CustomizeWindowHint`，Windows 拖拽/缩放用 nativeEvent(WM_NCHITTEST) 标准做法，均不引新环境。
- **执行意见**（P2）：
  1. 主窗体/业务窗体：frameless + 自绘 Z_TitleBar（与 ⑦ 无底色按钮联动交付），提供 Drag/resize 处理封装 `Z_FramelessHost`（nativeEvent 命中测试 + 双击最大化）。
  2. 弹窗：QMessageBox 场景替换为自绘 `Z_Dialog`（QDialog + dialog token + 自绘头部/按钮区），模态与非模态均可。
  3. 浮层：`Z_Popup/Z_Overlay`（QWidget + Qt::Popup/Tool 标志）承接 tooltip 之外的浮层需求。
  4. 说明：Linux 下 frameless 由窗口管理器差异引入边角情况，先交付 Windows 语义，Linux 侧由运维环境验证时再适配（同 ① 的双形态时机）。

---

## 3. 组件缺口（C 族）

### ⑤"可视图组件：顶部标题（标题 + 叉号），下方一张图"

- **真问题**：需要可复用的"可视化面板"组件：标题栏（标题文本 + 关闭叉号）+ 内容图。现有 titleBar（含 3 个 QToolButton、focus 契约）与 chart token（`component.chart.base.state.default.slot.content.*`）已齐。
- **执行意见**（P2）：新建 `Z_VisualPanel` = Z_TitleBar + 图像内容区组合；标题按钮用"无底色"变体（见 ⑦）；若图表家族无"image/viewer"槽位，按规范流程在 chart 配方 + Registry 补槽位与状态，再跑 component_compile.py 三阶段校验。不做新设计层。

### ⑥"布局：侧边抽屉栏（可向右收起）；左两个表分区；右纵向图组（每图一文本标签）"

- **真问题**：一个复合页面：可折叠抽屉 + 左双表 + 右图组列表。这是**应用级组合页**，不是单一 widget；组件库要补齐的是"可折叠抽屉容器"与"图组列表项"两个基件。
- **执行意见**（P2）：抽屉容器优先复用现有 navigation/side-menu token + QDockWidget 或自绘 QWidget 折叠动画；图组 = Z_VisualPanel 列表 + caption（⑤ 产物复用）；整体作为 ComponentLab 新 demo 页，带 figma 证据落地。若抽屉家族 token 缺失，按规范补配方/Registry。

### ⑦"窗口标题栏组件，需要支持无底色的图标按钮"

- **真问题**：现有 titleBar 的图标按钮继承普通按钮的 hover/pressed 背景 token，需要"ghost/无底色"按钮外观变体。
- **执行意见**（P2）：在 button 配方/Registry 增加 appearance 变体（如 `ghost`：默认背景透明、hover 用 `semantic.color.background.hover` 或透明），更新 button-contract 变体矩阵（注意 materialStates 契约格式与 R5-V2 版本线），重跑编译器与 contract 校验；Z_TitleBar 按钮改绑 ghost 变体；CompositionTests 补断言。与 ⑪ frameless 联动交付。

### ⑫"穿梭框优化"

- **表象**：穿梭框需要优化（此前未知现实现位置）。
- **真问题（已定位源码）**：`commonWidget::Z_Transfer` 在 SDK 运行时 `runtime/component/Z_FoundationComponents.{h,cpp}`（h:298 / cpp:783）。实测发现：
  1. 构造器 `Tokens tokens(profile)` 后 `Q_UNUSED(tokens)` —— **零 token 消费**，全部几何/色彩硬编码（面板 205×180、按钮 44×36、边距 14/12/8、字号 14/13）；
  2. API 已有 Disabled/Loading 状态（`setSnapshotState` 白名单含 Loading），但 **Loading 无视觉接线**（与"等待状态没有显示"同源）；
  3. 列表已用 QListView + model（非 QListWidget），数据层不用动；行移动由 demo 侧操作 model。
- **执行意见**（P2，可提前为独立小项）：几何（面板尺寸/边距/间距/按钮尺寸）改读 canonical token（transfer/selection-control 家族语义尺寸），色彩改语义角色；Loading 态渲染（禁用命令按钮 + 列表遮挡/指示）；命令按钮绑定 ghost/neutral 变体（与 ⑦ 同批）；完成后 ComponentLab CompositionTests 补 token 断言。

### ⑬"等待状态没有显示"

- **真问题**：loading/等待反馈缺失。token 与契约已就绪：progress-bar 配方含 `loading` 状态；button-contract B07 已定义 800ms 线性旋转加载动画策略——但运行时未接线渲染。
- **执行意见**（P2，小改动）：接线两处：Z_ProgressBar 的 indeterminate（不确定进度）模式绑定 `loading` 状态色；Z_PushButton loading 态渲染旋转指示器（QSS `@PRIMARY@` + 动画），禁用时暂停动画（B07 已规定）。补 CompositionTests 断言。

---

## 4. 执行优先级总表（第二轮修订）

| 优先级 | 条目 | 动作 | 依赖 |
|---|---|---|---|
| **P0** | ② | SDK 源包含**增量并行模式**（include/src/resources/cmake 四段新增；运维消费面冻结） | 无 |
| P1 | ③④⑨ | 资源聚合目录 + 自动聚合 qrc + 整体打包配置（增量，不改既有布局） | ② |
| P1 | ⑩ | 聚合 QSS 阶段 + Z_ThemeManager 替换 | ②（资源布局） |
| P2 | ⑦ | button ghost 变体 + 标题栏无底色按钮 | ⑩（变体矩阵） |
| P2 | ⑪ | frameless 自绘窗体（Z_FramelessHost + Z_TitleBar）+ Z_Dialog/Z_Popup 替换原生 | ⑦ |
| P2 | ⑤⑥ | Z_VisualPanel + 抽屉布局页 | ⑦ |
| P2 | ⑬⑫ | waiting 接线 + 穿梭框 token 化（含 Loading 视觉） | 无（已定位源码） |
| P3 | ① | .dll/.so 插件化（立项触发条件：热更新需求书面确认） | ② |

## 5. 执行前需向用户确认的问题（第二轮）

1. **P0 开工授权**：是否现在开始执行 ②（非侵入式源包含模式）？不动运维 cmake.txt 结构、不动现有 `build/sdk` 布局。
2. `cmake.txt` 内容（可选）：若运维方能提供，我按"消费面冻结"核对兼容点；不便提供则按上述冻结策略执行。
3. ⑫ 穿梭框与 ⑬ 等待状态是否合并为一个小批次提前交付（不依赖 ②）？
4. ⑪ 自绘标题栏的按钮规格（最小化/最大化/关闭的顺序与图标）以 UCD 设计稿为准——开工时需要图稿或等价描述。
