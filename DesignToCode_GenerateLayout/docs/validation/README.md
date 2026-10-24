# 验证代码

本目录保存可重复运行的测试实现。机器结果不写回 docs；通用检查写入 `output/logs`，第二路径的隔离消费者结果、截图和日志写入其明确指定的 `output/work/pixso-native/<批次>`。

| 路径或文件 | 检查内容 |
| --- | --- |
| `component` | 组件映射与 SDK 消费 |
| `layout` | Box、Grid、Form、结构预览和 Profile |
| `formal` | 正式采集、生成和页面交付 |
| `v2` | V2 示例行为 |
| `pixso_native` | 独立 Vue 静态解析、组件/布局转换、资源及真实页面消费者读回；不调用第一路径编译器 |
| `test_fixture_integrity.py` | 固定输入、字节和哈希 |
| `test_independence.py` | 项目独立性与 SDK 引用 |
| `test_output_boundary.py` | 输出路径边界 |
| `check_delivery.py` | 页面交付检查 |

Python 测试使用固定的 Python 3.9。根目录的 discover 不会自动覆盖这些没有 Python 包标记的子目录，必须分组执行并分别记录结果：

```powershell
py -3.9 -B -m unittest discover -s docs/validation -v
if ($LASTEXITCODE -ne 0) { throw '顶层验证失败' }
py -3.9 -B -m unittest discover -s docs/validation/layout -v
if ($LASTEXITCODE -ne 0) { throw '第一路径布局验证失败' }
py -3.9 -B -m unittest discover -s docs/validation/formal -v
if ($LASTEXITCODE -ne 0) { throw '第一路径正式合同验证失败' }
# 第二路径的 QT_ROOT 和解析工具按其说明显式选择，不自动补装。
py -3.9 -B -m unittest discover -s docs/validation/pixso_native -v
if ($LASTEXITCODE -ne 0) { throw '第二路径验证失败' }
```

Qt、正式生成、消费者编译和视觉检查需要相应工具与 SDK。缺少依赖时记录为未执行，不能将旧日志作为本次通过证据。

formal 的 NTFS 联接测试会复制所选编译器 EXE。它的配套 DLL 和 Qt 插件由该测试的子进程环境显式指向原受控工具目录；只复制 EXE 后的 Windows 装载失败不能算作路径保护生效。测试不安装运行库，也不修改全局 PATH。

第二路径的 Python 与 Node 测试需要分别执行，命令见[第二路径说明](../Pixso原生DSM-D2C到Qt第二路径说明.md)。真实页面消费者检查必须提供 `--generation-record`；不传时为带附加控件的 API 探针，不能代表目标页面验收。配置切换测试不等于已有第二套真实 SDK，离屏运行不等于视觉一致性。
