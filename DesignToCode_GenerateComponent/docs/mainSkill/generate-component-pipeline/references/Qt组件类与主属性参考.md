
# Qt 组件类与主属性参考

本文按 Qt 5.15.2 的继承层级和控件分类整理公共属性。表格列出每个类新增或特有的主属性；继承属性在父类条目中说明。

## 使用方式

1. 先按控件分类找到 Qt 原生类。
2. 查看父类条目，合并继承属性。
3. 在 GenerateComponent 的映射合同中登记设计身份、`Z_*` 类、属性和值域。
4. 运行时实现以实际 `QMetaObject` 为准；文档字段需要在 Qt 5.15.2 环境中复核。

本文是属性参考表，组件是否已经实现、是否已经发布到 SDK，以组件 Registry、Contract、Recipe 和 SDK manifest 为准。

---

## 一、根基类

### QObject

| 属性名 | 类型 | 说明 |
|---|---|---|
| objectName | QString | 对象名称，用于查找和自动连接 |
| parent | QObject* | 父对象（构造时决定对象树） |

### QCoreApplication / QGuiApplication / QApplication

| 属性名 | 类型 | 说明 |
|---|---|---|
| applicationName | QString | 应用名称 |
| applicationVersion | QString | 应用版本 |
| organizationName | QString | 组织名称 |
| organizationDomain | QString | 组织域名 |
| quitOnLastWindowClosed | bool | 最后窗口关闭时是否退出 |

---

## 二、QWidget 体系

### QWidget

| 属性名 | 类型 | 说明 |
|---|---|---|
| enabled | bool | 控件是否可用 |
| geometry | QRect | 位置和尺寸 |
| sizePolicy | QSizePolicy | 布局中的缩放策略 |
| minimumSize / maximumSize | QSize | 最小/最大尺寸 |
| font | QFont | 字体 |
| cursor | QCursor | 鼠标光标 |
| toolTip | QString | 悬停提示 |
| statusTip | QString | 状态栏提示 |
| whatsThis | QString | 帮助信息 |
| styleSheet | QString | CSS 样式表 |
| focusPolicy | Qt::FocusPolicy | 焦点获取方式 |
| contextMenuPolicy | Qt::ContextMenuPolicy | 右键菜单策略 |
| acceptDrops | bool | 是否接受拖放 |
| autoFillBackground | bool | 是否自动填充背景 |
| layoutDirection | Qt::LayoutDirection | 布局方向 |
| locale | QLocale | 语言地区 |
| windowTitle | QString | 窗口标题 |
| windowIcon | QIcon | 窗口图标 |
| windowOpacity | qreal | 窗口透明度 |
| windowModality | Qt::WindowModality | 窗口模态 |
| windowFlags | Qt::WindowFlags | 窗口标志 |

### QFrame

| 属性名 | 类型 | 说明 |
|---|---|---|
| frameShape | QFrame::Shape | 边框形状 |
| frameShadow | QFrame::Shadow | 边框阴影 |
| lineWidth | int | 边框线宽 |
| midLineWidth | int | 中间线宽 |

**QFrame 子类**：QLabel、QLCDNumber、QAbstractScrollArea、QStackedWidget、QToolBox 等。

---

## 三、布局类

### QLayout

| 属性名 | 类型 | 说明 |
|---|---|---|
| margin | int | 布局外边距 |
| spacing | int | 控件间距 |
| sizeConstraint | QLayout::SizeConstraint | 尺寸约束策略 |

### QBoxLayout → QHBoxLayout / QVBoxLayout

| 属性名 | 类型 | 说明 |
|---|---|---|
| direction | QBoxLayout::Direction | 排列方向 |
| stretch | int | 伸缩因子（按索引） |

### QGridLayout

| 属性名 | 类型 | 说明 |
|---|---|---|
| horizontalSpacing | int | 水平间距 |
| verticalSpacing | int | 垂直间距 |
| rowStretch / columnStretch | int | 行/列伸缩因子 |
| rowMinimumHeight / columnMinimumWidth | int | 行/列最小尺寸 |

### QFormLayout

| 属性名 | 类型 | 说明 |
|---|---|---|
| fieldGrowthPolicy | FieldGrowthPolicy | 字段增长策略 |
| rowWrapPolicy | RowWrapPolicy | 行换行策略 |
| labelAlignment | Qt::Alignment | 标签对齐 |
| formAlignment | Qt::Alignment | 整体对齐 |
| horizontalSpacing / verticalSpacing | int | 间距 |

### QStackedLayout

| 属性名 | 类型 | 说明 |
|---|---|---|
| currentIndex | int | 当前显示页索引 |
| stackingMode | StackingMode | 堆叠模式 |

---

## 四、按钮类

### QAbstractButton

