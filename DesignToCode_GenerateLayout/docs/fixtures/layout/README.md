# 布局生成固定输入

| 目录 | 内容 |
| --- | --- |
| `design-inputs` | DSL 与设计节点数据 |
| `html-dsl` | HTML、DSL、capture manifest 和页面资源 |
| `Case-27-450` | 节点案例输入 |
| `mapping` | 公共组件、布局和 Profile 配置 |
| `mapping-commerly` | Commerly 专用映射探针 |
| `structure` | 结构预览输入 |

文件来源、设计版本、根节点、关联资源和哈希由 capture manifest 表达。`structure` 与 `html-dsl` 是两种输入合同，分别用于结构预览和正式 HTML + DSL 路线。

固定输入只读。一次执行产生的采集先写入 `output/work`，长期回归输入经过核对后再复制到本目录。
