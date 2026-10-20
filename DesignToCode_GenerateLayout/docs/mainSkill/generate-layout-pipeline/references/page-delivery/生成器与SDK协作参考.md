# 布局生成器与组件 SDK 的协作

## 权限矩阵

| 能力 | GenerateComponent | GenerateLayout |
| --- | --- | --- |
| 定义已实现设计组件身份和属性值域 | 负责 | 读取 |
| 独立目标组件集类名合同 | 不生成假实现 | 通过命名配置解析，planned 仅预览 |
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
- `resources/mappings/figma-component-map-v2.json`
- `resources/mappings/component-source-map.json`
- `resources/mappings/qt-component-map.json`
- `manifest/component-sdk.json`
- `licenses/*.txt`

GenerateLayout 运行 `Source/Verify-GeneratedComponentSDK.ps1`。脚本拒绝旧清单，校验 v2 发布身份、权限、Release/x64/MD（显式 Debug 对应 MDd）、工具链元数据及全量文件集合/长度/SHA-256，不复制文件。优先级为显式 `-Package` → `GENERATED_COMPONENT_SDK_ROOT` → 默认同级 `DesignToCode_GenerateComponent/build/sdk`。布局 Build.ps1 在 CMake 前验证 SDK，缓存绑定不同 SDK 根路径时拒绝继续；不提供组件源码回退。

SDK 位于 build 并不等于应提交二进制到源码库。开发仓库通过工作区 Source/Workspace.ps1 的 build 按依赖顺序恢复 SDK 和下游工具；外部 SDK 只读消费，缺失或不匹配时不能覆盖。离线发布另由 package 收集固定运行依赖和已验证二进制，消费者使用包内 Run.cmd，无需在上位机重建开发环境。

## 未映射处理

1. 生成器保存设计节点 ID、组件身份、属性和状态。
2. 映射表找不到身份或值域时，输出结构化未映射错误。
3. GenerateComponent 补充组件或映射，完成 Token 和组件测试。
4. 发布新版 SDK。
5. GenerateLayout 校验新版权威 SDK 并生成页面。
6. 页面测试验证 XML、组件调用和运行态。

这个顺序保证布局生成器只做解析与装配，组件能力始终来自同一发布源。

## 可替换目标库

组件生成与目标库声明分离：library-profiles.json 选择配置，componentLibrary 指向独立 JSON；classPrefix 只改变目标类和自动头文件名，默认 SDK 保持不变。contract-only/planned 不需要为测试而生成实现，也不能用于正式发布。解析器构建当前仍需要原 SDK；解耦目标映射不等于已消除所有构建依赖。

历史版本通过 Git 追溯，不再维护 docs/archive；当前机器报告归 output/logs。Python 构建和测试显式用已安装的 3.9；环境不可自主安装或更新，执行前遵循工作区 AGENTS.md。