| 属性名 | 类型 | 说明 |
|---|---|---|
| text | QString | 按钮文字 |
| icon | QIcon | 按钮图标 |
| iconSize | QSize | 图标尺寸 |
| checkable | bool | 是否可勾选 |
| checked | bool | 是否已勾选 |
| autoExclusive | bool | 同组自动互斥 |
| autoRepeat | bool | 是否自动重复 |
| autoRepeatDelay | int | 重复延迟 |
| autoRepeatInterval | int | 重复间隔 |
| shortcut | QKeySequence | 快捷键 |

### QPushButton

| 属性名 | 类型 | 说明 |
|---|---|---|
| autoDefault | bool | 是否自动成为默认按钮 |
| default | bool | 是否为默认按钮 |
| flat | bool | 是否扁平显示 |
| menu | QMenu* | 关联菜单 |

### QToolButton

| 属性名 | 类型 | 说明 |
|---|---|---|
| toolButtonStyle | Qt::ToolButtonStyle | 图标/文字显示方式 |
| arrowType | Qt::ArrowType | 箭头类型 |
| popupMode | ToolButtonPopupMode | 弹出菜单模式 |
| autoRaise | bool | 是否自动凸起 |

Qt 5.15.2 的 `popupMode` 在启用 menu 功能时为本类可写 `Q_PROPERTY`，枚举值为 `DelayedPopup`、`MenuButtonPopup`、`InstantPopup`；`MenuButtonPopup` 的分区交互不能代表其他两种模式。`menu()`/`setMenu()` 和 `defaultAction()`/`setDefaultAction()` 是公开 API（后者 setter 为槽），不是同名 Q_PROPERTY；`showMenu()` 为槽、`triggered(QAction*)` 为本类信号。`text`、`icon`、`checkable`、`checked` 等属性及 `toggled(bool)` 信号继承自 `QAbstractButton`。本项目 `Z_ToolButton` 目前只继承原生类并设置身份元数据，没有新增公开菜单或 QAction 合同。设计变体中的 Checked、Open、Focus 是视觉表达；其中 Checked 需结合原生 `checkable/checked`，Open 及菜单箭头/分区命中仍需运行时核查。以上不代表生成器已映射这些 API。

### QRadioButton

无新增属性（继承 QAbstractButton 全部）。

### QCheckBox

| 属性名 | 类型 | 说明 |
|---|---|---|
| tristate | bool | 是否三态 |

### QCommandLinkButton

| 属性名 | 类型 | 说明 |
|---|---|---|
| description | QString | 说明文字 |

### QDialogButtonBox

| 属性名 | 类型 | 说明 |
|---|---|---|
| standardButtons | StandardButtons | 标准按钮集合 |
| orientation | Qt::Orientation | 排列方向 |
| centerButtons | bool | 是否居中 |

Qt 5.15.2 的 `QDialogButtonBox` 是容纳子按钮的 `QWidget`，不是一个可点击的 `QAbstractButton`。以上三项是本类可写 `Q_PROPERTY`；`StandardButton` 是标准按钮枚举及标志，`ButtonRole` 通过 `addButton(..., role)` 指派、由 `buttonRole(button)` 查询，不是名为 `buttonRole` 的 Q_PROPERTY。标准按钮可由 `standardButton(button)` 反查。其本类信号为 `clicked(QAbstractButton*)`、`accepted()`、`rejected()` 和 `helpRequested()`。本机 Qt 5.15.2 离屏探针显示：AcceptRole/YesRole 的点击附带 `accepted()`，RejectRole/NoRole 附带 `rejected()`，HelpRole 附带 `helpRequested()`；DestructiveRole、ActionRole、ResetRole、ApplyRole 仅触发 `clicked(button)`。该探针不证明平台排序、默认按钮或键盘行为。`ButtonLayout` 包括 Win、Mac、KDE、GNOME 等布局枚举，但没有把 Figma 固定顺序变成跨平台原生行为的可写属性；实际排列须随平台/样式验证。

---

## 五、输入类

### QLineEdit

| 属性名 | 类型 | 说明 |
|---|---|---|
| text | QString | 当前文本 |
| displayText | QString | 显示文本 |
| placeholderText | QString | 占位提示 |
| echoMode | EchoMode | 回显模式 |
| maxLength | int | 最大字符数 |
| inputMask | QString | 输入掩码 |
| validator | QValidator* | 验证器 |
| readOnly | bool | 是否只读 |
| frame | bool | 是否显示边框 |
| alignment | Qt::Alignment | 文本对齐 |
| cursorPosition | int | 光标位置 |
| hasSelectedText | bool | 是否有选中文本 |
| selectedText | QString | 选中文本 |
| dragEnabled | bool | 允许拖拽文本 |
| clearButtonEnabled | bool | 显示清除按钮 |
| completer | QCompleter* | 自动补全器 |
| textMargins | QMargins | 文本边距 |
| modified | bool | 内容是否修改过 |

