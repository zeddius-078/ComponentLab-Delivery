# 组件完成度与 Ant 对照

这张表用于安排 GenerateComponent 的增量开发。统计范围是项目已登记的 59 个具体可视 Qt 类型，不代表 Qt 全部控件，也不把 Ant Design 的 Web 实现视为 Qt 已有能力。

一阶段范围与组合策略以[组件基线与原生组合映射纲领](DesignMapping/一阶段组件基线与原生组合映射纲领.md)为准。本表仍按 Qt 类型记录实现现状，不是最终设计组件族清单；以下四项规则较成熟，不表示只完成四项即可满足一阶段。用户指出的 DropdownMenu 须与本表 Select 分别核对语义和实现入口。

当前只有四个组件的规则相对稳定，作为下一轮设计到开发映射的基线；这不等于 Pixso 身份、全部状态和视觉验收均已完成。

| 基线组件 | 开发类 | 当前进度 | 下一步 |
| --- | --- | --- | --- |
| Button | `Z_PushButton` | 专用实现、组件 Token 和 Pixso Button 试点 | 复核变体与运行状态的对应关系 |
| Checkbox | `Z_CheckBox` | 专用实现、组件 Token 和属性映射 | 接入真实 Pixso 组件身份并完成三态验收 |
| Input | `Z_InputField` | 语义组件、表单 Token 和属性映射 | 接入真实 Pixso 组件身份并验收输入状态 |
| Select | `Z_Select` | 语义组件、表单 Token 和属性映射 | 接入真实 Pixso 组件身份并验收选项与选择状态 |

59 个 Qt 可视类型中，Button 和 Checkbox 属于上述基线；其余 57 个是已发布的薄包装，统一按**待改造**管理。`Z_LineEdit`、`Z_ComboBox` 分别是 Qt 薄包装，不继承 `Z_InputField`、`Z_Select` 的规则成熟度。

