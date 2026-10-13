# 布局生成器与组件 SDK 的协作

## 权限矩阵

| 能力 | GenerateComponent | GenerateLayout |
| --- | --- | --- |
| 定义设计组件身份和属性值域 | 负责 | 读取 |
| 编译 Token、QSS、catalog | 负责 | 无生成入口 |
| 实现 QWidget、自绘和状态 | 负责 | 无组件源码 |
| 发布组件 SDK 与映射 | 负责 | 直接引用并校验 |
| 解析设计稿容器和嵌套布局 | 提供组件身份 | 负责 |
| 生成 `.ui` 和页面数据 | 提供可引用类 | 负责 |
| 连接页面业务交互 | 提供组件信号/API | 负责 |

## 发布与消费

GenerateComponent 运行 `Source/Publish-ComponentSDK.ps1`，输出：

- `lib/GeneratedComponentSDK.lib`
- `include/*.h`
- `resources/mappings/design-component-map.json`
- `resources/mappings/qt-component-map.json`
- `manifest/component-sdk.json`
- `licenses/*.txt`

GenerateLayout 运行 `Source/Verify-GeneratedComponentSDK.ps1`。脚本校验工作区唯一权威 SDK 的发布者、权限字段和每个文件的 SHA-256，不复制文件。CMake 默认从同级 `DesignToCode_GenerateComponent/build/sdk` 导入静态库，外部工作区可通过 `GENERATED_COMPONENT_SDK_ROOT` 指向同一权威发布目录；不提供源码回退路径。

## 未映射处理

1. 生成器保存设计节点 ID、组件身份、属性和状态。
2. 映射表找不到身份或值域时，输出结构化未映射错误。
3. GenerateComponent 补充组件或映射，完成 Token 和组件测试。
4. 发布新版 SDK。
5. GenerateLayout 校验新版权威 SDK 并生成页面。
6. 页面测试验证 XML、组件调用和运行态。

这个顺序保证布局生成器只做解析与装配，组件能力始终来自同一发布源。
