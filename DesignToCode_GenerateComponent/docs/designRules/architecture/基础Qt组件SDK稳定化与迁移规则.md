# 基础 Qt 组件 SDK 稳定化与迁移规则

> 状态：基础原生 SDK 已独立发布；复合、自绘、页面型和实验性组件仍不属于此发布物。
> 生效日期：2026-10-24
> 适用项目：`DesignToCode_GenerateComponent`、`DesignToCode_GenerateLayout`、`ComponentLab`

## 1. 发布边界

本轮只发布 **Qt 原生类到 `Z_*` 包装类** 的基础层。唯一允许的身份关系为：

```text
Qt 5 原生 QWidget 类 ── public inheritance ──> commonWidget::Z_<Qt 类去掉 Q 前缀>
```

基础层保留 Qt 的构造、信号槽、事件、可访问性和数据模型语义；`Z_*` 仅提供项目命名空间、组件专属 QSS 选择器和必要的迁移兼容边界。它不是重新定义一套平行控件体系。

基础 SDK 不包含下列类型：

- 由多个控件拼装的复合组件；
- 以项目 `QPainter`、影子层或独立状态机重绘的自绘组件；
- 页面、导航壳、应用窗口或页面流转组件；
- 实验性组件、未完成组件，以及仅有 Figma/合同但没有可验证原生实现的组件。

这些项目可以保留为 `legacy-runtime`、`contract-only` 或 `planned`，但不得出现在基础 SDK 的公开头文件、安装清单、Factory 注册表或“已发布组件数”中。它们不阻塞基础迁移包。

基础范围的唯一机器可读清单是 [base-sdk-scope.json](../../../Source/ComponentKit/component-definitions/base-sdk-scope.json)。该清单冻结目前获准的 Qt 类集合；新增 Qt 包装类必须经过范围评审，不能因写入 `native-components.json` 自动进入发布包。

## 2. 分层与职责

本规则是 [Qt 功能分层](Qt功能分层.md) 的发布约束，不替代其五层边界。

| 层 | 负责内容 | 不负责内容 |
| --- | --- | --- |
| GenerateComponent | `Z_*` 公开头文件/CPP、组件 `View` 中的 QSS/QRC、组件 QSS 编译、基础 SDK Manifest | 页面 `.ui`、页面业务和应用数据 |
| GenerateLayout | 页面 `Form/.ui`、布局、objectName、页面局部 QSS | 直接改写组件 QSS，或改动 `Z_*` 的公开 API/行为 |
| ComponentLab | 只消费已发布 SDK，展示已发布组件与状态 | 伪造组件外观、携带组件实现，或把待实现合同展示为已发布 |

GenerateLayout 需要刷新组件视觉时，只能调用 GenerateComponent 的受控 QSS 编译入口；组件 QSS 的所有权仍属于 GenerateComponent。

## 3. Token 与视觉边界

Token 的作用是构建期生成或更新对应组件的固定 QSS 值，而不是在运行时创建新变量系统。

```text
Token 原始值 / 已确认组件 QSS
  → GenerateComponent QSS 编译或直接编辑
  → View/<component>.<profile>.qss + .qrc + manifest 哈希
  → SDK / 源码包消费
```

允许：组件 QSS 内的固定颜色、字体、尺寸、状态值；必要的 `qproperty-*` 仅用于自绘组件的组件局部绘制属性。

禁止：

- 在运行时从 `palette.json`、roles JSON、Token JSON 读取视觉值；
- 新增 `--component-*`、`--qtlab-*`、sToken 或全局 carrier/probe 变量体系；
- 仅因 Token 值变化修改 `Include/`、`SRC/`、公开 API、信号槽或控件行为；
- 用 Gallery 画布 QSS 伪装组件视觉。

基础原生控件优先使用 Qt/QSS 直接选择器。若 Qt 原生能力无法表达某一视觉，先记录为后续组件局部适配项，不把通用中间件重新引入基础包。

## 4. 可迁移源码包合同

每一个可迁移组件包必须按下列结构交付；目录名是合同的一部分：

```text
<ComponentPackage>/
  Form/        # .ui 文件
  View/        # .qrc、.qss、SVG、字体及视觉资源
  Include/     # 对外公开 .h
  SRC/         # 对应 .cpp
  cmake/       # CMake 导入和构建描述
  manifest/    # 文件清单、哈希、版本、依赖与许可证
```

必须同时支持两种消费方式：

1. 源码包：消费者以 `add_subdirectory()` 引入，引用 `Include/` 中的公开头文件；
2. SDK 包：消费者以 `find_package(GeneratedComponentSDK CONFIG REQUIRED)` 链接已验证的库。

源码包不得依赖生产方绝对路径、生产方 `output/work`、构建缓存或未发布的私有资源。`.ui` 只通过 `Form`，QRC/QSS/字体等只通过 `View`，公开 C++ 接口只通过 `Include`，实现只通过 `SRC`。

## 5. 基础 SDK 发布门禁

基础 SDK 只有同时满足以下条件才能标记为 `published`：

1. 清单中每一项均是批准的 `Q* → Z_*` 一对一包装，且不含复合/自绘/页面/实验性公开项；
2. Release 包通过独立临时目录的 `find_package` 消费构建和运行验证；Debug 只能使用完整、同 CRT 的 Debug 包，不能把 Release `.lib` 伪装成 Debug；
3. 源码包通过独立临时目录的 `add_subdirectory` 构建验证；
4. 所有 QRC 引用在包内闭包，QSS 不包含生产方绝对路径、`output/work` 或构建缓存路径；
5. Manifest 记录文件哈希、Qt/编译器/CRT、组件范围、许可证和已知限制；
6. Token 变更回归证明只影响 `View/`、QRC 和 Manifest，未改变基础包装类的头文件、CPP 或 Qt 行为；
7. ComponentLab 仅把通过上述门禁的项显示为“已发布”；延后项须显示其实际状态。

当前 `build/sdk` 已由基础原生发布流程事务切换为 `qt-native-baseline`。此前的全量运行时保留在源码和唯一上一有效版本中，作为历史/回退证据，不再是基础 SDK 的发布入口。

## 6. 状态命名

| 状态 | 含义 | 是否可安装到基础 SDK |
| --- | --- | --- |
| `baseline-native` | 已批准的 Qt 原生 `Q* → Z_*` 映射，待/已通过基础门禁 | 可以，前提是门禁通过 |
| `legacy-runtime` | 历史可运行实现，但架构或视觉链路尚未收敛 | 不可以 |
| `contract-only` | 只有命名/设计合同，不能实例化或验收 | 不可以 |
| `planned` | 后续设计或工程范围 | 不可以 |

不得用 `mapped`、Factory 可创建或 ComponentLab 有截图替代上述发布状态。

## 7. 验收证据与回退

发布证据必须保存在该次 `output/delivery` 的验收摘要中，包含源包与 SDK 包的独立构建日志、运行结果和 Manifest 哈希。`build/sdk` 事务切换前保留唯一上一有效版本于 `build/.state/previous-sdk`；普通 clean 不删除 `build` 或 `output/delivery`。

若任一门禁失败，保留此前 SDK，失败产物只留在 `output/work`，不得向 ComponentLab 或其他消费者宣称已发布。
