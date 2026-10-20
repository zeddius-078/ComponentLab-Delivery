# ComponentLab

本项目消费 GenerateComponent 发布的组件 SDK，展示组件外观、状态、公开属性和对象树。它不生成组件、Token、QSS 或第二套 SDK。

## 目录

| 路径 | 作用 |
| --- | --- |
| Source/delivery/demo | 维护的展示页、导航、对象树与交互示例 |
| Source/delivery/tests | 组件消费、状态与展示测试 |
| Source/delivery/tools | 构建、验证和打包工具 |
| Source/presentation | 演示主题与控制器 |
| docs | 指导、规则、Main Skill 与历史档案 |
| build/demo/component_gallery | 可再生展示程序 |
| build/cache、build/reports | 编译缓存、当次验证输出 |

SDK 默认来自同级 DesignToCode_GenerateComponent/build/sdk；外部工作区可显式设置 GENERATED_COMPONENT_SDK_ROOT。校验后直接引用，不复制 SDK，不引用兄弟项目私有实现。遵循工作区 AGENTS.md，不自主安装或升级环境。

## 构建与检查

在本项目根执行：

```powershell
pwsh -File Source/Verify-GeneratedComponentSDK.ps1
pwsh -File Source/delivery/tools/Build.ps1
python -B Source/delivery/tools/CheckProject.py
pwsh -File Source/delivery/tools/Verify.ps1
```

只有实际通过对应构造、交互和视觉检查的组件才能报告可用。目录项、Figma 变体数量和 JSON 登记不能替代运行证据；本项目不展示 analysis-system 的未实现类为“真实组件”。

运行上下文为 standard-light、standard-dark、wide-light、wide-dark；组件密度与上下文分开。外部完整 Token 目录通过 --token-dir 用于验证，不修改 SDK 发布包。

相关失败须按当次实测报告，不在 Demo 中制作替代实现。阶段数量、旧截图和迁移过程统一保存在 [历史归档](docs/archive/README.md)，不再作为当前完成结论。

详见 [上手指南](docs/tutorial/组件展示上手指南.md)、[Token 使用](docs/designRules/token/Token变量集使用.md)和 [文档中心](docs/README.md)。
