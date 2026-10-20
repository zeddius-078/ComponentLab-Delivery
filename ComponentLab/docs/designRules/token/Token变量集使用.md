# ComponentLab 的 Token 变量集使用

## 来源

展示程序读取 GenerateComponent SDK 中的 Token 资源和 Profile。当前 Profile 名称为：

```text
standard-light
standard-dark
wide-light
wide-dark
```

Profile 组合设备宽度与明暗主题。组件密度由组件合同和布局属性决定，不从 Profile 名称推断。

## 选择方式

展示页通过 SDK 提供的主题或 Profile 接口选择变量集。页面代码记录当前 Profile，组件从 Token/QSS 和 C++ 组件实现读取颜色、字体、尺寸、圆角、状态和资源。

## 验收方式

选择同一组件的两个 Profile，检查：

1. 颜色和表面是否来自对应 Token。
2. 字体和组件高度是否符合当前 Profile。
3. hover、pressed、disabled、loading 和 focus 状态是否有对应视觉。
4. 对象树和公开注册信息是否保持一致。

展示截图用于记录结果。Token 的来源、编译和 SDK 完整性以 GenerateComponent 的 manifest 和生成检查为准。
