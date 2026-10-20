# Pixso 设计侧节点命名规范

## 目标

命名规则为页面节点提供稳定身份、可读名称和可生成的 Qt `objectName`。设计名称、canonical ID 和节点 ID 同时保留。

## 设计侧字段

| 字段 | 作用 |
| --- | --- |
| `name` | 设计稿中的可读名称 |
| `componentRef` | 组件合同或母版身份 |
| `variantProperties` | 设计变体和状态 |
| `nodeId` | 当前文件中的稳定节点引用 |
| `parentId` / `children` | 页面层级 |

组件属性使用 `lowerCamelCase`，枚举使用稳定英文值，布尔值使用 `true/false`。固定槽名包括 `LABEL`、`LEADING`、`TRAILING`、`CONTENT`、`CONTROL`、`HINT`、`PREFIX` 和 `SUFFIX`。

## Qt 输出名称

已登记节点使用 `engineering-names.json` 中的名称。未登记节点生成 `node_<规范化名称>_<稳定后缀>`；空名、重名、C++ 保留字和无法转为 ASCII 标识符的名称直接报告错误。原始名称写入 `designNodeName` 和结构报告，不作为 C++ 类名。

名称规则只解决身份映射。组件是否可生成仍由 SDK manifest 和组件合同决定。
