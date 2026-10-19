

## 一、.ui 文件的基本结构

本文说明 Qt Designer `.ui` 的通用 XML 能力。DesignToCode 当前实际生成字段、设计侧输入要求和失败条件以[布局转换器通用化规范](布局转换器通用化规范.md)为准；Qt 原生 XML 能表达的能力不会自动等同于设计映射链路已经接入。

`.ui` 文件是 Qt Designer 生成的 XML 格式文件，根元素为 `<ui>`，内部包含 `<class>`（类名）和 `<widget>`（顶层部件）两个核心元素。

```xml
<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
  <class>Form</class>
  <widget class="QWidget" name="Form">
    <!-- 部件属性 -->
    <property name="geometry">
      <rect><x>0</x><y>0</y><width>400</width><height>300</height></rect>
    </property>
    <property name="windowTitle">
      <string>Form</string>
    </property>
    <!-- 布局在此处定义 -->
  </widget>
  <resources/>
  <connections/>
</ui>
```

**关键规则**：`<layout>` 元素必须作为 `<widget>` 的直接子元素出现，它管理该 widget 内部所有子部件的几何排列。布局元素不能独立存在，必须依附于一个容器部件。


## 二、布局元素的核心 XML 标签

| XML 标签 | 对应 C++ 类 | 用途 |
|---|---|---|
| `<layout class="QVBoxLayout">` | `QVBoxLayout` | 垂直排列子部件 |
| `<layout class="QHBoxLayout">` | `QHBoxLayout` | 水平排列子部件 |
| `<layout class="QGridLayout">` | `QGridLayout` | 二维网格排列 |
| `<layout class="QFormLayout">` | `QFormLayout` | 标签-字段双列表单 |
| `<item>` | `QLayoutItem` | 布局中的一个子项（部件、子布局或间隔器） |
| `<spacer>` | `QSpacerItem` | 弹性或固定空白占位 |
| `<property>` | 属性设置 | 设置布局或部件的属性 |


## 三、各类布局的 XML 写法

### 3.1 QVBoxLayout / QHBoxLayout

最基础的布局，子项按顺序垂直或水平排列。通过 `<item>` 包装每个子部件。

```xml
<layout class="QVBoxLayout" name="verticalLayout">
  <property name="leftMargin"><number>5</number></property>
  <property name="topMargin"><number>5</number></property>
  <property name="rightMargin"><number>5</number></property>
  <property name="bottomMargin"><number>5</number></property>
  <property name="spacing"><number>6</number></property>

  <item>
    <widget class="QLabel" name="titleLabel">
      <property name="text"><string>标题</string></property>
    </widget>
  </item>
  <item>
    <widget class="QLineEdit" name="inputEdit"/>
  </item>
  <item>
    <widget class="QPushButton" name="okButton">
      <property name="text"><string>确定</string></property>
    </widget>
  </item>
</layout>
```

**方向控制**：`QVBoxLayout` 和 `QHBoxLayout` 本身不设方向属性，方向由类名决定。`QBoxLayout` 的 `Direction` 枚举（`LeftToRight`、`TopToBottom` 等）在 .ui 中通常不直接使用，因为 Designer 会生成对应的 `QVBoxLayout` 或 `QHBoxLayout`。

### 3.2 QGridLayout

网格布局通过 `<item>` 的 `row`、`column`、`rowspan`、`colspan` 属性定位子部件。

```xml
<layout class="QGridLayout" name="gridLayout">
  <property name="leftMargin"><number>5</number></property>
  <property name="spacing"><number>5</number></property>

  <item row="0" column="0">
    <widget class="QLabel" name="nameLabel">
      <property name="text"><string>姓名：</string></property>
    </widget>
  </item>
  <item row="0" column="1">
    <widget class="QLineEdit" name="nameEdit"/>
  </item>
  <item row="1" column="0">
    <widget class="QLabel" name="emailLabel">
      <property name="text"><string>邮箱：</string></property>
    </widget>
  </item>
  <item row="1" column="1" colspan="2">
    <widget class="QLineEdit" name="emailEdit"/>
  </item>
</layout>
```

**拉伸因子**：可通过布局的属性设置列/行的拉伸比例：

```xml
<layout class="QGridLayout" name="gridLayout" columnstretch="0,1,2" rowstretch="0,1">
```

`columnstretch="0,1,2"` 表示三列的额外空间按 0:1:2 分配，第一列不获取额外空间，第二列和第三列分别获得 1/3 和 2/3 的剩余空间。

