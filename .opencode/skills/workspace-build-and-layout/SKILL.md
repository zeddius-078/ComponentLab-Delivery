---
name: workspace-build-and-layout
description: Use for Pixso to Qt UI generation, capture/adapt/compile, first checkout, missing SDK or tool errors, building ComponentLab Delivery, or offline toolkit delivery. Routes to the maintained workspace and project Main Skills instead of reconstructing the pipeline.
---

# 工作区构建与页面生成入口

1. 读取工作区 `AGENTS.md`、`README.md`、`docs/mainSkill/design-to-qt-workspace/SKILL.md`。
2. 页面生成再读 `DesignToCode_GenerateLayout/docs/mainSkill/generate-layout-pipeline/SKILL.md` 和其指定参考。
   先按用户目标区分“结构预览＋初始视觉”和“正式组件集成”。前者使用维护的 PreviewStructure.py（同一编译器的 structure-preview 分支），不要求完整页面组件合同；固定画布、占位和视觉差异必须披露。原有 Preview/qt-native 仍按合同校验。Commerly 只作样例，不把其名称或结构写入通用规则。
3. 判断是源码开发还是完整离线包。开发走 `Source/Workspace.ps1` 的 doctor → build → verify；使用包走 `Run.cmd`。不得要求离线用户构建 SDK。
4. 固定 Python 3.9，环境缺失一次列全，不自行安装、升级或回退较新 Python。
5. 不自行拼替代脚本、伪造布局证据或绕过缺组件。已有 D2C 返回包通过维护的 import_d2c.py 落地，再走同一个 adapter/compiler。
6. Capture PASS、Adapt PASS、Generate PASS、Visual PASS 分别报告，未产出 UI 不能称为生成完成。
7. build 是受控工具/SDK/runtime，不允许普通清理；临时产物进 output/work，交付进 output/delivery，日志进 output/logs。clean 默认只清 work/logs，指定交付批次才清 delivery。使用稳定工具不要求 VS/CMake，不从旧 cache 寻找 EXE。

工具定位是面向明确输入与配置的工程生成工具，不是“万能基准工具”。交付说明要限定实际适用范围；结构占位、初始样式和正式实现分开标注，不把局部演示扩大为通用还原承诺。

结构预览中的未知组件按已落实的规则生成：原生 QWidget +
Pixso 节点名规范化后的 objectName + ID 稳定后缀。保留 designNodeName 和源身份，
继续解析子树；不猜开发类、不生成假的自定义组件。实现位于布局项目
Source/HtmlDsl/pipeline/compiler/StructurePreview.h，由 PreviewStructure.py 调用，
不改变正式一对一映射合同。

入口参数、离线依赖清单和排错见工作区 `docs/tutorial/三项目上手指南.md`、`docs/tutorial/离线交付.md`、`docs/designRules/architecture/工具与交付依赖.md`。这里只分流，不另维护一套实现规则。
