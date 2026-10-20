# ComponentKit Source SDK

这是 ComponentKit 的源码级 CMake 包，用于在组件生产流程中编译 `GeneratedComponentSDK`。业务工程使用发布后的 `build/sdk`，不直接引用这个目录。

## 使用

```cmake
add_subdirectory(Source/ComponentKit/source-sdk)
target_link_libraries(app PRIVATE GeneratedComponentSDK::Components)
```

公开头文件位于 `include/MyDesignSystem`。组件源码、Token 资源和 QSS 由 ComponentKit 的构建规则登记。

## 与发布 SDK 的关系

Source SDK 是编译输入；`build/sdk` 是经过 Token 检查、编译、manifest 校验和独立消费测试后的发布包。两者的文件清单和来源指纹由发布器核对。
