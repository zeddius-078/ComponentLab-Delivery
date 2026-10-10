
## 一、三者的角色定位

| 文件 | 本质 | 角色 | 类比 |
|------|------|------|------|
| `.ui` | XML 界面描述 | **结构层**：有哪些控件、怎么摆、初始属性 | HTML |
| `.qss` | Qt 样式表 | **外观层**：长什么样、颜色/圆角/间距/状态 | CSS |
| `.cpp` | C++ 实现 | **逻辑层**：做什么、怎么响应、数据怎么流转 | JS |

一句话：

- `.ui` 管 **“有什么、在哪”**
- `.qss` 管 **“长什么样”**
- `.cpp` 管 **“干什么、怎么变”**

三者应遵循类似 Web 的 **结构 / 表现 / 行为分离** 原则。

---

## 二、边界规则（什么该放哪）

### 1. `.ui` 应该放什么

✅ 该放：
- 控件声明与层级
- 布局管理器
- 静态初始属性（文本、sizePolicy、minimumSize）
- 菜单/工具栏/动作结构
- Tab 顺序、buddy
- 控件的 `objectName`（**关键**，供 QSS 和代码定位）

❌ 不该放：
- 业务逻辑
- 复杂 QSS（最多放极少量局部样式，且应尽量外移）
- 动态内容
- 数据绑定逻辑

> 原则：`.ui` 只描述**静态骨架**，且每个需要被 QSS 或代码访问的控件都要有**语义化 objectName**。

---

### 2. `.qss` 应该放什么

✅ 该放：
- 全局主题：颜色、字体、间距、圆角、边框
- 控件外观：`QPushButton`、`QLineEdit`、`QTableView` 等
- 伪状态：`:hover`、`:pressed`、`:checked`、`:disabled`
- 动态属性选择器：`[state="error"]`
- 通过 `objectName` 选择器做局部定制：`#submitButton`

❌ 不该放：
- 布局（margin 可微调，但结构不在这）
- 逻辑判断
- 与业务状态强耦合的硬编码（应通过**动态属性**驱动）

> 原则：`.qss` 只描述**外观**，且优先用**类选择器 + 动态属性**，而不是给每个控件写死 `#id`。

---

### 3. `.cpp` 应该放什么

✅ 该放：
- 业务逻辑、数据处理
- 信号槽连接（尤其是带参数、lambda、运行时决定的）
- 动态创建/销毁控件
- 模型/视图的数据源
- 事件处理、自定义绘制
- 状态变化（并通过 `setProperty` + `polish` 触发样式更新）
- 调用 `ui->setupUi(this)` 和初始化

❌ 不该放：
- 大量 `setStyleSheet("...")` 硬编码样式
- 大段 `new QWidget` 手工搭静态界面（应放 `.ui`）
- 具体像素级的视觉描述

> 原则：`.cpp` 只描述**行为**，视觉变化通过**属性驱动 QSS**，而不是直接改样式字符串。

---

## 三、优雅分离的核心机制

### 1. objectName 是连接三者的“契约”

`.ui` 里给控件起语义化名字：

```
submitButton, userNameEdit, statusLabel, mainTable
```

- `.qss` 通过 `#submitButton` 或类选择器定位
- `.cpp` 通过 `ui->submitButton` 访问

> 命名规范统一（如 lowerCamelCase），是三者协作的基础。

---

### 2. 动态属性 + QSS：逻辑与样式的解耦桥梁

这是最关键的一招。**不要让 `.cpp` 直接写样式，而是改属性，让 QSS 响应。**

`.cpp` 中：

```cpp
// 逻辑层只表达“状态”，不表达“外观”
ui->userNameEdit->setProperty("state", "error");
ui->userNameEdit->style()->unpolish(ui->userNameEdit);
ui->userNameEdit->style()->polish(ui->userNameEdit);
```

`.qss` 中：

```css
QLineEdit[state="error"] {
    border: 1px solid #e53935;
    background: #ffebee;
}
QLineEdit[state="normal"] {
    border: 1px solid #bdbdbd;
}
```