### 3.3 QFormLayout

表单布局专为“标签-输入字段”模式设计，每个 `<item>` 用 `row` 指定行，用 `column` 区分标签（column=0）和字段（column=1）。

```xml
<layout class="QFormLayout" name="formLayout">
  <property name="fieldGrowthPolicy">
    <enum>QFormLayout::FieldsStayAtSizeHint</enum>
  </property>
  <property name="labelAlignment">
    <set>Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter</set>
  </property>
  <property name="horizontalSpacing"><number>5</number></property>
  <property name="verticalSpacing"><number>5</number></property>

  <item row="0" column="0">
    <widget class="QLabel" name="nameLabel">
      <property name="text"><string>姓名：</string></property>
    </widget>
  </item>
  <item row="0" column="1">
    <widget class="QLineEdit" name="nameEdit"/>
  </item>
  <item row="1" column="0">
    <widget class="QLabel" name="ageLabel">
      <property name="text"><string>年龄：</string></property>
    </widget>
  </item>
  <item row="1" column="1">
    <widget class="QSpinBox" name="ageSpin"/>
  </item>
</layout>
```

QFormLayout 的关键枚举属性：

| 属性 | 可选值 | 说明 |
|---|---|---|
| `fieldGrowthPolicy` | `FieldsStayAtSizeHint` / `ExpandingFieldsGrow` / `AllNonFixedFieldsGrow` | 字段列的增长方式 |
| `rowWrapPolicy` | `DontWrapRows` / `WrapLongRows` / `WrapAllRows` | 行的换行策略 |
| `labelAlignment` | `Qt::AlignLeft` / `Qt::AlignRight` 等 | 标签列对齐 |
| `formAlignment` | `Qt::AlignHCenter` 等 | 整个表单的对齐方式 |


## 四、布局的通用属性

所有布局类都支持以下 `<property>` 设置：

| 属性名 | XML 写法 | 说明 |
|---|---|---|
| `leftMargin` | `<property name="leftMargin"><number>N</number></property>` | 左外边距 |
| `topMargin` | 同上 | 上外边距 |
| `rightMargin` | 同上 | 右外边距 |
| `bottomMargin` | 同上 | 下外边距 |
| `spacing` | `<property name="spacing"><number>N</number></property>` | 子项之间的间距 |
| `sizeConstraint` | `<property name="sizeConstraint"><enum>QLayout::SetDefaultConstraint</enum></property>` | 布局对窗口尺寸的约束 |

**sizeConstraint 的可用枚举值**：

| 枚举值 | 含义 |
|---|---|
| `QLayout::SetDefaultConstraint` | 使用默认约束 |
| `QLayout::SetFixedSize` | 窗口固定为 sizeHint |
| `QLayout::SetMinimumSize` | 窗口最小为 sizeHint |
| `QLayout::SetMaximumSize` | 窗口最大为 sizeHint |
| `QLayout::SetMinAndMaxSize` | 窗口固定为 sizeHint |
| `QLayout::SetNoConstraint` | 无约束 |


## 五、间隔器（Spacer）

间隔器是布局中的弹性或固定空白，用于控制部件的对齐和空间分配。

```xml
<item>
  <spacer name="horizontalSpacer">
    <property name="orientation">
      <enum>Qt::Horizontal</enum>
    </property>
    <property name="sizeType">
      <enum>QSizePolicy::Expanding</enum>
    </property>
    <property name="sizeHint" stdset="0">
      <size><width>40</width><height>20</height></size>
    </property>
  </spacer>
</item>
```

**sizeType 的常用值**：

| 值 | 说明 |
|---|---|
| `QSizePolicy::Fixed` | 固定尺寸，不伸缩 |
| `QSizePolicy::Expanding` | 尽可能占据可用空间 |
| `QSizePolicy::Minimum` | 至少为 sizeHint，可以更大 |
| `QSizePolicy::Preferred` | 优先使用 sizeHint，可伸缩 |

**典型用法**：在按钮行中，在左侧按钮和右侧按钮之间插入一个 `Expanding` 的水平间隔器，可以将右侧按钮推到最右边。


## 六、嵌套布局

布局可以嵌套：一个 `<layout>` 可以作为另一个 `<layout>` 的 `<item>` 子元素出现。嵌套时，内层布局的 `<item>` 结构保持不变。