### QAbstractSpinBox

| 属性名 | 类型 | 说明 |
|---|---|---|
| buttonSymbols | ButtonSymbols | 按钮符号 |
| specialValueText | QString | 特殊值文本 |
| accelerated | bool | 是否加速 |
| readOnly | bool | 是否只读 |
| keyboardTracking | bool | 实时跟踪键盘 |
| alignment | Qt::Alignment | 对齐方式 |
| frame | bool | 是否显示边框 |

### QSpinBox

| 属性名 | 类型 | 说明 |
|---|---|---|
| value | int | 当前值 |
| minimum / maximum | int | 最小/最大值 |
| singleStep | int | 步长 |
| prefix / suffix | QString | 前缀/后缀 |
| displayIntegerBase | int | 显示进制 |
| wrapping | bool | 是否循环 |

### QDoubleSpinBox

| 属性名 | 类型 | 说明 |
|---|---|---|
| value | double | 当前值 |
| minimum / maximum | double | 最小/最大值 |
| singleStep | double | 步长 |
| decimals | int | 小数位数 |
| prefix / suffix | QString | 前缀/后缀 |

### QAbstractSlider

| 属性名 | 类型 | 说明 |
|---|---|---|
| value | int | 当前值 |
| minimum / maximum | int | 最小/最大值 |
| singleStep | int | 单步 |
| pageStep | int | 翻页步长 |
| orientation | Qt::Orientation | 方向 |
| invertedAppearance | bool | 反向显示 |
| invertedControls | bool | 反向控制 |
| tracking | bool | 实时跟踪 |
| sliderPosition | int | 滑块位置 |
| sliderDown | bool | 滑块是否按下 |

### QSlider

| 属性名 | 类型 | 说明 |
|---|---|---|
| tickPosition | TickPosition | 刻度位置 |
| tickInterval | int | 刻度间隔 |

### QScrollBar

无新增主要属性。

### QDial

| 属性名 | 类型 | 说明 |
|---|---|---|
| wrapping | bool | 是否循环 |
| notchSize | int | 刻度尺寸 |
| notchTarget | double | 刻度目标 |
| notchesVisible | bool | 显示刻度 |

### QComboBox

| 属性名 | 类型 | 说明 |
|---|---|---|
| currentText | QString | 当前文本 |
| currentIndex | int | 当前索引 |
| placeholderText | QString | Qt 5.15 起的原生占位文本；无有效选中项时显示。可编辑模式使用内部 QLineEdit 的占位文本 |
| editable | bool | 是否可编辑 |
| insertPolicy | InsertPolicy | 插入策略 |
| sizeAdjustPolicy | SizeAdjustPolicy | 尺寸调整策略 |
| minimumContentsLength | int | 最小内容长度 |
| iconSize | QSize | 图标尺寸 |
| maxVisibleItems | int | 最大可见项数 |
| maxCount | int | 最大项数 |
| duplicatesEnabled | bool | 允许重复 |
| frame | bool | 显示边框 |
| modelColumn | int | 模型列 |

### QFontComboBox

| 属性名 | 类型 | 说明 |
|---|---|---|
| writingSystem | WritingSystem | 书写系统 |
| currentFont | QFont | 当前字体 |

### QDateTimeEdit

| 属性名 | 类型 | 说明 |
|---|---|---|
| dateTime | QDateTime | 日期时间 |
| date | QDate | 日期 |
| time | QTime | 时间 |
| minimumDateTime / maximumDateTime | QDateTime | 日期时间范围；原生可写 Q_PROPERTY |
| minimumDate / maximumDate | QDate | 日期范围 |
| minimumTime / maximumTime | QTime | 时间范围 |
| displayFormat | QString | 显示格式 |
| calendarPopup | bool | 弹出日历 |
| currentSection / currentSectionIndex | Section / int | 当前编辑分段；原生可写 Q_PROPERTY |
| displayedSections / sectionCount | Sections / int | 当前显示分段及数量；原生只读 Q_PROPERTY |
| timeSpec | Qt::TimeSpec | 时间规格 |

`QDateTimeEdit` 的 `dateTimeChanged`、`dateChanged`、`timeChanged` 是原生信号。`setDateTimeRange`、`setDateRange`、`setTimeRange`、`setCalendarWidget` 和 `setSelectedSection` 是公开配置方法，不是同名 Q_PROPERTY；`readOnly`、`keyboardTracking`、`wrapping` 则继承自 `QAbstractSpinBox`。设计稿中的 Focus、Error 等视觉/扩展状态不能据此登记为 Qt 原生可写属性。以上按本机 Qt 5.15.2 `QtWidgets/qdatetimeedit.h`、`qabstractspinbox.h` 及元对象核对；不代表项目生成器已经映射这些接口。

### QDateEdit / QTimeEdit

