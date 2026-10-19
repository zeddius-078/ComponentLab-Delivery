# 固定样例与验证资产

本目录是维护输入，不是输出目录。Qt 示例源文件、instrument_host 模板和字体保留于此；样例执行产生的文件只能进入项目 `build`。

- 解析器构建：项目根执行 `pwsh -File Source/Build.ps1 -Test`。
- V2 演示：项目根执行 `pwsh -File docs/examples/Qt/V2/Build.ps1 -Test`。
- 单节点案例：`Qt/V2/Case-27-450/Build.ps1`。
- 历史 V1 包装入口：`Package-V1.ps1`。

`build` 不保留任何唯一维护文件；删除后以这里的固定输入和 `Source` 工具重建。Qt、MSVC、Node/Playwright 和外部权威组件 SDK 仍是显式环境依赖，不由删除 build 的测试替代验证。
