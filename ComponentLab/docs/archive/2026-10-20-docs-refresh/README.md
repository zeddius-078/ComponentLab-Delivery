# ComponentLab

ComponentLab 是已发布组件与 Figma 设计合同的展示、差距识别和验收程序。`Z-DesignSystem` 读取当前融合目录：10 个章节、98 个 `20.*` 规范页、148 个组件集、1616 个 Variant。权威目录共 110 个库条目，其中 105 个 Widget/Composite 组件身份、2 个 LayoutHelper、3 个 Asset；105 个组件身份现均可实例化，当前组件工程缺口为 0。图库导航由 SDK 的全量库条目接口动态分为“组件／布局辅助／资产”，三类分别统计；LayoutHelper 和 Asset 不进入组件 Factory。“外观”用于交互式调参和身份、属性检查；“分组与状态”用于按源稿展示变体。目录完整、可实例化、资产可消费和设计审批是独立状态。

| 目录 | 职责 |
| --- | --- |
| [Source](Source/README.md) | 演示页面、展示控制器、构建和验证入口 |
| [docs](docs/README.md) | 打开方式、Profile 切换和验收范围 |
| `../DesignToCode_GenerateComponent/build/sdk` | 工作区唯一权威组件 SDK；本项目直接读取，不保存副本 |
| `build/demo/component_gallery` | 98 个 Figma 规范页、148 个组件集，以及 105 组件／2 布局辅助／3 资产分区的离线演示程序 |
| `build/cache` / `build/reports` | 可重建缓存 / 验证结果 |

既有 73 个 Figma 页面已按源文件的 Component Set、Variant 名称、数量、层级与画布几何展示“分组与状态”：56 页使用专用原生或源稿画廊，17 页使用 `/figma/gallery-layouts.json` 驱动的运行时证据画廊。新增 25 页已进入统一目录和合同展示，所有 Widget/Composite 都使用真实 SDK 实例。Transfer 页面由 SDK `Z_Transfer` 展示 Default／Disabled／Loading 三个受控状态；SDK 组合两个列表和方向命令但不修改外部模型，集合移动、排序、权限与错误恢复由页面选择 controller 负责。Upload 页面由 SDK `Z_Upload` 展示 Default／Hover／Uploading／Success／Error 五个受控状态；浏览与拖放只产生请求，文件校验、网络传输、鉴权、取消和重试继续由外部上传服务负责。Timeline 页面由 SDK `Z_Timeline` 展示 Comfortable／Compact 两个密度变体，并保持只读事件历史职责。Stepper、Pagination 和 AnchorNavigation 页面使用专用 SDK 画廊展示其实际固有尺寸，避免通用矩阵拉伸或裁切；Pagination 展示完整 `1 … 3 4 5 6 7 … 29` 页序列、边界翻页按钮、实色当前页及禁用态。AnchorNavigation 同时验证受控激活、方向键移动焦点和组件级焦点环开关。Filter 页面由 SDK `Z_Filter` 展示 Inline／Panel × Default／Applied 四个变体；条件按钮与应用按钮保持受控请求，外部 filter controller 继续持有条件模型、校验、清除和持久化。ColorPicker 页面由 SDK `Z_ColorPicker` 展示 Default／Focus／Disabled 三个变体；色样和值使用同一受控 QColor，点击或键盘激活只发出选择流程请求，不直接创建系统对话框。NumericKeypad 页面由 SDK `Z_NumericKeypad` 展示 Comfortable／Compact 两个变体；组件只输出语义键和可选提交请求，外部 input controller 持有目标字段、locale 解析、约束、焦点路由和提交策略。三个资产页均消费已发布 SDK 且不进入组件 Factory：`IconAsset` 通过 `IconAssetCatalog` 展示 16／20／24 与 Primary／Secondary 六个 Figma 变体；`ImageAsset` 通过 `VisualAssetCatalog` 展示 Contain／Cover × Loaded／Empty／Error 六个变体；`ModelAsset` 展示 Ready／Loading／Error 三个二维原理图状态。Chart 明确覆盖 Bar／Line／Status × Data／Empty 六个 Variant，DataTable 明确覆盖 Size × State 十五个 Variant。证据画廊只在已有实现时实例化对应 `Z_*` 类；内部 Anatomy、LayoutHelper 和 Asset 不会被伪造成新的公开控件。

