# Qt 功能分层

本文说明 `.ui`、QSS、C++ 组件和页面逻辑在组件生产链中的职责。

## 分层

| 层 | 文件或对象 | 负责内容 |
| --- | --- | --- |
| 结构层 | `.ui`、Qt Layout、`objectName` | 父子关系、布局、静态属性、提升类声明 |
| 视觉层 | Token、组件 QSS、页面 QSS | 颜色、字体、尺寸、边框、圆角、状态外观 |
| 组件层 | `Z_*.h/.cpp` | 公共 API、内部结构、绘制、事件和状态 |
| 页面层 | 页面 `.h/.cpp`、Controller | 页面初始化、业务信号、运行时状态和数据 |
| 资源层 | `.qrc`、SDK resources | 图标、图片、字体、组件 QSS 和页面 QSS |

## `.ui` 的职责

`.ui` 保存可以由 Qt Designer 和 `uic` 表达的结构：

- QWidget 和提升组件类型；
- objectName；
- QVBoxLayout、QHBoxLayout、QGridLayout、QFormLayout 等布局；
- 尺寸策略、最小/最大尺寸、间距、边距、对齐；
- 适合序列化的静态属性。

组件内部对象树由 SDK 组件实现维护，页面 `.ui` 只声明页面级组件实例。

## QSS 的职责

组件 QSS 由 GenerateComponent 根据 Token 生成，随 SDK 发布。页面 QSS 只处理页面容器和页面局部状态。颜色、字体、间距、边框、圆角和状态选择从 Token 或组件合同读取。

QSS 不负责业务判断、数据绑定、模型查询或复杂绘制。复杂图形由 QWidget/C++ 组件实现。

## C++ 的职责

`Z_*` 组件通过公开头文件提供构造函数、`Q_PROPERTY`、setter、信号和状态 API。页面 C++ 在 `ui_->setupUi(this)` 后执行必要的运行时初始化、信号连接和页面级 QSS 加载。

组件公共视觉由 SDK 管理。页面构造函数只生成合同明确要求的运行时调用，不重复写入组件内部颜色和尺寸。

## objectName 合同

`objectName` 是页面 XML、页面 C++、QSS 选择器和验证脚本之间的连接字段。生成器使用稳定工程名；同名设计实例通过稳定后缀区分。设计节点原名和 ID 保存在追溯信息中。

## 主题与 Token

Token 的运行上下文由 Profile 和 Theme 组成：

```text
standard-light / standard-dark / wide-light / wide-dark
```

组件密度、布局间距和页面尺寸由组件或布局属性表达。Profile 不承担业务状态；State 由组件合同登记。

## 一个组件的落点

```text
设计组件身份 → Registry / Contract / Recipe
Token 绑定 → Token 编译 → 组件 QSS
Qt 基类映射 → Z_* .h/.cpp
页面实例 → .ui
运行时属性 → 页面 .cpp / ComponentFactory
```

## 维护原则

组件公共行为和公共视觉修改 GenerateComponent。页面结构和页面局部样式修改 GenerateLayout。展示页面修改 ComponentLab。三者通过发布 SDK 连接。

## 基础 SDK 发布范围

基础 SDK 的发布边界、包目录合同、Token 视觉边界和验收门禁以
[基础 Qt 组件 SDK 稳定化与迁移规则](基础Qt组件SDK稳定化与迁移规则.md) 为准。

本轮仅发布 `Q* → commonWidget::Z_*` 的基础包装层；复合、自绘、页面型和实验性组件必须标为 `legacy-runtime`、`contract-only` 或 `planned`，不得混入基础 SDK 的公开安装内容。
