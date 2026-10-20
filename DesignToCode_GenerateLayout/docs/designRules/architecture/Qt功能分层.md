# Qt 页面功能分层

## 页面文件职责

| 文件 | 负责内容 | 典型输入 |
| --- | --- | --- |
| `.ui` | 页面结构、objectName、布局、可序列化的 Qt 属性 | 设计节点树、布局合同 |
| 页面 `.qss` | 页面容器和页面专属视觉规则 | 页面 Token、页面状态 |
| SDK QSS | 公共组件状态、Token 和主题 | GenerateComponent SDK |
| `.cpp` | 页面构造、运行时属性、信号连接和页面状态初始化 | 组件公开 API、页面合同 |
| `.h` | 页面类声明和稳定访问接口 | 生成器页面模型 |
| `.qrc` | 页面 QSS 和页面自有资源注册 | 页面资源清单 |

## 连接方式

`objectName` 是 `.ui`、页面代码和 QSS 之间的稳定连接。组件类、属性和状态通过 SDK manifest 与组件合同解析；页面生成器使用公开 API，不拼接未知 setter。

```text
设计节点
  -> objectName / componentRef
  -> .ui 结构
  -> 页面 .cpp 初始化
  -> SDK QSS + 页面 QSS
```

## 属性落点

| 属性 | 生成位置 |
| --- | --- |
| 类型、父子关系、布局 | `.ui` |
| 颜色、字体、边框、状态样式 | SDK Token/QSS 或页面 QSS |
| 必须调用的公开运行时 API | `.cpp` |
| 业务数据、模型、校验和动作 | 业务工程 |

页面生成器只输出页面所需内容。公共组件内部绘制、Token 计算和主题安装由 SDK 负责。

## 初始化顺序

SDK 页面按以下顺序运行：

```text
ThemeHost / 应用主题
  -> ui_->setupUi(this)
  -> 注册页面 QRC
  -> ComponentFactory 应用运行时属性
  -> 加载页面 QSS
```

实际顺序以生成器和 SDK 公开接口为准。`public-qt` 页面使用公开 Qt API，不调用 `ThemeHost` 或 `ComponentFactory`。
