# Figma 统一组件融合规则

## 适用范围

本文用于将 Figma/Pixso 设计组件、变量和实例绑定到 GenerateComponent 的 Registry、Contract、Recipe 和 Qt 映射。

## 设计文件结构

组件集按以下区域组织：

```text
Design System
├── Components
├── Properties
├── States
├── Token Bindings
└── Engineering Mapping
```

设计组件集提供稳定的 componentSetId、componentKey、variantProperties 和实例属性。变量绑定保存变量 ID、变量名、Token 路径和适用模式。

## 命名

组件集名称使用 `ComponentLab/<Component>`。属性使用 lowerCamelCase；枚举值使用稳定英文值；布尔值使用 `true/false`。内部结构槽使用 `CONTAINER`、`LABEL`、`LEADING`、`TRAILING`、`CONTENT` 等固定名称。

## 融合步骤

1. 读取设计节点和变量原始证据。
2. 记录 fileKey、pageId、nodeId、componentSetId 和 componentKey。
3. 将设计属性转换为 Registry 轴或 Contract 属性。
4. 将变量绑定转换为 Token 路径。
5. 将组件身份绑定到 Qt 类和 SDK 公开 API。
6. 生成 ComponentLab 展示数据和 SDK 映射资源。

## 证据

原始 DSL、变量读取结果和标准化合同保存来源、采集时间、输入哈希和工具版本。模型推断的字段单独标记为 derived；它们不能覆盖原始设计身份。

## 资产缺失

原资产无法读取时，可以根据已确认的组件合同建立本地重建输入。文件中必须标记来源为 reconstructed，并列出缺失证据。重建内容需要经过设计侧确认后才进入正式 Binding。

## 验收

设计组件、变量、Token、Qt 类和 SDK manifest 逐项核对。组件身份唯一、属性值域明确、状态完整、公开头文件存在、组件能够独立消费后，才进入发布流程。