继承 `QDateTimeEdit` 的主要属性；`QDateEdit` 重声明 `date`，以 `userDateChanged(QDate)` 作为用户属性通知信号；`QTimeEdit` 重声明 `time`，以 `userTimeChanged(QTime)` 作为用户属性通知信号。两者仍分别保有 `QDateTimeEdit` 的继承 API，不应合并为同一个设计或 SDK 类身份。

### QKeySequenceEdit

| 属性名 | 类型 | 说明 |
|---|---|---|
| keySequence | QKeySequence | 快捷键序列 |

Qt 5.15.2 的 `QKeySequenceEdit` 仅将 `keySequence` 暴露为本类可写 `Q_PROPERTY`；`setKeySequence()` 与 `clear()` 为公开槽，`keySequenceChanged` 与 `editingFinished` 为信号。`maximumSequenceLength` 和 `finishingKeyCombinations` 自 Qt 6.5 起提供，不得作为本项目 Qt 5.15.2 原生属性或生成器 setter。快捷键冲突检测与分配属于上层工程合同，不由 Figma 静态状态证明。

---

## 六、显示类

### QLabel

| 属性名 | 类型 | 说明 |
|---|---|---|
| text | QString | 显示文本 |
| pixmap | QPixmap | 显示图片 |
| scaledContents | bool | 缩放内容 |
| alignment | Qt::Alignment | 对齐方式 |
| wordWrap | bool | 自动换行 |
| margin | int | 边距 |
| indent | int | 缩进 |
| openExternalLinks | bool | 打开外部链接 |
| textFormat | Qt::TextFormat | 文本格式 |
| textInteractionFlags | Qt::TextInteractionFlags | 文本交互标志 |
| buddy | QWidget* | 伙伴控件 |
| hasSelectedText / selectedText | bool/QString | 选中状态 |

### QLCDNumber

| 属性名 | 类型 | 说明 |
|---|---|---|
| value | double | 当前值 |
| digitCount | int | 位数 |
| mode | Mode | 显示模式 |
| segmentStyle | SegmentStyle | 段样式 |
| smallDecimalPoint | bool | 小数点样式 |
| intValue / doubleValue | int/double | 读取值 |
| overflow | bool | 是否溢出 |

### QProgressBar

| 属性名 | 类型 | 说明 |
|---|---|---|
| value | int | 当前值 |
| minimum / maximum | int | 范围 |
| orientation | Qt::Orientation | 方向 |
| textVisible | bool | 显示文字 |
| textDirection | Direction | 文字方向 |
| format | QString | 显示格式 |
| invertedAppearance | bool | 反向显示 |
| alignment | Qt::Alignment | 对齐 |

### QTextBrowser

| 属性名 | 类型 | 说明 |
|---|---|---|
| source | QUrl | 当前文档来源 |
| searchPaths | QStringList | 搜索路径 |
| openExternalLinks | bool | 打开外部链接 |
| openLinks | bool | 允许打开链接 |

### QGraphicsView

| 属性名 | 类型 | 说明 |
|---|---|---|
| scene | QGraphicsScene* | 关联场景 |
| backgroundBrush | QBrush | 背景画刷 |
| foregroundBrush | QBrush | 前景画刷 |
| interactive | bool | 可交互 |
| dragMode | DragMode | 拖拽模式 |
| cacheMode | CacheModeFlag | 缓存模式 |
| alignment | Qt::Alignment | 对齐 |
| transformationAnchor | ViewportAnchor | 变换锚点 |
| resizeAnchor | ViewportAnchor | 缩放锚点 |
| renderHints | QPainter::RenderHints | 渲染提示 |
| viewportUpdateMode | ViewportUpdateMode | 视口更新模式 |
| optimizationFlags | OptimizationFlags | 优化标志 |

### QCalendarWidget

| 属性名 | 类型 | 说明 |
|---|---|---|
| selectedDate | QDate | 选中日期 |
| minimumDate / maximumDate | QDate | 日期范围 |
| firstDayOfWeek | Qt::DayOfWeek | 一周第一天 |
| gridVisible | bool | 显示网格 |
| horizontalHeaderFormat | HeaderFormat | 水平表头格式 |
| verticalHeaderFormat | HeaderFormat | 垂直表头格式 |
| navigationBarVisible | bool | 显示导航栏 |
| dateEditEnabled | bool | 启用日期编辑 |
| dateEditAcceptDelay | int | 接受延迟 |

---

## 七、文本编辑类

### QTextEdit

| 属性名 | 类型 | 说明 |
|---|---|---|
| html | QString | HTML 内容 |
| plainText | QString | 纯文本内容 |
| markdown | QString | Markdown 内容 |
| readOnly | bool | 只读 |
| undoRedoEnabled | bool | 撤销重做 |
| tabChangesFocus | bool | Tab 切换焦点 |
| lineWrapMode | LineWrapMode | 换行模式 |
| lineWrapColumnOrWidth | int | 换行列/宽度 |
| wordWrapMode | QTextOption::WrapMode | 单词换行 |
| overwriteMode | bool | 覆盖模式 |
| acceptRichText | bool | 接受富文本 |
| autoFormatting | AutoFormatting | 自动格式化 |
| cursorWidth | int | 光标宽度 |
| document | QTextDocument* | 文档对象 |
| placeholderText | QString | 占位文字 |

