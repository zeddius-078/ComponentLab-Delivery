# Pixso 设计侧节点命名规范

## 目标

设计侧节点名是页面对象名的单一事实源。开发侧 objectName 直接对齐设计侧规范名；手工登记表只兜例外，不承担全量命名。

## 命名规则（设计侧）

1. 页面内所有业务节点（FRAME / INSTANCE / PARAGRAPH）使用稳定英文标识，lowerCamelCase，只允许 `^[a-z][A-Za-z0-9]*$`。
   - 正例：`workOrderInput`、`confirmReviewButton`、`table1`、`returnButton`
   - 反例：`实例 1`、`Frame 3`、`Rectangle 12`、`按钮-2`
2. 语义优先：按组件角色命名（返回 / 确认 / 提交 / 选项 / 表单），不按图层序号或坐标命名。
3. 页内唯一：同一页面不允许重名；也不与同页其他节点产生的对象名冲突。
4. 避开 C++ 保留字与 Qt 关键字：`signals`、`slots`、`emit`、`class`、`int` 等一律不可用。
5. 命名稳定：节点改名视为合同变更，需同步验证生成结果；不使用 Pixso 自动生成的占位名。

## 开发侧消费优先级

| 优先级 | 来源 | 行为 |
| --- | --- | --- |
| 1 | `engineering-names.json` 登记条目 | 原样使用（历史页面、重命名例外、语义补充） |
| 2 | 设计侧规范 ASCII 名 | 自动 lowerCamelCase 对齐为 objectName |
| 3 | 无规范名（中文、占位名、空名） | **退化为类名加数字**，并记录退化证据 |

## 退化规则（类名加数字）

1. 触发条件：节点设计名不是规范英文标识（含中文、Pixso 自动占位名、空名、非法字符）。
2. 退化结果：`<类基名 lowerCamelCase> + <页内同类序号>`，序号按文档顺序从 1 开始。
   - 组件实例：取设计组件基名。`ComponentLab/Button` → `button1`、`button2`；`ComponentLab/SelectOption` → `selectOption1`。
   - PARAGRAPH 文本节点：`text1`、`text2`。
   - FRAME 容器：`container1`、`container2`。
3. 稳定性：基名取**设计组件名**而非 Qt 类名，保证同一设计在 token-sdk / qt-native 等不同映射下对象名一致。
4. 冲突处理：退化名与规范名或登记名冲突时生成失败并输出明确错误，不得自动重编号掩盖冲突。
5. 可追溯：每次退化在生成记录中登记 `{nodeId, designName, degradedName, reason}`，包外报告保留证据，不静默丢弃。

## 校验

- 采集后、生成前校验页面节点命名：能推导的规范名直接对齐；触发了退化的节点必须出现在生成记录的退化清单里。
- 生成器仍保持失败关闭：重名、保留字、非法字符导致的不可解析命名立即失败。

## 与既有规则的关系

- 取代此前“中文未映射直接失败”（`ENGINEERING_NAME_MAPPING_REQUIRED`）对**占位/无语义名**的处理：现在允许退化，但必须留痕。
- “不把节点 ID 拼进业务对象名”仍有效：退化名不含节点 ID。
- 登记表（engineering-names.json）保留，用于语义命名与历史页面，不再要求每个页面逐节点登记。