Badge、StatusIndicator、Link、Tag、Tooltip 的变体轴和文本槽同时由 GenerateLayout 消费。ComponentLab 仍只负责用同一发布 SDK 展示与验收，不持有第二套组件实现。

MDI Workspace 展示 6 个子窗口状态和 4 个工作区组合，使用原生 Z_MdiSubWindow／Z_MdiArea。固定 State 不执行最大化、最小化、激活或禁用命令；可切回跟随真实状态。工作区选变体后点击“应用所选布局”才执行 Qt 排列／viewMode 命令，不新增四值枚举。原生标题栏命中、拖动／缩放、关闭拒绝、页签关闭、removeSubWindow 对象保留及 WA_DeleteOnClose 已回归；演示关闭默认只隐藏。画廊专用冻结布局保留源稿 Tile 的 24px 重叠、Tabbed 换行和低对比徽标，交互页不安装冻结逻辑。Shade／Help 标题栏按钮组合无源稿规格，保留 Qt 样式回退；不宣称完整平台人工验收。

DockWidget 展示 8 个 Area × Placement 变体和 2 个组合样本。外观预览与实际停靠、allowedAreas、features、floating 分开；图标显隐不自动变成命令权限。原生按钮、内容对象和 QMainWindow 布局保留，可验证关闭后重显、浮动、页签化及仅内存的布局保存／恢复。真实浮动窗口和竖向标题栏沿用 Qt／平台外观，不冒充源稿的静态 Floating 样本。双击和 Ctrl 拖离已用与未加样式 Qt 控件相同的输入序列回归；不等于所有平台拖放目标均已人工验收。

ToolBox 展示 5 个内部 ToolBoxItem 状态与三页组合，公共类型仍为 Z_ToolBox。原生 currentIndex 始终指向唯一当前页，重复点击不收起；不是多开折叠面板。“外观”分别编辑目标条目、当前页、标签／助记键、图标、启用、键盘聚焦和 RTL。固定状态不更改选页、按下或焦点，其他条目的真实焦点仍可见。保留原生按钮和 Alt 助记键；仅接管绘制，removeItem 不删除页面。画廊的固定头部／内容宽度差异只用于忠实展示源稿。

Tabs & TabWidget 展示 20 个 Position × State 的 TabItem 变体，以及完整页签和溢出组合。TabItem 保持内部结构身份，公共类型仍为 Z_TabWidget。“外观”可分别编辑目标页签与 currentIndex、标签、图标、启用、位置、关闭策略、移动、滚动、省略和 RTL。固定 Selected／Focus 不改变原生选页或焦点；关闭只记录请求，不移除或删除页面。原生关闭按钮保持信号连接，单独接入 16px 源稿资源；画廊固定几何与真实页面自适应布局分离。

Dialog Actions 展示 32 个组合、80 个 canonical Button 子实例。画廊保留 Figma 示意顺序和 420×160 样本框；“外观”保留原生 QStyle 排序、standardButtons、方向、居中、RTL 和 ButtonRole 信号。Apply／Reset／Delete 不触发 accepted；演示 Delete 不执行操作，也不作为默认按钮。可打开无实际操作的对话框验证 Enter／Escape。透明容器只负责间距，子按钮复用已有 Button 外观配方；未据此提升设计审批或自动导出状态。

Tool Button 展示 Surface × Menu × 有效 State 共 34 个变体，Open 仅属于 Split。主命令、分割菜单、DelayedPopup 长按及 InstantPopup 保持 Qt 原生；固定 Open 不实际打开菜单。可编辑 defaultAction、显示模式、图标尺寸、autoRaise、勾选、优先级和 RTL。绑定 QAction 时，文本、启用和勾选由该动作统一管理。源稿图标槽实际为 18px，原始 SVG 为 16px；菜单箭头保留两段旋转矩形，未替换成通用图标。