### QPlainTextEdit

| 属性名 | 类型 | 说明 |
|---|---|---|
| plainText | QString | 纯文本内容 |
| readOnly | bool | 只读 |
| undoRedoEnabled | bool | 撤销重做 |
| lineWrapMode | LineWrapMode | 换行模式 |
| wordWrapMode | QTextOption::WrapMode | 单词换行 |
| tabChangesFocus | bool | Tab 切换焦点 |
| tabStopDistance | qreal | Tab 停靠距离 |
| overwriteMode | bool | 覆盖模式 |
| cursorWidth | int | 光标宽度 |
| maximumBlockCount | int | 最大块数 |
| backgroundVisible | bool | 背景可见 |
| centerOnScroll | bool | 滚动居中 |
| placeholderText | QString | 占位文字 |

---

## 八、视图类

### QAbstractItemView

| 属性名 | 类型 | 说明 |
|---|---|---|
| model | QAbstractItemModel* | 数据模型 |
| selectionModel | QItemSelectionModel* | 选择模型 |
| selectionMode | SelectionMode | 选择模式 |
| selectionBehavior | SelectionBehavior | 选择行为 |
| currentIndex | QModelIndex | 当前索引 |
| editTriggers | EditTriggers | 编辑触发方式 |
| alternatingRowColors | bool | 交替行颜色 |
| dragEnabled | bool | 允许拖拽 |
| dragDropMode | DragDropMode | 拖放模式 |
| defaultDropAction | Qt::DropAction | 默认放下动作 |
| dropIndicatorShown | bool | 显示放下指示器 |
| autoScroll | bool | 自动滚动 |
| autoScrollMargin | int | 自动滚动边距 |
| tabKeyNavigation | bool | Tab 键导航 |
| showDropIndicator | bool | 显示拖放指示 |
| textElideMode | Qt::TextElideMode | 文本省略模式 |
| verticalScrollMode | ScrollMode | 垂直滚动模式 |
| horizontalScrollMode | ScrollMode | 水平滚动模式 |

### QListView

| 属性名 | 类型 | 说明 |
|---|---|---|
| flow | Flow | 排列方向 |
| isWrapping | bool | 是否换行 |
| resizeMode | ResizeMode | 调整模式 |
| layoutMode | LayoutMode | 布局模式 |
| spacing | int | 间距 |
| gridSize | QSize | 网格尺寸 |
| viewMode | ViewMode | 视图模式 |
| modelColumn | int | 模型列 |
| uniformItemSizes | bool | 统一项尺寸 |
| batchSize | int | 批处理大小 |
| wordWrap | bool | 单词换行 |
| movement | Movement | 移动方式 |

### QTreeView

| 属性名 | 类型 | 说明 |
|---|---|---|
| header | QHeaderView* | 表头 |
| headerHidden | bool | 隐藏表头 |
| columnViewportPosition | int | 列视口位置 |
| columnWidth | int | 列宽 |
| columnHidden | bool | 列隐藏 |
| indentation | int | 缩进 |
| rootIsDecorated | bool | 根节点装饰 |
| uniformRowHeights | bool | 统一行高 |
| itemsExpandable | bool | 项可展开 |
| animated | bool | 动画效果 |
| sortingEnabled | bool | 启用排序 |
| allColumnsShowFocus | bool | 所有列显示焦点 |
| wordWrap | bool | 单词换行 |
| expandsOnDoubleClick | bool | 双击展开 |

### QTableView

| 属性名 | 类型 | 说明 |
|---|---|---|
| model | QAbstractItemModel* | 数据模型 |
| showGrid | bool | 显示网格 |
| gridStyle | Qt::PenStyle | 网格样式 |
| sortingEnabled | bool | 启用排序 |
| wordWrap | bool | 单词换行 |
| cornerButtonEnabled | bool | 角按钮 |
| horizontalHeader | QHeaderView* | 水平表头 |
| verticalHeader | QHeaderView* | 垂直表头 |
| rowHeight | int | 行高 |
| columnWidth | int | 列宽 |
| rowHidden | bool | 行隐藏 |
| columnHidden | bool | 列隐藏 |

### QColumnView

| 属性名 | 类型 | 说明 |
|---|---|---|
| resizeGripsVisible | bool | 调整手柄可见 |
| previewWidget | QWidget* | 预览控件 |

### QUndoView

