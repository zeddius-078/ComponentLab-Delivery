# Commerly 映射探针

## 用途

本目录保存 Commerly 设计输入的身份核对结果和失败关闭配置。它用于检查组件合同，不作为通用默认映射，也不发布可运行组件。

设计文件：`HUTs4r6S0Eb3QzdijQyAzA`，页面：`0:1`。配置文件保留组件集、变体、Boolean 属性和直接实例的可追溯信息。

## 当前输入

`component-mapping.json` 的规则为空时，编译器会返回 `UNMAPPED_COMPONENT`。这表示当前配置没有登记可用组件身份。`engineering-names.json` 为空表示页面对象名尚未建立稳定表。

组件内容开关、图标槽位、标签可见性和业务组合卡片需要完整设计合同后再登记。仅填入目标类名不会补齐属性、资源和状态字段。

## 执行

```powershell
py -3.9 -B -m unittest discover -s docs/validation/layout -p test_commerly_mapping.py -v
```

测试输入为合同探针。测试通过表示拒绝路径和错误信息稳定，不表示整页组件映射、视觉还原或 SDK 发布已经完成。