Command Link 展示 Raised／Flat × 八状态共 16 个变体，保留原生点击、键盘、checkable／checked、default／autoDefault 和 description。固定 Pressed／Focus 不伪造原生状态；Show description／icon 仅控制绘制，不清空原文或图标。画廊保留源稿 52px 内容框对 62px 文本布局的裁切，真实预览按说明换行提供足够高度，此差异待设计确认；不内置互斥分组或页面跳转。

ComboBox 与 FontComboBox 各保留 Small／Middle／Large × 四状态共 12 个源稿变体。外观页可编辑选择、原生编辑模式、占位符、插入策略、RTL 和字体过滤／书写系统；Focus 预览不打开列表。字体画廊的 Arial 是固定设计文案，真实字体模型与缺失字体回退保持 Qt 原生。Prefix/Clear/任意 Suffix 仍是待确认的适配器提案，未冒充原生属性。

LineEdit 按 3 个 Size × 6 个 State 展示 18 个变体，保留原生文本编辑、IME、选区、撤销、校验、掩码、密码显示及 QAction。固定 Error 仅为视觉预览；不代表 validator 结果。原始图标保持 16px，动作槽位保持 Qt 命中规则；Qt 5 的清除按钮在只读时禁用而非隐藏。

Dial 展示 Wrapping × State 共 8 个源稿变体，保留 18 个原始 SVG 和刻度变换。画廊按 Figma 的圆弧和指针角度展示；外观页按 Qt 原生数值与鼠标角度联动，Off 使用原生 300° 底部缺口而非源稿 270° 左侧缺口，On 也保留原生起点。标签独立于数值，notchSize 只读、notchTarget 可编辑。Qt 的非零最小值反转映射偏差及零范围循环风险已明确记录；未改写原生鼠标事件或伪造拖动态，最终设计适配仍待确认。

Slider 和 ScrollBar 各有 8 个横向／纵向变体。画廊保留源稿几何，交互页的数值、位置、范围、步长、tracking、反转和 RTL 仍由 Qt 管理。Slider 的标签独立于数值，可编辑显隐、文本及刻度；ScrollBar 的 pageStep 决定运行时滑块比例，隐藏两端按钮会同时重算轨道与命中区。固定 Dragging 仅改变外观，不伪造拖动状态。关闭 tracking 后，释放或禁用会按 Qt 5.15 原生规则提交待定位置。ScrollBar 默认 NoFocus，可显式启用键盘聚焦；原生视口测试验证其尺寸未被画廊框架强制放大。

三个日期／时间页各展示 8 个变体。外观页可编辑值、有效范围、日历开关、只读、当前编辑分段、本地化；DateTimePicker 可隐藏时间及注释，TimePicker 可切换 12/24 小时制及秒显示。显示格式切换保留原值，不把隐藏时间归零；倒置范围会提示且不应用。`CST` 仅为源稿注释，不代表已实现时区转换。原生日历、编辑器和步进保留；捕获外观单独接管样式，避免全局 QSS 缩小日历命中区。

SpinBox 与 DoubleSpinBox 各展示 16 个变体：4 种 Controls × 4 种 State。“外观”可编辑范围、值、步长、前后缀、只读、小数位、本地化等真实属性；SplitPlusMinus 仅调整 Qt 原生步进子控件的位置，不新增 Qt 枚举或重建编辑器。非编辑态采用源稿数值／前后缀字级；实际编辑时保留原生光标、选择和输入法绘制，混合字级编辑及跨渲染器字体差异仍不作像素一致性声明。

Input 展示类型、尺寸、校验、受控值、清除、计数、加载与密码可见性。`QLineEdit`／`QPlainTextEdit` 不重建，保留输入法、选择、撤销和原生信号；视觉、字号、行高与状态直接来自 V2 `tokens_qt.json`。旧 `figma-light` 快照、私有 Forms QSS 与 warning 兼容分支已移除。

