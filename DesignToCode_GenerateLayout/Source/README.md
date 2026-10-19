# GenerateLayout 源码

本目录只保存当前布局生成器源码、构建入口和校验工具。

硬约束：`Source/` 不保存任何生成产物、构建缓存、页面包或报告。预览、编译、交付和验证结果统一写入同级 `build/`。

- 组件 Token、组件 Contract、Recipe 和生成规则由 `DesignToCode_GenerateComponent/Source/` 维护。
- 输入夹具统一位于项目级 `docs/fixtures/layout`，不得由生成器回写。
- 历史资料进入项目级 `docs/archive/`。
- 固定样例、模板、字体和验证代码进入 `docs/examples`、`docs/fixtures`、`docs/validation`；生成器只读取显式输入，不回写。
- `build` 随时可删除；预览进入 `build/preview`，正式页面包进入 `build/delivery`，编译缓存进入 `build/cache`，验证报告进入 `build/reports`。
- `Build.ps1`、CMake 配置、采集和校验脚本本身是维护源码，必须留在 `Source`，不能放进可删除的 `build`。
