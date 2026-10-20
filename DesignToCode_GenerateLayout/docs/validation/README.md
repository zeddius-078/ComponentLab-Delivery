# 验证代码

本目录保存可重复运行的测试实现。测试文件是验证入口，运行结果写入 `output/logs`。

| 路径或文件 | 检查内容 |
| --- | --- |
| `component` | 组件映射与 SDK 消费 |
| `layout` | Box、Grid、Form、结构预览和 Profile |
| `formal` | 正式采集、生成和页面交付 |
| `v2` | V2 示例行为 |
| `test_fixture_integrity.py` | 固定输入、字节和哈希 |
| `test_independence.py` | 项目独立性与 SDK 引用 |
| `test_output_boundary.py` | 输出路径边界 |
| `check_delivery.py` | 页面交付检查 |

Python 测试使用固定的 Python 3.9：

```powershell
py -3.9 -B -m unittest discover -s docs/validation -v
```

Qt、正式生成、消费者编译和视觉检查需要相应工具与 SDK。缺少依赖时记录为未执行，不能将旧日志作为本次通过证据。