```xml
<layout class="QVBoxLayout" name="mainLayout">
  <!-- 顶部工具栏区域：水平布局 -->
  <item>
    <layout class="QHBoxLayout" name="toolbarLayout">
      <item>
        <widget class="QToolButton" name="newBtn">
          <property name="text"><string>新建</string></property>
        </widget>
      </item>
      <item>
        <widget class="QToolButton" name="openBtn">
          <property name="text"><string>打开</string></property>
        </widget>
      </item>
      <item>
        <spacer name="toolbarSpacer">
          <property name="orientation"><enum>Qt::Horizontal</enum></property>
          <property name="sizeType"><enum>QSizePolicy::Expanding</enum></property>
        </spacer>
      </item>
    </layout>
  </item>

  <!-- 中间内容区域 -->
  <item>
    <widget class="QTextEdit" name="contentEdit"/>
  </item>

  <!-- 底部按钮区域：水平布局 -->
  <item>
    <layout class="QHBoxLayout" name="buttonLayout">
      <item>
        <spacer name="buttonSpacer">
          <property name="orientation"><enum>Qt::Horizontal</enum></property>
          <property name="sizeType"><enum>QSizePolicy::Expanding</enum></property>
        </spacer>
      </item>
      <item>
        <widget class="QPushButton" name="cancelBtn">
          <property name="text"><string>取消</string></property>
        </widget>
      </item>
      <item>
        <widget class="QPushButton" name="okBtn">
          <property name="text"><string>确定</string></property>
        </widget>
      </item>
    </layout>
  </item>
</layout>
```


## 七、子部件的尺寸策略（sizePolicy）

`sizePolicy` 是 widget 的属性（不是布局的属性），但它直接影响布局如何分配空间。在 `<widget>` 元素内设置：

```xml
<widget class="QTextEdit" name="textEdit">
  <property name="sizePolicy">
    <sizepolicy hsizetype="Expanding" vsizetype="Expanding">
      <horstretch>0</horstretch>
      <verstretch>1</verstretch>
    </sizepolicy>
  </property>
</widget>
```

**hsizetype / vsizetype 的常用值**：

| 值 | 说明 |
|---|---|
| `Fixed` | 固定为 sizeHint，不伸缩 |
| `Minimum` | 至少为 sizeHint，可以更大 |
| `Maximum` | 至多为 sizeHint，可以更小 |
| `Preferred` | 优先 sizeHint，可伸缩 |
| `Expanding` | 尽可能占据空间，可伸缩 |
| `MinimumExpanding` | 至少为 sizeHint，且优先扩展 |


## 八、对齐设置（alignment）

可以在 `<item>` 级别设置子项在布局单元格内的对齐方式：

```xml
<item alignment="Qt::AlignHCenter|Qt::AlignVCenter">
  <widget class="QListWidget" name="listWidget"/>
</item>
```

也可以作为 widget 的属性设置：

```xml
<widget class="QLabel" name="infoLabel">
  <property name="alignment">
    <set>Qt::AlignCenter</set>
  </property>
</widget>
```

**常用的 Qt::Alignment 值**：`Qt::AlignLeft`、`Qt::AlignRight`、`Qt::AlignHCenter`、`Qt::AlignTop`、`Qt::AlignBottom`、`Qt::AlignVCenter`、`Qt::AlignCenter`。多个值用竖线 `|` 组合。


## 九、表单级默认布局属性

在 `<ui>` 元素的末尾（`<widget>` 之后），可以设置 `<layoutdefault>` 来指定表单中所有布局的默认边距和间距：

```xml
<layoutdefault spacing="6" margin="11"/>
```

这些默认值会被 `uic` 在生成代码时使用，如果没有为具体布局显式设置属性，则采用这些默认值。


## 十、完整示例：一个带网格布局的设置对话框