Radio 展示 10 个单选状态及 4 个横向／纵向组合，共 22 个原生 `Z_RadioButton`。外观页保留文本、选中、禁用、实时交互和固定状态比对；组合由 `QButtonGroup` 管理正整数选项 ID 与互斥，不将组合示例冒充已完成的公共 `Z_RadioGroup` 适配器。当前指示器按实际变量和图层的 18px 实现，旧说明中的 20px 未用于覆盖画布事实。

本轮视觉回归不再用通用占位矩阵替代设计资产：Select 的 12 个公开变体使用真实 `Z_Select`，Radio 修正 18px 指示器同心偏置，Switch 固定为 44×24／36×20 且不参与布局拉伸，SegmentedControl 保持内容尺寸；Transfer 的集合移动由 Demo controller 响应 SDK 请求，Tooltip 覆盖 12 个方向并支持多行。ScrollArea 使用 `Bars × Resize × State` 的 24 个 Token-backed `Z_ScrollArea`，Splitter 外观页提供真实拖动、折叠、恢复和方向切换。Sidebar 外观页使用公开 `Z_Sidebar`，不再默认展示内部 NavItem 空壳。Calendar 使用 `WeekStart × Grid × State` 的 8 个 440×360 `Z_CalendarWidget`，保留 Qt 原生翻月、日期选择和键盘能力，并通过主题语义调色板约束内部标题栏及禁用态。

ComponentLab 可以切换 Profile 或加载外部 Token 集；`figma-light` 则明确表示当前 Figma Light 画廊快照，不代表四套 Theme 均获设计验收。Checkbox 启用态半选按已确认产品决策保留紫底白色横杠，避免源稿白底白标记在运行时不可辨识。Progress 的 Completed 示例保持源稿 68%，与实际值分离；busy 无百分比，画廊为静态切片，运行时动画节奏未获设计确认。组件源码、外观适配和 Token 编译由 GenerateComponent 提供。

焦点环不是所有组件的强制外观。全局可选择 Off／Keyboard／Always；单个交互组件可调用 `FocusIndicator::instance().setEnabledFor(widget, false)` 关闭焦点环，同时保留键盘焦点与 Space／Enter 激活。纯展示组件使用 `widget->setFocusPolicy(Qt::NoFocus)`，从 Qt 原生焦点链移除。Qt 5 没有统一的 QWidget 属性可以在保留焦点的同时只隐藏所有样式焦点环，因此这两种关闭方式不得混用。

```powershell
pwsh -File Source/Verify-GeneratedComponentSDK.ps1
pwsh -File Source/delivery/tools/Build.ps1 -CMake '<cmake.exe>'
python -B Source/delivery/tools/CheckProject.py
```

开发项目通过 `#include <MyDesignSystem/MyDesignSystem.h>` 和 `GeneratedComponentSDK::Components` 使用组件。默认从同级 `DesignToCode_GenerateComponent/build/sdk` 读取；外部工作区可用 `GENERATED_COMPONENT_SDK_ROOT` 指向权威发布目录。双击 `build/demo/component_gallery/Run.cmd` 打开效果演示。

### SDK 构建契约

组件 SDK 是 MSVC 静态库，必须与消费端的 Qt、编译器和 Debug/Release 运行库完全匹配。Release 使用 `DesignToCode_GenerateComponent/build/sdk`；Debug 使用同目录下的 `build/sdk-debug`。未显式设置 `GENERATED_COMPONENT_SDK_ROOT` 时，CMake 在 Debug 配置且存在 `sdk-debug` 时自动选择它，Release 仍默认选择 `sdk`。切换 SDK 目录或构建类型请使用新的 `ComponentLab/build/cache` 构建目录；配置阶段会拒绝复用已绑定到另一 SDK 的 imported target，避免把旧 Release 库悄悄带入 Debug 链接。
