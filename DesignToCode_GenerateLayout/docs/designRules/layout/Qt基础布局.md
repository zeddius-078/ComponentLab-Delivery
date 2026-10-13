基于 Qt 6 官方中文文档 [Layout Management](https://doc.qt.io/qt-6/zh/layout.html)，以下整理了 Qt 布局管理器的详细列表，包含类继承关系、核心属性、关键方法及枚举说明。

---

## 一、布局类总览

Qt 布局类均继承自 `QLayout`（而 `QLayout` 继承自 `QObject` 与 `QLayoutItem`），负责管理一组 widget 的几何图形。

| 类名 | 继承自 | 用途说明 |
|---|---|---|
| `QLayout` | `QObject` / `QLayoutItem` | 所有布局管理器的抽象基类，定义几何管理器的通用接口 |
| `QBoxLayout` | `QLayout` | 将子部件按水平或垂直方向排列的基类 |
| `QHBoxLayout` | `QBoxLayout` | 按从左到右（或从右到左的语言）排列部件 |
| `QVBoxLayout` | `QBoxLayout` | 按垂直列从上到下排列部件 |
| `QGridLayout` | `QLayout` | 将部件排列在二维网格中，部件可占据多个单元格 |
| `QFormLayout` | `QLayout` | 以两栏（标签–字段）形式管理输入部件及其关联标签 |
| `QStackedLayout` | `QLayout` | 提供一组堆叠的部件，同一时间仅一个可见 |
| `QGraphicsAnchorLayout` | `QLayout` | 可在图形视图中将部件锚定在一起的布局 |


## 二、QLayout 基类 — 通用属性与接口

`QLayout` 是所有布局类的基类，以下属性与函数适用于所有布局管理器。

### 属性

| 属性名 | 类型 | 说明 |
|---|---|---|
| `contentsMargins` | `QMargins` | 布局内容区域的外边距（左、上、右、下） |
| `sizeConstraint` | `SizeConstraint` | 布局对包含它的窗口尺寸的约束方式 |
| `horizontalSizeConstraint` | `SizeConstraint` | 水平方向的尺寸约束（Qt 6.10 引入） |
| `verticalSizeConstraint` | `SizeConstraint` | 垂直方向的尺寸约束（Qt 6.10 引入） |
| `spacing` | `int` | 布局中部件之间的默认间距 |

### SizeConstraint 枚举

| 枚举值 | 说明 |
|---|---|
| `SetDefaultConstraint` | 使用窗口的默认约束（默认值） |
| `SetFixedSize` | 窗口尺寸固定为 `sizeHint()` |
| `SetMinimumSize` | 窗口最小尺寸为 `sizeHint()`，但可更大 |
| `SetMaximumSize` | 窗口最大尺寸为 `sizeHint()` |
| `SetMinAndMaxSize` | 窗口尺寸固定为 `sizeHint()`（最小和最大相同） |
| `SetNoConstraint` | 不施加尺寸约束 |

### 常用公共函数

| 函数 | 说明 |
|---|---|
| `addWidget(QWidget *w)` | 向布局中添加一个部件 |
| `removeWidget(QWidget *widget)` | 从布局中移除部件（部件本身不会被删除） |
| `addItem(QLayoutItem *item)` | 添加一个布局项（纯虚函数，由子类实现） |
| `count()` | 返回布局中的项数（纯虚函数） |
| `itemAt(int index)` | 返回指定索引处的布局项（纯虚函数） |
| `indexOf(const QWidget *widget)` | 返回指定部件在布局中的索引 |
| `setAlignment(QWidget *w, Qt::Alignment)` | 设置部件在布局中的对齐方式 |
| `parentWidget()` | 返回安装此布局的父部件 |
| `setEnabled(bool)` | 启用或禁用布局 |
| `activate()` | 重新计算布局并激活 |
| `contentsRect()` | 返回布局的内容矩形区域 |


## 三、QBoxLayout（QHBoxLayout / QVBoxLayout）

`QBoxLayout` 将子部件沿水平或垂直方向排成一行，`QHBoxLayout` 和 `QVBoxLayout` 是其便捷子类，通常直接使用后者。

### Direction 枚举

| 枚举值 | 说明 |
|---|---|
| `LeftToRight` | 从左到右排列（水平） |
| `RightToLeft` | 从右到左排列（水平） |
| `TopToBottom` | 从上到下排列（垂直） |
| `BottomToTop` | 从下到上排列（垂直） |

### 关键方法

| 方法 | 说明 |
|---|---|
| `addWidget(QWidget *widget, int stretch=0, Qt::Alignment alignment=Qt::Alignment())` | 添加部件，可指定拉伸因子与对齐方式 |
| `addLayout(QLayout *layout, int stretch=0)` | 嵌套添加子布局 |
| `addStretch(int stretch=0)` | 在末尾添加可伸缩的空白空间 |
| `addSpacing(int size)` | 添加固定大小的空白间距 |
| `addSpacerItem(QSpacerItem *spacerItem)` | 添加自定义的间隔项 |
| `insertWidget(int index, QWidget *widget, int stretch=0, Qt::Alignment alignment=Qt::Alignment())` | 在指定位置插入部件 |
| `setStretch(int index, int stretch)` | 设置指定索引处项的拉伸因子 |
| `setStretchFactor(QWidget *widget, int stretch)` | 设置指定部件的拉伸因子 |
| `setDirection(QBoxLayout::Direction direction)` | 动态改变排列方向 |
| `direction()` | 返回当前排列方向 |


## 四、QGridLayout

`QGridLayout` 将部件排列在二维网格中，每个部件占据一个或多个单元格。

### 属性

| 属性名 | 类型 | 说明 |
|---|---|---|
| `horizontalSpacing` | `int` | 水平方向部件间距 |
| `verticalSpacing` | `int` | 垂直方向部件间距 |

### 关键方法

| 方法 | 说明 |
|---|---|
| `addWidget(QWidget *widget, int row, int column, Qt::Alignment alignment=Qt::Alignment())` | 将部件添加到指定行列 |
| `addWidget(QWidget *widget, int fromRow, int fromColumn, int rowSpan, int columnSpan, Qt::Alignment alignment=Qt::Alignment())` | 添加跨多行多列的部件 |
| `addLayout(QLayout *layout, int row, int column, Qt::Alignment alignment=Qt::Alignment())` | 在指定位置嵌套子布局 |
| `setRowStretch(int row, int stretch)` | 设置行的拉伸因子 |
| `setColumnStretch(int column, int stretch)` | 设置列的拉伸因子 |
| `setRowMinimumHeight(int row, int minSize)` | 设置行的最小高度 |
| `setColumnMinimumWidth(int column, int minSize)` | 设置列的最小宽度 |
| `rowCount()` / `columnCount()` | 返回布局中的行数 / 列数 |
| `cellRect(int row, int column)` | 返回指定单元格的矩形区域 |
| `setOriginCorner(Qt::Corner corner)` | 设置网格的原点角（默认左上角） |

### 拉伸因子说明

拉伸因子通过 `setColumnStretch()` / `setRowStretch()` 设置，决定列/行在获得必要最小尺寸之外，还能额外获得多少可用空间。例如，若三列的拉伸因子分别为 1、2、3，则额外空间按 1:2:3 的比例分配。


## 五、QFormLayout

`QFormLayout` 以两栏形式管理输入部件及其关联标签，是创建表单界面的高层便捷类。

### 属性

| 属性名 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `fieldGrowthPolicy` | `FieldGrowthPolicy` | 取决于样式 | 控制表单字段的增长方式 |
| `formAlignment` | `Qt::Alignment` | 取决于样式 | 表单内容在布局几何区域中的对齐方式 |
| `labelAlignment` | `Qt::Alignment` | 取决于样式 | 标签列的对齐方式 |
| `rowWrapPolicy` | `RowWrapPolicy` | 取决于样式 | 行的换行策略 |
| `horizontalSpacing` | `int` | 取决于样式 | 标签与字段之间的水平间距 |
| `verticalSpacing` | `int` | 取决于样式 | 行与行之间的垂直间距 |

### FieldGrowthPolicy 枚举

| 枚举值 | 值 | 说明 |
|---|---|---|
| `FieldsStayAtSizeHint` | 0 | 字段永远不会超出其有效尺寸提示（QMacStyle 的默认值） |
| `ExpandingFieldsGrow` | 1 | 水平尺寸策略为 Expanding 或 MinimumExpanding 的字段会增长以填满可用空间 |
| `AllNonFixedFieldsGrow` | 2 | 所有非固定尺寸策略的字段都会增长以填满可用空间 |

### RowWrapPolicy 枚举

| 枚举值 | 说明 |
|---|---|
| `DontWrapRows` | 字段始终排列在标签旁边（不换行） |
| `WrapLongRows` | 标签会根据需要换行，为过长的字段腾出空间 |
| `WrapAllRows` | 字段始终排列在标签下方 |

### 关键方法

| 方法 | 说明 |
|---|---|
| `addRow(QWidget *label, QWidget *field)` | 添加一行，包含标签部件与字段部件 |
| `addRow(const QString &labelText, QWidget *field)` | 添加一行，标签为文本字符串 |
| `addRow(QLayout *layout)` | 添加一行，该行内容为一个子布局 |
| `insertRow(int row, QWidget *label, QWidget *field)` | 在指定行位置插入一行 |
| `setFieldGrowthPolicy(FieldGrowthPolicy)` | 设置字段增长策略 |
| `setRowWrapPolicy(RowWrapPolicy)` | 设置换行策略 |
| `setLabelAlignment(Qt::Alignment)` | 设置标签列对齐方式 |
| `getWidgetPosition(QWidget *widget, int *rowPtr, ItemRole *rolePtr)` | 查询部件所在的行与角色 |


## 六、QStackedLayout

`QStackedLayout` 提供一组堆叠的部件（称为“页”），同一时间仅有一个页面可见。

### 属性

| 属性名 | 类型 | 说明 |
|---|---|---|
| `currentIndex` | `int` | 当前可见部件的索引位置（只读，可通过槽修改） |
| `stackingMode` | `StackingMode` | 控制子部件可见性的处理方式 |

### StackingMode 枚举

| 枚举值 | 说明 |
|---|---|
| `StackOne` | 仅当前部件可见（默认） |
| `StackAll` | 所有部件均可见，可用于重叠绘制（如实现图形编辑器） |

### 关键方法

| 方法 | 说明 |
|---|---|
| `addWidget(QWidget *widget)` | 向堆叠中添加一个部件，返回其索引 |
| `insertWidget(int index, QWidget *widget)` | 在指定索引处插入部件 |
| `setCurrentIndex(int index)` | 设置当前可见部件的索引（槽函数） |
| `setCurrentWidget(QWidget *widget)` | 设置当前可见的部件（槽函数） |
| `currentWidget()` | 返回当前可见的部件 |
| `widget(int index)` | 返回指定索引处的部件 |
| `setStackingMode(StackingMode)` | 设置堆叠模式 |
| `count()` | 返回堆叠中的部件数量 |


## 七、布局通用特性

### 拉伸因子（Stretch Factor）

拉伸因子用于按比例分配部件之间的额外空间。未设置拉伸因子时，部件仅按 `sizePolicy` 或最小尺寸提示获得空间；设置后，额外空间按比例分配，但部件不会小于其最小尺寸提示。

### 嵌套布局

通过 `addLayout()` 可将一个布局嵌套到另一个布局中，内部布局成为插入布局的子布局，从而构建复杂的界面结构。

### 自定义部件的布局属性

当自定义 widget 时，可通过以下方式传达布局属性：

| 方法 | 说明 |
|---|---|
| 重写 `sizeHint()` | 返回 widget 的首选尺寸 |
| 重写 `minimumSizeHint()` | 返回 widget 的最小尺寸 |
| 调用 `setSizePolicy()` | 指定 widget 的空间需求 |
| 调用 `updateGeometry()` | 尺寸策略变化后通知布局重新计算 |
