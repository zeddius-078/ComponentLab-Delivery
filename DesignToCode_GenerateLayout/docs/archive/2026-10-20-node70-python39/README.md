# Pixso 70:1 / Python 3.9 构建与还原记录

结论：实时采集、正式页面生成、uic、隔离编译、运行与截图链路均已执行；**视觉验收不通过，不应作为还原完成的交付包发布**。本轮没有修改组件实现或解析器源码，没有安装、升级环境。

## 输入与环境

- 来源：https://pixso.cn/app/design/4S0Ktm81WnvzuJmZHvRuOg?item-id=70:1
- 节点：工单列表，1034 × 632；实时 eval_script 核对 fileKey 与目标一致。
- Python：`C:/Users/ZYC/AppData/Local/Programs/Python/Python39/python.exe`，3.9.13；全部 Python 采集、验证脚本均显式使用此解释器和 `-B`。
- 原生编译：既有 Qt 5.15.2 / MSVC / CMake / Ninja。Python 驱动采集和验证，不代替 C++ 编译器。
- SDK：兄弟项目已发布 `build/sdk`，校验通过；没有重新开发组件。
- SDK manifest SHA-256：`32067cb01e252acd6fb25a29f4e3838eb2de317b3b0c7bc9a965e1398b370047`。
- DSL SHA-256：`3b8304d8f7141ce8f3c6bfc40dfe6301759d03b782425cc315859f61f018793c`；与本机既有同节点快照一致，但本轮重新进行了实时采集。
- 未取得远端不可变 revision ID；证据为同次采集的 HTML、DSL、布局属性、截图、时间及哈希，不冒充服务端版本签名。

## 已执行链路

1. 阅读 Main Skill 与 Pixso D2C Skill；使用 HTML 中间层，再由现有 Qt 适配器生成，不手写替代 UI。
2. 桌面连接工具未返回，直接通过项目 `capture_mcp.py` 调用已配置的 `127.0.0.1:3667/mcp` 成功。没有更改服务配置。
3. design_to_code 获取 HTML，保存原始正文并下载 SVG、字体；完整 get_node_dsl 保留实例；只读 eval_script 获取 11 个 FRAME 的布局证据；获取节点截图。
4. 创建并验证 capture manifest、原始 DSL 逐字节校验。Node ID 是合并键，保留原始 HTML / DSL，不删除中间 HTML 的映射 ID。
5. 从当前 docs 映射生成独立 `wide-light` 副本，运行 `Source/HtmlDsl/Generate.ps1 -Formal`。设计 Profile 为 light-compact，运行 Profile 为 wide-light。
6. 正式包结构/哈希验证：5 个文件、54 个 widget/layout 对象；uic 通过。
7. 仅复制页面包、已发布 SDK 和测试用字体到独立消费者目录；CMake AUTOUIC/AUTORCC 编译链接通过，未链接布局生成器源码。
8. 原生运行测试 `PAGE_TESTS_PASS`：公开访问器、禁用状态、按钮信号及嵌套布局检查。它不是完整业务验收，也不是视觉验收。
9. 加载采集字体后，1034 × 632 离屏渲染并检查截图及组件几何。
10. Python 3.9 映射/结构测试 26/26；C++ 组件/布局测试 2/2；最终项目目录检查与 git diff --check 通过。

第一次单独运行 Python 回归时没有在该进程初始化 Qt DLL 路径，原生子进程报 3221225781；随后通过现有 NativeTools 初始化当前进程后 26/26 通过。没有修改全局 PATH，也没有安装依赖。

## 未通过项及定位

| 问题 | 直接证据 | 当前判断 |
| --- | --- | --- |
| 两行按钮变成一行 | 原始节点 `70:18` 的 `autoLayout.stackMode=GRID`；截图为 2 × 5；HTML 为 flex row | `Source/HtmlDsl/tools/adapt_v2.cjs:128` 用 HTML 方向覆盖 DSL stackMode，网格语义丢失；属于布局链路问题 |
| 差异未阻止生成 | 第二排 5 个节点 HTML/DSL 的最大坐标差均为 510 px，adapter 仍返回 PASS | 当前成功状态只说明转换执行成功，缺少布局语义冲突阻断；不能用它证明还原正确 |
| Qt 尺寸与字体不一致 | 返回按钮设计 86 × 32，Qt 实测 106 × 40；Qt 按钮字体属性 Helvetica / 18 px；标题则为 HarmonyOS Sans SC / 24 px | 已加载设计字体，仍有组件级差异；SDK Profile、Token 与运行时尺寸应用根因待专项核对，不在本轮私自修改组件 |

Qt 截图可见按钮挤压、裁切及文字差异，因此 visualAcceptance=FAIL。后续应先保留 GRID 及行列信息、阻止 HTML/DSL 语义冲突通过，再核对 SDK 组件尺寸和字体；禁止靠手改生成的 `.ui` 或覆盖原始设计值掩盖问题。

原始采集审计报告中的临时 URL 错误来自刻意保留的 raw HTML 和清单。离线 HTML 单文件复核为 0 error、2 warning；warning 是 Node ID / 生成 class，作为合并中间层有意保留。生成器自身会剥除缺失 animation.js 并处理重复 const，记录原始脚本错误；不改原始采集。

## 产物与重跑入口

- 页面包：`build/delivery/node70-python39-20261020/ticketListPageV3/`。
- UI：`form/TicketListPageV3.ui`；其余为 include、src、view/res 下的 h/cpp/qss/qrc。
- 报告：`build/reports/formal-delivery/node70-python39-20261020/`。
- 隔离消费者：`build/cache/consumer-node70-python39-20261020/`。
- 原始采集：`build/captures/node-70-1-{d2c,dsl,evidence,mapping}-20261020/`。
- 本轮运行脚本：报告目录 `run-consumer.ps1`；采集与准备脚本在 `build/captures`。
- 正式生成入口拒绝覆盖既有输出；重跑时更换 Batch，不能复用原目录直接覆盖。
- `evidence.zip` 冻结输入、映射、页面包、报告、截图和消费者测试源码；不重复归档 SDK 二进制。

目录清理：一个创建于本轮之前的 `Source/HtmlDsl/pipeline/capture/__pycache__/output_paths.cpython-39.pyc` 已移动到 `build/cache/preexisting-python-cache-20261020/`，可恢复；没有删除维护源码。其他原有未提交修改保持不动。
