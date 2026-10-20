# Qt 表格组件样式、Token 与应用合同

本文记录表格组件的设计、Token 和 Qt 实现边界。表格是由容器、表头、行、单元格、编辑器、排序/筛选控制和状态提示组成的复合组件。

## 试点范围

当前试点验证：

| 项目 | 内容 |
| --- | --- |
| 结构 | 表格容器、表头、内容区、空状态和加载状态 |
| 视觉 | 背景、分隔线、文字、选中行、悬停行、禁用和错误提示 |
| Token | 表面、边框、文字、间距、行高、字体和状态颜色 |
| Qt 能力 | `QTableView`、Model、Delegate、SelectionModel |
| 输出 | Contract、Recipe、QSS、组件 API 和 ComponentLab 展示 |

## 部件模型

| 部件 | 设计身份 | Qt 落点 |
| --- | --- | --- |
| Root | `Table` | `Z_TableView` 或已登记复合组件 |
| Header | `Table/Header` | `QHeaderView` 与表头 QSS |
| Body | `Table/Body` | `QTableView` viewport |
| Row | `Table/Row` | Model 数据与选中/悬停状态 |
| Cell | `Table/Cell` | Delegate 绘制和编辑器 |
| Empty | `Table/Empty` | 空状态组件 |
| Loading | `Table/Loading` | 运行时状态和进度表达 |

## Token 绑定

Token 绑定使用固定 Slot 和属性名。常见目标包括：

```text
container.background
header.background
row.background.default
row.background.hover
row.background.selected
cell.text.default
cell.text.disabled
cell.border
cell.padding
cell.height
```

生成器输出 QSS 时保留来源路径。Delegate 需要的绘制参数通过 Token 服务或组件主题接口读取，不在 Delegate 中复制硬编码颜色。

## Qt API 分工

| 需求 | Qt 或组件接口 |
| --- | --- |
| 数据行和列 | `QAbstractItemModel` |
| 选中状态 | `QItemSelectionModel` |
| 单元格绘制 | `QStyledItemDelegate` |
| 排序 | Model 的排序能力或 `QSortFilterProxyModel` |
| 编辑器 | Delegate 创建的 QWidget 或已登记 Z 组件 |
| 静态视觉 | QSS 和 Token |
| 业务加载/错误 | Controller 或业务 Model |

## 验收

至少覆盖：空表、有数据表、选中行、悬停行、禁用、错误、加载、编辑和主题切换。验证分为 Token 生成、组件构建、运行交互和视觉核对四步。

表格数据模型属于业务工程。ComponentLab 只提供固定演示数据，GenerateLayout 只负责页面结构和已登记组件实例。