```xml
<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
  <class>SettingsDialog</class>
  <widget class="QDialog" name="SettingsDialog">
    <property name="geometry">
      <rect><x>0</x><y>0</y><width>500</width><height>350</height></rect>
    </property>
    <property name="windowTitle">
      <string>设置</string>
    </property>

    <layout class="QGridLayout" name="mainGrid" columnstretch="0,1">
      <property name="leftMargin"><number>10</number></property>
      <property name="topMargin"><number>10</number></property>
      <property name="rightMargin"><number>10</number></property>
      <property name="bottomMargin"><number>10</number></property>
      <property name="spacing"><number>8</number></property>

      <!-- 第0行：服务器地址 -->
      <item row="0" column="0">
        <widget class="QLabel" name="serverLabel">
          <property name="text"><string>服务器地址：</string></property>
        </widget>
      </item>
      <item row="0" column="1">
        <widget class="QLineEdit" name="serverEdit">
          <property name="placeholderText">
            <string>https://example.com</string>
          </property>
        </widget>
      </item>

      <!-- 第1行：端口 -->
      <item row="1" column="0">
        <widget class="QLabel" name="portLabel">
          <property name="text"><string>端口：</string></property>
        </widget>
      </item>
      <item row="1" column="1">
        <widget class="QSpinBox" name="portSpin">
          <property name="minimum"><number>1</number></property>
          <property name="maximum"><number>65535</number></property>
          <property name="value"><number>8080</number></property>
        </widget>
      </item>

      <!-- 第2行：启用日志（复选框跨两列） -->
      <item row="2" column="0" colspan="2">
        <widget class="QCheckBox" name="logCheck">
          <property name="text"><string>启用详细日志</string></property>
        </widget>
      </item>

      <!-- 第3行：说明文字（跨两列） -->
      <item row="3" column="0" colspan="2">
        <widget class="QLabel" name="hintLabel">
          <property name="text">
            <string>修改后需重启应用才能生效。</string>
          </property>
          <property name="alignment">
            <set>Qt::AlignLeft|Qt::AlignVCenter</set>
          </property>
        </widget>
      </item>

      <!-- 第4行：弹性间隔，将按钮推到下方 -->
      <item row="4" column="0">
        <spacer name="verticalSpacer">
          <property name="orientation">
            <enum>Qt::Vertical</enum>
          </property>
          <property name="sizeType">
            <enum>QSizePolicy::Expanding</enum>
          </property>
        </spacer>
      </item>

      <!-- 第5行：按钮行（跨两列，内部嵌套水平布局） -->
      <item row="5" column="0" colspan="2">
        <layout class="QHBoxLayout" name="buttonRow">
          <item>
            <spacer name="buttonSpacer">
              <property name="orientation">
                <enum>Qt::Horizontal</enum>
              </property>
              <property name="sizeType">
                <enum>QSizePolicy::Expanding</enum>
              </property>
            </spacer>
          </item>
          <item>
            <widget class="QPushButton" name="cancelBtn">
              <property name="text"><string>取消</string></property>
            </widget>
          </item>
          <item>
            <widget class="QPushButton" name="okBtn">
              <property name="text"><string>确定</string></property>
            </widget>
          </item>
        </layout>
      </item>

    </layout>
  </widget>
  <layoutdefault spacing="6" margin="11"/>
  <resources/>
  <connections/>
</ui>
```

这个示例展示了：网格布局的行列定位、跨列合并（`colspan`）、弹性间隔器的空间分配、嵌套水平布局管理按钮行，以及 `columnstretch` 控制列宽比例。


## 十一、编写 .ui 布局的关键规则总结

1. **布局必须依附于 `<widget>`**：`<layout>` 只能作为 `<widget>` 的直接子元素出现，不能独立存在。

2. **每个子项必须用 `<item>` 包装**：无论是子部件、子布局还是间隔器，都必须放在 `<item>` 元素内。

3. **网格定位用 `row` / `column` 属性**：`<item>` 上的 `row`、`column`、`rowspan`、`colspan` 仅在 `QGridLayout` 和 `QFormLayout` 中有效。

4. **嵌套时保持 `<item>` 结构不变**：子布局作为 `<item>` 的子元素插入，子布局内部的 `<item>` 结构保持不变。

5. **`name` 属性必须唯一且有意义**：布局的 `name` 和部件的 `name` 在同一个 `.ui` 文件中应唯一，便于在代码中引用。

6. **属性值用 `<number>` 包裹整数**：如 `<property name="spacing"><number>6</number></property>`。

7. **枚举值用 `<enum>` 包裹**：如 `<enum>Qt::Horizontal</enum>`。

8. **对齐用 `<set>` 包裹**：如 `<set>Qt::AlignCenter</set>`。

9. **`layoutdefault` 放在 `<ui>` 末尾**：作为表单级默认值，与 `<resources/>` 和 `<connections/>` 同级。

10. **`QFormLayout` 中标签用 `column="0"`，字段用 `column="1"`**：这是 QFormLayout 的约定，与普通 QGridLayout 不同。