数据依据：[组件定义](../Source/ComponentKit/component-definitions/native-components.json)、[Qt 能力与 Ant 参照登记](../Source/Qt/MappingWorkbench/mapping/qt-native-catalog.json)、[五项详细映射](../Source/Qt/MappingWorkbench/mapping/qt-component-map.json)、[SDK 消费测试](../Source/SDKConsumerTest/main.cpp)。Ant 名称参照[官方组件总览](https://ant.design/components/overview-cn/)；其中“直接参照”表示组件语义接近，“适配参照”表示需要组合或改造，均不表示 API、视觉和交互已经等价。

## 判定口径

| 列 | 当前标记 | 实际含义 |
| --- | --- | --- |
| 设计身份 | Button 试点 | 已保存并核对 Pixso Button 的节点、组件集、变体与输入哈希；见 [Pixso Button 输入](../Source/Qt/MappingWorkbench/mapping/design/pixso-button/) |
| 设计身份 | 待绑定 | 已登记 `ComponentLab/<Name>` 建议名称，尚无该组件可核对的 Pixso `componentKey` 到 `Z_*` 的验收证据 |
| 实现 | 专用实现 | 独立的项目 API、绘制或状态逻辑 |
| 实现 | Qt 继承 | 独立 `.h/.cpp` 和 `Z_*` 身份；主要行为来自 Qt 基类，尚未完成 Ant 语义适配 |
| Token | 组件合同 | 独立 Token 合同与 Profile 样式 |
| Token | 基础 Profile | 已生成四套 QSS，覆盖基础字体、前景、背景、边框及部分状态；未逐项验收组件专属 Token |
| 验证 | SDK 创建 | [消费测试](../Source/SDKConsumerTest/main.cpp)包含该类的工厂实例化；不等于状态与视觉验收 |
| 验证 | 交互 + SDK | 映射工作台测试含该专用类的交互路径；完整变体矩阵仍需继续核对 |

本地逐项检查结果：59 个定义均有 `.h/.cpp` 和四套 Profile QSS。能力登记中的 Ant 关联为 21 个直接参照、31 个适配参照、7 个 Qt 扩展。详细属性/API 映射目前覆盖 Button、IconButton、Checkbox、Input、Select；IconButton 沿用 Button 实现。

## 59 个组件

| Qt 原生类 | Z 组件 | Ant 语义参考 | 设计身份 | 实现 | Token | 验证 |
| --- | --- | --- | --- | --- | --- | --- |
| **基础** | | | | | | |
| `QWidget` | `Z_Widget` | Qt 扩展 | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QFrame` | `Z_Frame` | Qt 扩展 | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| **按钮与选择** | | | | | | |
| `QPushButton` | `Z_PushButton` | Button（直接参照） | Button 试点 | 专用实现 | 组件合同 | 交互 + SDK |
| `QToolButton` | `Z_ToolButton` | Button（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QRadioButton` | `Z_RadioButton` | Radio（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QCheckBox` | `Z_CheckBox` | Checkbox（直接参照） | 待绑定 | 专用实现 | 组件合同 | 交互 + SDK |
| `QCommandLinkButton` | `Z_CommandLinkButton` | Button（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QDialogButtonBox` | `Z_DialogButtonBox` | Space+Button（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| **数据录入** | | | | | | |
| `QLineEdit` | `Z_LineEdit` | Input（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | 属性 + SDK |
| `QSpinBox` | `Z_SpinBox` | InputNumber（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QDoubleSpinBox` | `Z_DoubleSpinBox` | InputNumber（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QSlider` | `Z_Slider` | Slider（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QScrollBar` | `Z_ScrollBar` | Qt 扩展 | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QDial` | `Z_Dial` | Slider（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QComboBox` | `Z_ComboBox` | Select（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QFontComboBox` | `Z_FontComboBox` | Select（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QDateTimeEdit` | `Z_DateTimeEdit` | DatePicker（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QDateEdit` | `Z_DateEdit` | DatePicker（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QTimeEdit` | `Z_TimeEdit` | TimePicker（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QKeySequenceEdit` | `Z_KeySequenceEdit` | Input（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| **信息展示** | | | | | | |
| `QLabel` | `Z_Label` | Typography（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QLCDNumber` | `Z_LCDNumber` | Statistic（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QProgressBar` | `Z_ProgressBar` | Progress（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QTextBrowser` | `Z_TextBrowser` | Typography（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QGraphicsView` | `Z_GraphicsView` | Qt 扩展 | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QCalendarWidget` | `Z_CalendarWidget` | Calendar（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QTextEdit` | `Z_TextEdit` | Input.TextArea（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QPlainTextEdit` | `Z_PlainTextEdit` | Input.TextArea（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| **数据视图** | | | | | | |
| `QListView` | `Z_ListView` | Listy（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QTreeView` | `Z_TreeView` | Tree（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QTableView` | `Z_TableView` | Table（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QColumnView` | `Z_ColumnView` | Cascader（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QUndoView` | `Z_UndoView` | Qt 扩展 | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QListWidget` | `Z_ListWidget` | Listy（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QTreeWidget` | `Z_TreeWidget` | Tree（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QTableWidget` | `Z_TableWidget` | Table（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| **容器** | | | | | | |
| `QGroupBox` | `Z_GroupBox` | Card（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QScrollArea` | `Z_ScrollArea` | Layout（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QTabWidget` | `Z_TabWidget` | Tabs（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QToolBox` | `Z_ToolBox` | Collapse（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QStackedWidget` | `Z_StackedWidget` | Tabs（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QSplitter` | `Z_Splitter` | Splitter（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QDockWidget` | `Z_DockWidget` | Drawer（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QMdiArea` | `Z_MdiArea` | Qt 扩展 | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QMdiSubWindow` | `Z_MdiSubWindow` | Card（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QWizard` | `Z_Wizard` | Steps+Form（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QWizardPage` | `Z_WizardPage` | Form（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| **窗口与反馈** | | | | | | |
| `QMenu` | `Z_Menu` | Menu（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QMenuBar` | `Z_MenuBar` | Menu（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QToolBar` | `Z_ToolBar` | Space+Button（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QStatusBar` | `Z_StatusBar` | App（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QOpenGLWidget` | `Z_OpenGLWidget` | Qt 扩展 | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QDialog` | `Z_Dialog` | Modal（直接参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QMessageBox` | `Z_MessageBox` | Modal（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QFileDialog` | `Z_FileDialog` | Upload（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QColorDialog` | `Z_ColorDialog` | ColorPicker（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QFontDialog` | `Z_FontDialog` | Select（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QInputDialog` | `Z_InputDialog` | Modal+Input（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |
| `QProgressDialog` | `Z_ProgressDialog` | Modal+Progress（适配参照） | 待绑定 | Qt 继承 | 基础 Profile | SDK 创建 |

## 容易混淆的两组入口

`Z_LineEdit` 和 `Z_ComboBox` 是上表中的 Qt 薄包装。`Z_InputField` 和 `Z_Select` 是另外发布的语义组件，具有更完整的字段结构与接口；`Z_OtpInput` 也是独立语义组件。这三者进入 SDK 工厂，但不计入 59 个 Qt 类型的薄包装统计。设计稿出现 Input、Select、OTP 时，须在详细映射中明确选择语义组件，不能把它们的能力记到原生薄包装名下。

## 补全顺序与完成条件

1. **固定四个基线组件的设计身份**：先完成 Button、Checkbox、Input、Select。为每个真实 Pixso 组件集记录文件标识、`componentSetId/componentKey`、变体值域和采集哈希；缺项保留“待绑定”。
2. **验证四个基线组件的规则可复用性**：在 [详细映射](../Source/Qt/MappingWorkbench/mapping/qt-component-map.json)中核对设计属性到 C++ API 的转换、默认值、非法组合和错误处理。四条链路通过后，再按 Radio、InputNumber、Slider、Date/Time 的顺序改造薄包装。
3. **补组件级 Token**：以 Button、Checkbox 的合同为范例，把各组件的尺寸、颜色、边框、焦点和禁用状态拆出项目 Token；重新生成 QSS 并检查四套 Profile。基础 Profile QSS 只算起点。
4. **验收组合组件**：Table、Tree、Tabs、Dialog 等逐一写出模型、子控件、操作状态与 Qt 容器的边界。以 Table 为例，排序、筛选、分页属于组合能力，不能由 `Z_TableView` 的继承关系推定完成；参照 [Ant Table](https://ant.design/components/table-cn/)。
5. **逐项升级表格状态**：只有设计身份可追溯、API 值域可校验、Token 可重建、交互测试通过并在 SDK 消费端运行时，才将对应行标记为“完成”。ComponentLab 负责展示，GenerateLayout 负责装配；组件实现仍由本项目维护。

每轮修改先更新事实源和测试，再同步本表。表中的“待绑定”和“Qt 继承”是明确的后续任务，不作完成率分子。

