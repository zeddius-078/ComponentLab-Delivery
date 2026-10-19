---
name: componentlab-gallery
description: 维护 G:/ComponentLab-Delivery/ComponentLab 的 Qt5 C++ 组件展示、状态示例、对象树、Profile 切换、SDK 消费和离线演示。用户要求新增组件页签、统一展示形式、修复演示交互或验证 GeneratedComponentSDK 时使用。
---

# ComponentLab Gallery

## 项目职责

本项目展示和验收已经发布的 `Z_*` 组件。允许创建展示页、示例数据、展示控制器和测试；组件实现、Token 编译器、QSS 模板及组件工厂由 GenerateComponent 负责。

## 开始前

1. 先读取工作区 AGENTS.md；不安装或更新 Python、Qt、编译器、依赖及系统配置，环境缺口先问用户。
2. 读取 `G:/ComponentLab-Delivery/ComponentLab/README.md`。
3. 读取 `docs/tutorial/组件展示上手指南.md`。
4. 检查 `../DesignToCode_GenerateComponent/build/sdk/manifest/component-sdk.json` 的发布者与版本。
5. 运行 `git status --short`，保留现有修改。

## 关键目录

| 路径 | 作用 |
| --- | --- |
| `Source/delivery/demo` | 组件导航、逐组件展示、对象树与交互日志 |
| `Source/delivery/tests` | 合同、组合、表单、目录和显示测试 |
| `Source/delivery/tools` | 构建、验证、清单和离线打包 |
| `Source/presentation` | 演示专用主题安装、图标样例和组合控制器 |
| `../DesignToCode_GenerateComponent/build/sdk` | 工作区唯一权威 SDK；ComponentLab 不保留副本 |
| `build/demo/component_gallery` | 可离线运行的展示程序 |
| `build/reports` | 测试、校验和视觉复核结果 |

## 修改流程

### 展示一个新组件

1. 在 SDK 公共头、注册表与实现中确认组件存在，并进行实际构造验证；不能仅凭登记认定可运行。
2. 在 `Source/delivery/demo` 为该组件建立独立展示页。
3. 展示默认、交互、禁用、焦点及组件特有状态。
4. 展示对象树和关键公开属性，使用真实 `Z_*` 类名。
5. 把组件加入导航树；每个页面只承担一个组件族。
6. 增加最小构造、状态切换和 Profile 测试。

### 发现 SDK 缺口

记录缺失的设计身份、公开 API、状态或 Token。修复位置是 `DesignToCode_GenerateComponent`；SDK 重新发布并校验后继续消费，不在 ComponentLab 定义临时替代类。

## 展示约束

- 导航名称表达 Z-DesignSystem 组件，不使用“Qt 原生组件”作为产品分类。
- 所有组件采用一致的展示结构：组件说明、状态控制、实时示例、对象树和交互结果。
- Profile 只切换权威 SDK 提供的 Token 资源。
- 演示代码可以组合组件，但不得新增组件生产能力。
- 展示用数据与业务数据分离，示例不写入组件合同。

## 状态与归档

- GenerateLayout 的 analysis-system 是未实现类名合同，不能加入本项目并伪装成真实组件。
- 当前导航栏 container.height Token 消费问题按用户决定挂起；缺口影响构造测试时如实报告，不临时造替代类。
- 历史测试数量、迁移过程和旧说明放 docs/archive；build/reports 只存当次可再生输出。
- Profile/Theme 以 SDK 发布资源为准，standard/wide 与 light/dark 组合成四上下文；Density 不等同于 Profile。

## 验证

```powershell
pwsh -File Source/delivery/tools/Build.ps1
python -B Source/delivery/tools/CheckProject.py
pwsh -File Source/delivery/tools/Verify.ps1
```

需要离线包时再运行：

```powershell
pwsh -File Source/delivery/tools/Package.ps1
```

完成条件：Demo 构建成功，项目检查通过，新增组件可从导航独立打开，状态控制有效，对象树使用真实 SDK 类型，运行包不依赖 GenerateComponent 源码目录。
