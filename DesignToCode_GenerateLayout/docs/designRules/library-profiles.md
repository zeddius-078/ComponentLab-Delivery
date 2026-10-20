# 组件库 Profile 与映射替换

## 作用

Profile 选择页面生成时使用的组件映射合同。配置文件为 `docs/fixtures/layout/mapping/library-profiles.json`，入口参数为 `-LibraryProfile`。

| Profile | 用途 | 运行条件 |
| --- | --- | --- |
| `token-sdk` | 正式页面，使用 GenerateComponent 发布的 SDK | SDK manifest、头文件、库和资源齐全 |
| `analysis-system` | 检查类名和映射规则 | `contract-only/planned`，只用于映射测试和预览 |

## 按组件名替换

映射条目以稳定设计身份为键。`componentOverrides` 做单个组件替换，`classRenames` 做一组类名前缀替换。替换保留 canonical ID、Qt 基类、属性和值域；只改变目标库的实现名称或头文件路径。

```json
{
  "componentOverrides": {
    "ComponentLab/Button": {"className":"commonWidget::Z_PushButton"}
  },
  "classRenames": {"commonWidget::Z_":"analysisWidget::A_"}
}
```

校验顺序是：组件身份、属性合同、类名、头文件、SDK 状态、生成和消费者编译。任何条目缺失都停止，不把未知类降级为另一个组件。

## 限制

`analysis-system` 没有可运行 SDK 时，Qt 基类测量或 XML 解析通过只代表映射合同成立。正式模式拒绝该 Profile。布局类、Spacer 和页面容器继续使用 Qt 原生 XML，不按组件类名前缀替换。

## 验证

```powershell
py -3.9 -B -m unittest discover -s docs/validation/layout -p test_library_profiles.py -v
pwsh -File Source/HtmlDsl/Generate.ps1 -Preview -LibraryProfile token-sdk ...
```

预览和正式入口都必须显式传入捕获清单、映射目录和字体。Profile 不改变 HTML、DSL 或布局定位数据。
