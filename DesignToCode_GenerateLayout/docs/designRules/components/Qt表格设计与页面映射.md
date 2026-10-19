# Qt 表格设计与页面映射

## 当前结论

表格能力必须拆成三层验证：Qt 原生视图/模型能力、GenerateComponent 的公开组件合同、GenerateLayout 的页面装配。不能因 `QTableView` 或 `QTableWidget` 可创建，就宣称排序、筛选、分页、编辑、加载和空态均已完成。

当前 Profile 使用 `standard-light`、`standard-dark`、`wide-light`、`wide-dark`。表格的紧凑度、行高、单元格 Padding 和控件 Size 是独立属性，不得编码进 Profile 名称。

## 职责划分

| 层 | 负责 | 不负责 |
| --- | --- | --- |
| Figma | 表格组件身份、Variant、视觉 Token 绑定和展示状态 | Qt Model/View 生命周期 |
| GenerateComponent | 表格公开类、Token/QSS、基础状态、资产解析 | 页面业务查询与数据权限 |
| GenerateLayout | 页面容器、布局、公开属性装配、控制器连接 | 复制表格内部实现或重建 Token |
| 业务控制器 | 数据源、排序/筛选请求、分页结果、编辑提交与错误恢复 | 修改组件库内部样式合同 |

## 页面映射规则

1. 设计身份必须解析到当前组件目录；禁止按图层名称猜类。
2. 页面只设置公开属性和信号，不访问 SDK 私有子控件。
3. 外部布局控制占位、拉伸和页面间距；组件 Token 控制内部行高、边距、字体、边框和状态。
4. `Compact` 等 Density 只传给声明该属性的组件或 LayoutContainer。
5. 图标、空态图和大型插图使用 `icon.*`、`image.*`、`model.*` 语义 ID，通过 SDK Catalog 解析。
6. 未登记列类型、状态、资源或交互必须返回明确错误，不能用普通矩形或相似图标替代。

## 最小验收矩阵

| 维度 | 最少检查 |
| --- | --- |
| 数据 | 空、单行、多行、长文本、缺失值 |
| 状态 | Default、Hover、Selected、Selected-hover、Disabled；仅检查合同中存在的状态 |
| 结构 | 表头、行、列宽、滚动、父布局伸缩 |
| 交互 | 选择、键盘焦点、禁用；排序/筛选/分页仅在接入对应控制器时检查 |
| Profile | 至少验证交付目标 Profile；未运行的组合保持“未验证” |
| Density | Default/Compact 等值按组件合同逐项验证，不从 Profile 推导 |
| 资源 | 所有公共语义 ID 可解析，未知 ID 明确失败 |
| 可追溯 | 组件 ID、映射版本、输入哈希、SDK manifest 与运行结果可回溯 |

自动结构和运行测试通过，只证明已覆盖的代码路径；设计视觉一致性仍需在指定字体、缩放、窗口尺寸和 Profile 下人工复核。