| 属性名 | 类型 | 说明 |
|---|---|---|
| emptyLabel | QString | 空标签 |
| cleanIcon | QIcon | 干净图标 |
| stack | QUndoStack* | 撤销栈 |

---

## 九、项组件类

### QListWidget

| 属性名 | 类型 | 说明 |
|---|---|---|
| count | int | 项数量 |
| currentRow | int | 当前行 |
| currentItem | QListWidgetItem* | 当前项 |
| sortingEnabled | bool | 启用排序 |

### QTreeWidget

| 属性名 | 类型 | 说明 |
|---|---|---|
| columnCount | int | 列数 |
| topLevelItemCount | int | 顶层项数 |
| currentItem | QTreeWidgetItem* | 当前项 |
| currentColumn | int | 当前列 |

### QTableWidget

| 属性名 | 类型 | 说明 |
|---|---|---|
| rowCount | int | 行数 |
| columnCount | int | 列数 |
| currentRow | int | 当前行 |
| currentColumn | int | 当前列 |
| currentItem | QTableWidgetItem* | 当前项 |

---

## 十、容器类

### QGroupBox

| 属性名 | 类型 | 说明 |
|---|---|---|
| title | QString | 分组标题 |
| alignment | Qt::Alignment | 标题对齐 |
| flat | bool | 是否扁平 |
| checkable | bool | 可勾选 |
| checked | bool | 已勾选 |

### QScrollArea

| 属性名 | 类型 | 说明 |
|---|---|---|
| widget | QWidget* | 内部控件 |
| widgetResizable | bool | 内部控件可调整 |
| horizontalScrollBarPolicy | Qt::ScrollBarPolicy | 水平滚动条策略 |
| verticalScrollBarPolicy | Qt::ScrollBarPolicy | 垂直滚动条策略 |
| alignment | Qt::Alignment | 对齐 |

### QTabWidget

| 属性名 | 类型 | 说明 |
|---|---|---|
| currentIndex | int | 当前页索引 |
| currentTabText | QString | 当前页文本 |
| tabPosition | TabPosition | 标签位置 |
| tabShape | TabShape | 标签形状 |
| tabBarAutoHide | bool | 标签栏自动隐藏 |
| movable | bool | 标签可移动 |
| tabsClosable | bool | 标签可关闭 |
| documentMode | bool | 文档模式 |
| iconSize | QSize | 图标尺寸 |
| usesScrollButtons | bool | 使用滚动按钮 |
| elideMode | Qt::TextElideMode | 省略模式 |

### QToolBox

| 属性名 | 类型 | 说明 |
|---|---|---|
| currentIndex | int | 当前页索引 |
| count | int | 页数 |

### QStackedWidget

| 属性名 | 类型 | 说明 |
|---|---|---|
| currentIndex | int | 当前页索引 |
| count | int | 页数 |

### QSplitter

| 属性名 | 类型 | 说明 |
|---|---|---|
| orientation | Qt::Orientation | 方向 |
| childrenCollapsible | bool | 子控件可折叠 |
| opaqueResize | bool | 不透明调整 |
| handleWidth | int | 手柄宽度 |

### QDockWidget

| 属性名 | 类型 | 说明 |
|---|---|---|
| windowTitle | QString | 标题 |
| dockWidgetArea | Qt::DockWidgetArea | 停靠区域 |
| features | DockWidgetFeatures | 功能特性 |
| allowedAreas | Qt::DockWidgetAreas | 允许区域 |
| floating | bool | 是否浮动 |

### QMdiArea

| 属性名 | 类型 | 说明 |
|---|---|---|
| background | QBrush | 背景 |
| activationOrder | WindowOrder | 激活顺序 |
| viewMode | ViewMode | 视图模式 |
| documentMode | bool | 文档模式 |
| tabsClosable | bool | 标签可关闭 |
| tabsMovable | bool | 标签可移动 |
| tabPosition | TabPosition | 标签位置 |
| tabShape | TabShape | 标签形状 |

### QMdiSubWindow

| 属性名 | 类型 | 说明 |
|---|---|---|
| keyboardSingleStep | int | 键盘单步 |
| keyboardPageStep | int | 键盘翻页 |
| rubberBandResize | bool | 橡皮筋调整 |

### QWizard

| 属性名 | 类型 | 说明 |
|---|---|---|
| currentId | int | 当前页 ID |
| wizardStyle | WizardStyle | 向导样式 |
| titleFormat | Qt::TextFormat | 标题格式 |
| options | WizardOptions | 选项 |

### QWizardPage

| 属性名 | 类型 | 说明 |
|---|---|---|
| title | QString | 标题 |
| subTitle | QString | 副标题 |
| finalPage | bool | 是否最终页 |
| commitPage | bool | 是否提交页 |

---

## 十一、菜单 / 工具栏 / 动作类

### QAction

