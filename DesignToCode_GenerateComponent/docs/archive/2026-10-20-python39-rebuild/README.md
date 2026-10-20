# Python 3.9 构建刷新记录

日期：2026-10-20。执行范围：刷新维护输入副本、完整生成、SDK 编译发布、独立消费与目录边界回归。不安装或更新环境，不修复已挂起的导航 Token 合同。

## 环境与执行

使用已有 Python 3.9.13：`C:\Users\ZYC\AppData\Local\Programs\Python\Python39\python.exe`。Qt 使用 `D:\QT\5.15.2\msvc2019_64`，CMake/Ninja 使用 `D:\QT\Tools`，MSVC 使用已有 `D:\VS`。仅设置执行进程环境，不修改全局 PATH 或系统默认解释器。

依次执行 `py -3.9 -B Source/PrepareBuild.py --copy-only`、不带 copy-only 的生成、`Source/Publish-ComponentSDK.ps1`、`docs/validation/SDKConsumerTest/Build.ps1`、Python 回归及 `Source/CheckProject.py`。

| 检查 | 实际结果 |
| --- | --- |
| build/work 刷新及完整生成 | 通过；406 个维护输入的源文件、副本和 input-manifest 哈希全部一致 |
| CMake Python 选择 | 缓存从原 Python 3.13 切换为明确的 Python 3.9.13；版本范围 3.9 至小于 3.10 |
| Token 生成与一致性检查 | 通过；有设计容量报告 639 与当前 773 不一致的非阻断警告 |
| SDK 编译与发布 | 通过；build/sdk 更新，发布清单哈希校验通过 |
| 独立消费编译 | 通过 |
| 独立消费运行 | 失败：缺少 component.navigation-bar.base.state.default.slot.container.height |
| Python 写入及源码边界测试 | 6/6 通过，真实 Python 3.9.13 |
| 项目目录检查 | PASS |
| 执行前后维护输入比较 | 归档写入前，990 个 Source/docs 文件内容及集合未变 |

生成器打印的 59 个原生条目、110 个公共条目和 105 个组件身份是生成统计，不构成全部组件可实例化或视觉验收证明。新增产物是本轮重建产物，但独立消费门禁仍失败，不能标记为可验收发布。

## 产物、证据与回退

- 当前源码生成副本：`build/work/Source/ComponentKit`。
- 当前 SDK：`build/sdk`；当前 CMake 缓存：`build/cache/GeneratedComponentSDK`。
- 运行日志：`build/reports/python39-refresh`；消费运行记录：`build/reports/sdk-consumer/ctest.xml`。
- `evidence.zip` 冻结本轮日志、输入基线和构建缓存；不依赖 build 永久保留。
- 发布前 SDK 由原发布入口保留于 `build/cache/previous-sdk-2f8913c8b2854b4885fe3060abca6067`。如需回退须先核对后续修改并完整恢复包，不能混用新头文件与旧静态库；旧包同样不代表导航 Token 问题已解决。

本轮没有修改 Token 源、组件实现或安装环境。挂起问题保留给后续专门处理。