好处：
- 逻辑层不关心颜色
- 换主题只改 `.qss`
- 状态可测试（属性可断言）

可封装成工具函数：

```cpp
void setState(QWidget *w, const QString &state) {
    w->setProperty("state", state);
    w->style()->unpolish(w);
    w->style()->polish(w);
}
```

---

### 3. 用 QSS 变量思想做主题化

Qt 的 QSS 不支持 CSS 变量，但可以：

- 用**模板占位符** + 构建时替换
- 或运行时用 `qApp->setStyleSheet(loadTheme("dark"))`
- 或按主题加载不同 `.qss` 文件

```cpp
void applyTheme(const QString &name) {
    QFile f(":/themes/" + name + ".qss");
    f.open(QFile::ReadOnly);
    qApp->setStyleSheet(f.readAll());
}
```

这样 `.cpp` 只负责“选主题”，`.qss` 负责“主题长什么样”。

---

### 4. 分层架构：把 `.cpp` 再拆开

`.cpp` 本身也不该什么都塞。推荐分层：

```
MainWindow (View)
   ├── 只做界面组装、信号转发
   ├── 持有 ui 指针
   ↓
Controller / ViewModel
   ├── 处理交互逻辑
   ├── 调用 Service
   ↓
Service / Model
   ├── 纯业务、可单测、无 Qt UI 依赖
   ↓
Data / Network / DB
```

对应文件：

- `mainwindow.ui` + `mainwindow.cpp` → 视图
- `xxxcontroller.cpp` → 逻辑
- `xxxservice.cpp` → 业务
- `theme.qss` → 样式

这样 `.ui` 和 `.qss` 只被 View 层引用，业务层完全不知道界面存在。

---

### 5. 自定义控件：把“结构+样式+逻辑”打包

对于复用组件（如带图标的输入框），推荐：

- 自定义类 `IconLineEdit : public QLineEdit`
- 内部结构可用 `.ui`（或用代码）
- 样式在 `.qss` 中用类选择器 `IconLineEdit { ... }`
- 逻辑在 `.cpp`

这样调用方只 `promote` 或直接 `new`，不需要重复写结构和样式。

---

## 四、一个具体例子串起来

**需求**：登录框，用户名错误时输入框变红。

`.ui`：

```xml
<widget class="QLineEdit" name="userNameEdit"/>
<widget class="QPushButton" name="loginButton">
  <property name="text"><string>登录</string></property>
</widget>
```

`.qss`：

```css
QLineEdit {
    border: 1px solid #bdbdbd;
    border-radius: 4px;
    padding: 6px;
}
QLineEdit[state="error"] {
    border: 1px solid #e53935;
    background: #ffebee;
}
QPushButton#loginButton {
    background: #1976d2;
    color: white;
    border-radius: 4px;
}
QPushButton#loginButton:hover {
    background: #1565c0;
}
```

`.cpp`：

```cpp
void LoginWindow::onLoginClicked() {
    QString name = ui->userNameEdit->text();
    if (name.isEmpty()) {
        setState(ui->userNameEdit, "error");   // 只改状态
        return;
    }
    setState(ui->userNameEdit, "normal");
    // ... 业务逻辑
}
```

三层各司其职，改颜色不用碰 `.cpp`，改逻辑不用碰 `.qss`。

---

## 五、常见反模式（要避免）

| 反模式 | 问题 |
|--------|------|
| `.cpp` 里满屏 `setStyleSheet("color:red")` | 样式散落，无法统一主题 |
| `.ui` 里塞大量 QSS | 结构表现混杂，难维护 |
| QSS 里给每个控件写 `#id` | 无法复用，改一处漏一处 |
| 用 `setFixedSize` 硬编码尺寸 | 不适应缩放/多语言 |
| 业务逻辑写进 MainWindow | 无法单测，越写越臃肿 |
| objectName 随便起（`pushButton_1`） | QSS 和代码都难读 |