| 属性名 | 类型 | 说明 |
|---|---|---|
| text | QString | 动作文字 |
| icon | QIcon | 图标 |
| shortcut | QKeySequence | 快捷键 |
| checkable | bool | 可勾选 |
| checked | bool | 已勾选 |
| enabled | bool | 是否可用 |
| visible | bool | 是否可见 |
| toolTip | QString | 提示 |
| statusTip | QString | 状态栏提示 |
| whatsThis | QString | 帮助 |
| menu | QMenu* | 子菜单 |
| menuRole | MenuRole | 菜单角色 |
| iconText | QString | 图标文本 |
| autoRepeat | bool | 自动重复 |

### QMenu

| 属性名 | 类型 | 说明 |
|---|---|---|
| title | QString | 菜单标题 |
| icon | QIcon | 图标 |
| tearOffEnabled | bool | 可撕下 |
| separatorsCollapsible | bool | 分隔符可折叠 |
| toolTipsVisible | bool | 提示可见 |

### QMenuBar

| 属性名 | 类型 | 说明 |
|---|---|---|
| defaultUp | bool | 默认向上 |
| nativeMenuBar | bool | 原生菜单栏 |

### QToolBar

| 属性名 | 类型 | 说明 |
|---|---|---|
| movable | bool | 可移动 |
| allowedAreas | Qt::ToolBarAreas | 允许区域 |
| orientation | Qt::Orientation | 方向 |
| iconSize | QSize | 图标尺寸 |
| toolButtonStyle | Qt::ToolButtonStyle | 按钮样式 |
| floating | bool | 是否浮动 |
| floatable | bool | 可浮动 |

### QStatusBar

| 属性名 | 类型 | 说明 |
|---|---|---|
| sizeGripEnabled | bool | 启用尺寸手柄 |
| message | QString | 当前消息 |

---

## 十二、模型类

### QAbstractItemModel

无 UI 属性，提供数据接口。

### QStandardItemModel

| 属性名 | 类型 | 说明 |
|---|---|---|
| rowCount | int | 行数 |
| columnCount | int | 列数 |
| sortRole | int | 排序角色 |

### QStringListModel

| 属性名 | 类型 | 说明 |
|---|---|---|
| stringList | QStringList | 字符串列表 |

### QFileSystemModel

| 属性名 | 类型 | 说明 |
|---|---|---|
| rootPath | QString | 根路径 |
| rootIndex | QModelIndex | 根索引 |
| readOnly | bool | 只读 |
| nameFilterDisables | bool | 名称过滤禁用 |
| nameFilters | QStringList | 名称过滤器 |

### QSortFilterProxyModel

| 属性名 | 类型 | 说明 |
|---|---|---|
| sourceModel | QAbstractItemModel* | 源模型 |
| filterRegExp | QRegExp | 过滤正则 |
| filterKeyColumn | int | 过滤键列 |
| sortCaseSensitivity | Qt::CaseSensitivity | 排序大小写敏感 |
| sortRole | int | 排序角色 |
| dynamicSortFilter | bool | 动态排序过滤 |
| filterCaseSensitivity | Qt::CaseSensitivity | 过滤大小写敏感 |
| recursiveFilteringEnabled | bool | 递归过滤 |

---

## 十三、图形 / 绘图类

### QGraphicsScene

| 属性名 | 类型 | 说明 |
|---|---|---|
| sceneRect | QRectF | 场景矩形 |
| itemIndexMethod | ItemIndexMethod | 项索引方法 |
| bspTreeDepth | int | BSP 树深度 |
| palette | QPalette | 调色板 |
| font | QFont | 字体 |
| backgroundBrush | QBrush | 背景画刷 |
| foregroundBrush | QBrush | 前景画刷 |
| stickyFocus | bool | 粘性焦点 |
| minimumRenderSize | qreal | 最小渲染尺寸 |

### QOpenGLWidget

| 属性名 | 类型 | 说明 |
|---|---|---|
| format | QSurfaceFormat | 表面格式 |
| updateBehavior | UpdateBehavior | 更新行为 |

---

## 十四、对话框类

### QDialog

| 属性名 | 类型 | 说明 |
|---|---|---|
| modal | bool | 是否模态 |
| sizeGripEnabled | bool | 尺寸手柄 |
| result | int | 结果码 |

### QMessageBox

| 属性名 | 类型 | 说明 |
|---|---|---|
| text | QString | 主文本 |
| informativeText | QString | 信息文本 |
| detailedText | QString | 详细文本 |
| icon | Icon | 图标类型 |
| standardButtons | StandardButtons | 标准按钮 |
| defaultButton | QPushButton* | 默认按钮 |
| escapeButton | QPushButton* | 逃逸按钮 |

### QFileDialog

| 属性名 | 类型 | 说明 |
|---|---|---|
| directory | QString | 目录 |
| selectedFiles | QStringList | 选中文件 |
| filter | QString | 过滤器 |
| viewMode | ViewMode | 视图模式 |
| fileMode | FileMode | 文件模式 |
| acceptMode | AcceptMode | 接受模式 |
| defaultSuffix | QString | 默认后缀 |
| nameFilters | QStringList | 名称过滤器 |
| options | Options | 选项 |

### QColorDialog

| 属性名 | 类型 | 说明 |
|---|---|---|
| currentColor | QColor | 当前颜色 |
| options | ColorDialogOptions | 选项 |

### QFontDialog

| 属性名 | 类型 | 说明 |
|---|---|---|
| currentFont | QFont | 当前字体 |
| options | FontDialogOptions | 选项 |

### QInputDialog

| 属性名 | 类型 | 说明 |
|---|---|---|
| labelText | QString | 标签文本 |
| inputMode | InputMode | 输入模式 |
| okButtonText | QString | 确定按钮文本 |
| cancelButtonText | QString | 取消按钮文本 |
| comboBoxItems | QStringList | 下拉项 |
| editable | bool | 可编辑 |
| textValue | QString | 文本值 |
| intValue | int | 整数值 |
| doubleValue | double | 浮点值 |

### QProgressDialog

| 属性名 | 类型 | 说明 |
|---|---|---|
| labelText | QString | 标签文本 |
| cancelButtonText | QString | 取消按钮文本 |
| minimum / maximum | int | 范围 |
| value | int | 当前值 |
| autoClose | bool | 自动关闭 |
| autoReset | bool | 自动重置 |
| minimumDuration | int | 最小持续时间 |
| wasCanceled | bool | 是否取消 |

---

## 十五、其他常用类

### QSizePolicy

| 属性名 | 类型 | 说明 |
|---|---|---|
| horizontalPolicy | Policy | 水平策略 |
| verticalPolicy | Policy | 垂直策略 |
| horizontalStretch | int | 水平伸缩 |
| verticalStretch | int | 垂直伸缩 |
| heightForWidth | bool | 高度依赖宽度 |
| hasHeightForWidth | bool | 是否依赖 |

### QPalette

包含 Active、Inactive、Disabled 三组颜色角色（Window、WindowText、Base、Text、Button、ButtonText、Highlight、HighlightedText 等）。

### QFont

| 属性名 | 类型 | 说明 |
|---|---|---|
| family | QString | 字体家族 |
| pointSize | int | 点大小 |
| pixelSize | int | 像素大小 |
| weight | int | 粗细 |
| bold | bool | 粗体 |
| italic | bool | 斜体 |
| underline | bool | 下划线 |
| strikeOut | bool | 删除线 |
| kerning | bool | 字距调整 |
| styleHint | StyleHint | 风格提示 |

### QCursor

| 属性名 | 类型 | 说明 |
|---|---|---|
| shape | Qt::CursorShape | 光标形状 |
| pixmap | QPixmap | 光标图片 |
| hotSpot | QPoint | 热点 |

### QKeySequence

| 属性名 | 类型 | 说明 |
|---|---|---|
| keySequence | QKeySequence | 快捷键序列 |
| count | int | 序列数量 |

---

## 继承关系速览

```
QObject
 ├── QCoreApplication → QGuiApplication → QApplication
 ├── QLayout → QBoxLayout → QHBoxLayout / QVBoxLayout
 │            ├── QGridLayout
 │            ├── QFormLayout
 │            └── QStackedLayout
 ├── QAction
 ├── QAbstractItemModel → QStandardItemModel / QStringListModel / QFileSystemModel
 │                      └── QSortFilterProxyModel
 ├── QGraphicsScene
 └── QWidget
      ├── QLabel / QLCDNumber / QProgressBar / QCalendarWidget
      ├── QAbstractButton → QPushButton / QToolButton / QRadioButton / QCheckBox / QCommandLinkButton
      ├── QLineEdit
      ├── QAbstractSpinBox → QSpinBox / QDoubleSpinBox / QDateTimeEdit → QDateEdit / QTimeEdit
      ├── QAbstractSlider → QSlider / QScrollBar / QDial
      ├── QComboBox → QFontComboBox
      ├── QKeySequenceEdit
      ├── QTextEdit → QTextBrowser
      ├── QPlainTextEdit
      ├── QAbstractScrollArea → QAbstractItemView → QListView / QTreeView / QTableView / QColumnView
      ├── QListWidget / QTreeWidget / QTableWidget
      ├── QUndoView
      ├── QGroupBox / QFrame / QSplitter / QTabWidget / QToolBox / QStackedWidget
      ├── QScrollArea / QMdiArea / QDockWidget
      ├── QDialog → QMessageBox / QFileDialog / QColorDialog / QFontDialog / QInputDialog / QProgressDialog
      ├── QMainWindow（含 QMenuBar / QToolBar / QStatusBar / QDockWidget）
      └── QGraphicsView / QOpenGLWidget
```
