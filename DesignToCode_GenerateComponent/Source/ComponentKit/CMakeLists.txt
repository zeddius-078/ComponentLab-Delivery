cmake_minimum_required(VERSION 3.21)
project(DesignToCodeComponentKit LANGUAGES CXX)
if(MSVC AND DEFINED BUTTON_MSVC_INCLUDE_PREFIX)
    set(CMAKE_CL_SHOWINCLUDES_PREFIX "${BUTTON_MSVC_INCLUDE_PREFIX}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets Svg)
option(D2C_BUILD_TOKEN_TOOLS "Build the private DesignToCode Token pipeline" OFF)
option(GENERATED_COMPONENT_SDK_INSTALL "Install the productized component SDK" ON)
file(GLOB GENERATED_NATIVE_COMPONENT_SOURCES CONFIGURE_DEPENDS
    runtime/component/native/Z_*.cpp runtime/component/native/Z_*.h)
set(NATIVE_COMPONENT_REGISTRY_SOURCES
    runtime/component/native/NativeComponentRegistry.cpp
    runtime/component/native/NativeComponentRegistry.h)

# 所有实现来自本目录的独立版本，不搜索或回退到 ComponentLab。
add_library(d2c_components STATIC
    ${GENERATED_NATIVE_COMPONENT_SOURCES}
    ${NATIVE_COMPONENT_REGISTRY_SOURCES}
    runtime/component/DesignComponentCatalog.cpp runtime/component/DesignComponentCatalog.h
    runtime/component/LabelAppearance.cpp runtime/component/LabelAppearance.h
    runtime/component/ProgressAppearance.cpp runtime/component/ProgressAppearance.h
    runtime/component/RadioAppearance.cpp runtime/component/RadioAppearance.h
    runtime/component/SpinBoxAppearance.cpp runtime/component/SpinBoxAppearance.h
    runtime/component/DateTimeAppearance.cpp runtime/component/DateTimeAppearance.h
    runtime/component/SliderAppearance.cpp runtime/component/SliderAppearance.h
    runtime/component/ScrollBarAppearance.cpp runtime/component/ScrollBarAppearance.h
    runtime/component/DialAppearance.cpp runtime/component/DialAppearance.h
    runtime/component/LineEditAppearance.cpp runtime/component/LineEditAppearance.h
    runtime/component/ShortcutAppearance.cpp runtime/component/ShortcutAppearance.h
    runtime/component/ComboBoxAppearance.cpp runtime/component/ComboBoxAppearance.h
    runtime/component/CommandLinkAppearance.cpp runtime/component/CommandLinkAppearance.h
    runtime/component/ToolButtonAppearance.cpp runtime/component/ToolButtonAppearance.h
    runtime/component/DialogButtonBoxAppearance.cpp runtime/component/DialogButtonBoxAppearance.h
    runtime/component/TabAppearance.cpp runtime/component/TabAppearance.h
    runtime/component/ToolBoxAppearance.cpp runtime/component/ToolBoxAppearance.h
    runtime/component/DockWidgetAppearance.cpp runtime/component/DockWidgetAppearance.h
    runtime/component/MdiAppearance.cpp runtime/component/MdiAppearance.h
    runtime/component/NavigationAppearance.cpp runtime/component/NavigationAppearance.h
    runtime/component/PageFlowAppearance.cpp runtime/component/PageFlowAppearance.h
    runtime/component/Z_PushButton.cpp runtime/component/Z_PushButton.h
    runtime/component/Z_CheckBox.cpp runtime/component/Z_CheckBox.h
    runtime/component/Z_FoundationComponents.cpp runtime/component/Z_FoundationComponents.h
    runtime/component/Z_DesignComponents.cpp runtime/component/Z_DesignComponents.h
    runtime/component/Z_InputField.cpp runtime/component/Z_InputField.h
    runtime/component/Z_Select.cpp runtime/component/Z_Select.h
    runtime/component/Z_OtpInput.cpp runtime/component/Z_OtpInput.h
    runtime/component/IconAssetCatalog.cpp runtime/component/IconAssetCatalog.h
    runtime/component/VisualAssetCatalog.cpp runtime/component/VisualAssetCatalog.h
    runtime/component/IconSource.cpp runtime/component/IconSource.h
    runtime/component/FocusIndicator.cpp runtime/component/FocusIndicator.h
    runtime/surface/Surface.cpp runtime/surface/Surface.h
    runtime/surface/FieldChrome.cpp runtime/surface/FieldChrome.h
    runtime/surface/SolidInnerShadow.cpp runtime/surface/SolidInnerShadow.h
    runtime/surface/ProfileContext.cpp runtime/surface/ProfileContext.h
    runtime/surface/Typography.cpp runtime/surface/Typography.h
    runtime/surface/TokenResources.cpp runtime/surface/TokenResources.h)
target_include_directories(d2c_components PUBLIC runtime/component runtime/component/native runtime/surface)
target_link_libraries(d2c_components PUBLIC Qt5::Widgets Qt5::Svg)
target_compile_features(d2c_components PUBLIC cxx_std_17)
if(MSVC)
    target_compile_options(d2c_components PUBLIC /utf-8 PRIVATE /W4 /permissive-)
    if(MSVC_VERSION GREATER_EQUAL 1951 AND Qt5Core_VERSION VERSION_EQUAL 5.15.2)
        target_compile_options(d2c_components PUBLIC "/FI${CMAKE_CURRENT_SOURCE_DIR}/qt5152_msvc_compat.h")
    endif()
endif()

# GenerateComponent publishes the only component implementation consumed by the two demos.
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(GENERATED_COMPONENT_SOURCES
    runtime/component/DateTimeAppearance.cpp runtime/component/DateTimeAppearance.h
    runtime/component/SpinBoxAppearance.cpp runtime/component/SpinBoxAppearance.h
    runtime/component/SliderAppearance.cpp runtime/component/SliderAppearance.h
    runtime/component/ScrollBarAppearance.cpp runtime/component/ScrollBarAppearance.h
    runtime/component/DialAppearance.cpp runtime/component/DialAppearance.h
    runtime/component/LineEditAppearance.cpp runtime/component/LineEditAppearance.h
    runtime/component/ShortcutAppearance.cpp runtime/component/ShortcutAppearance.h
    runtime/component/ComboBoxAppearance.cpp runtime/component/ComboBoxAppearance.h
    runtime/component/CommandLinkAppearance.cpp runtime/component/CommandLinkAppearance.h
    runtime/component/ToolButtonAppearance.cpp runtime/component/ToolButtonAppearance.h
    runtime/component/DialogButtonBoxAppearance.cpp runtime/component/DialogButtonBoxAppearance.h
    runtime/component/TabAppearance.cpp runtime/component/TabAppearance.h
    runtime/component/ToolBoxAppearance.cpp runtime/component/ToolBoxAppearance.h
    runtime/component/DockWidgetAppearance.cpp runtime/component/DockWidgetAppearance.h
    runtime/component/MdiAppearance.cpp runtime/component/MdiAppearance.h
    runtime/component/LabelAppearance.cpp runtime/component/LabelAppearance.h
    runtime/component/ProgressAppearance.cpp runtime/component/ProgressAppearance.h
    runtime/component/RadioAppearance.cpp runtime/component/RadioAppearance.h
    runtime/component/NavigationAppearance.cpp runtime/component/NavigationAppearance.h
    runtime/component/PageFlowAppearance.cpp runtime/component/PageFlowAppearance.h
    runtime/component/ComponentFactory.cpp runtime/component/ComponentFactory.h
    runtime/component/DesignComponentCatalog.cpp runtime/component/DesignComponentCatalog.h
    runtime/component/native/NativeComponents.h
    ${NATIVE_COMPONENT_REGISTRY_SOURCES}
    ${GENERATED_NATIVE_COMPONENT_SOURCES}
    runtime/component/Z_PushButton.cpp runtime/component/Z_PushButton.h
    runtime/component/Z_CheckBox.cpp runtime/component/Z_CheckBox.h
    runtime/component/Z_FoundationComponents.cpp runtime/component/Z_FoundationComponents.h
    runtime/component/Z_DesignComponents.cpp runtime/component/Z_DesignComponents.h
    runtime/component/Z_InputField.cpp runtime/component/Z_InputField.h
    runtime/component/Z_Select.cpp runtime/component/Z_Select.h
    runtime/component/Z_OtpInput.cpp runtime/component/Z_OtpInput.h
    runtime/component/IconAssetCatalog.cpp runtime/component/IconAssetCatalog.h
    runtime/component/VisualAssetCatalog.cpp runtime/component/VisualAssetCatalog.h
    runtime/component/IconSource.cpp runtime/component/IconSource.h
    runtime/component/FocusIndicator.cpp runtime/component/FocusIndicator.h
    runtime/surface/Surface.cpp runtime/surface/Surface.h
    runtime/surface/FieldChrome.cpp runtime/surface/FieldChrome.h
    runtime/surface/SolidInnerShadow.cpp runtime/surface/SolidInnerShadow.h
    runtime/surface/ProfileContext.cpp runtime/surface/ProfileContext.h
    runtime/surface/Typography.cpp runtime/surface/Typography.h
    runtime/surface/TokenResources.cpp runtime/surface/TokenResources.h
    runtime/theme/ComponentTheme.cpp runtime/theme/ComponentTheme.h
    runtime/designer/DesignerWidgets.cpp runtime/designer/DesignerWidgets.h
    runtime/adapter/ButtonAppearanceAdapter.cpp runtime/adapter/ButtonAppearanceAdapter.h
    runtime/component-resources.qrc)
add_library(generated_component_sdk STATIC ${GENERATED_COMPONENT_SOURCES})
set_target_properties(generated_component_sdk PROPERTIES
    OUTPUT_NAME GeneratedComponentSDK)
target_include_directories(generated_component_sdk PUBLIC
    runtime/component runtime/component/native runtime/surface runtime/theme runtime/designer runtime/adapter)
target_link_libraries(generated_component_sdk PUBLIC Qt5::Widgets Qt5::Svg)
target_compile_features(generated_component_sdk PUBLIC cxx_std_17)
if(MSVC)
    target_compile_options(generated_component_sdk PUBLIC /utf-8 PRIVATE /W4 /permissive-)
    if(MSVC_VERSION GREATER_EQUAL 1951 AND Qt5Core_VERSION VERSION_EQUAL 5.15.2)
        target_compile_options(generated_component_sdk PUBLIC "/FI${CMAKE_CURRENT_SOURCE_DIR}/qt5152_msvc_compat.h")
    endif()
endif()
if(GENERATED_COMPONENT_SDK_INSTALL)
    install(TARGETS generated_component_sdk ARCHIVE DESTINATION lib)
    install(DIRECTORY runtime/component/ runtime/surface/ runtime/theme/ runtime/designer/
        DESTINATION include/MyDesignSystem FILES_MATCHING PATTERN "*.h")
    install(FILES runtime/adapter/ButtonAppearanceAdapter.h MyDesignSystem.h qt5152_msvc_compat.h
        DESTINATION include/MyDesignSystem)
    install(DIRECTORY licenses/ DESTINATION licenses)
    install(FILES runtime/assets/fonts/HarmonyOSSans/LICENSE.txt
        DESTINATION licenses RENAME HarmonyOS-Sans-LICENSE.txt)
endif()
add_library(d2c_theme STATIC runtime/theme/ComponentTheme.cpp runtime/theme/ComponentTheme.h)
target_include_directories(d2c_theme PUBLIC runtime/theme)
target_link_libraries(d2c_theme PUBLIC d2c_components)
add_library(d2c_designer_widgets STATIC runtime/designer/DesignerWidgets.cpp runtime/designer/DesignerWidgets.h)
target_include_directories(d2c_designer_widgets PUBLIC runtime/designer)
target_link_libraries(d2c_designer_widgets PUBLIC d2c_theme)
add_library(d2c_appearance STATIC
    runtime/adapter/ButtonAppearanceAdapter.cpp runtime/adapter/ButtonAppearanceAdapter.h
    runtime/component/ComponentFactory.cpp runtime/component/ComponentFactory.h)
target_include_directories(d2c_appearance PUBLIC runtime/adapter runtime/component)
target_link_libraries(d2c_appearance PUBLIC d2c_components)

if(D2C_BUILD_TOKEN_TOOLS)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_executable(d2c_button_token_compiler design-assets/tools/token_compiler.cpp)
    add_executable(d2c_checkbox_token_compiler design-assets/tools/checkbox_token_compiler.cpp)
    add_executable(d2c_forms_token_compiler design-assets/tools/forms_token_compiler.cpp)
    foreach(COMPONENT button checkbox forms)
        target_link_libraries(d2c_${COMPONENT}_token_compiler PRIVATE Qt5::Core Qt5::Gui)
        if(MSVC)
            target_compile_options(d2c_${COMPONENT}_token_compiler PRIVATE /utf-8)
            if(MSVC_VERSION GREATER_EQUAL 1951 AND Qt5Core_VERSION VERSION_EQUAL 5.15.2)
                target_compile_options(d2c_${COMPONENT}_token_compiler PRIVATE "/FI${CMAKE_CURRENT_SOURCE_DIR}/qt5152_msvc_compat.h")
            endif()
        endif()
    endforeach()
    # 明确调用此目标才刷新私有 Token 产物；GUI 构建默认消费现有快照。
    add_custom_target(d2c_generate_tokens
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/design-assets/tools/export_tokens.py"
        COMMAND d2c_button_token_compiler "${CMAKE_CURRENT_SOURCE_DIR}/design-assets" "${CMAKE_CURRENT_SOURCE_DIR}/design-assets/generated/button"
        COMMAND d2c_checkbox_token_compiler "${CMAKE_CURRENT_SOURCE_DIR}/design-assets" "${CMAKE_CURRENT_SOURCE_DIR}/design-assets/generated/checkbox"
        COMMAND d2c_forms_token_compiler "${CMAKE_CURRENT_SOURCE_DIR}/design-assets" "${CMAKE_CURRENT_SOURCE_DIR}/design-assets/generated/forms"
        DEPENDS d2c_button_token_compiler d2c_checkbox_token_compiler d2c_forms_token_compiler
        COMMENT "Generate DesignToCode private Token outputs" VERBATIM)
    add_custom_target(d2c_check_tokens
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/design-assets/tools/export_tokens.py" --check
        COMMAND d2c_button_token_compiler "${CMAKE_CURRENT_SOURCE_DIR}/design-assets" "${CMAKE_CURRENT_SOURCE_DIR}/design-assets/generated/button" --check
        COMMAND d2c_checkbox_token_compiler "${CMAKE_CURRENT_SOURCE_DIR}/design-assets" "${CMAKE_CURRENT_SOURCE_DIR}/design-assets/generated/checkbox" --check
        COMMAND d2c_forms_token_compiler "${CMAKE_CURRENT_SOURCE_DIR}/design-assets" "${CMAKE_CURRENT_SOURCE_DIR}/design-assets/generated/forms" --check
        DEPENDS d2c_button_token_compiler d2c_checkbox_token_compiler d2c_forms_token_compiler
        COMMENT "Check DesignToCode private Token outputs" VERBATIM)
    add_custom_target(d2c_generate_native_components
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_native_components.py"
        DEPENDS d2c_generate_tokens
        COMMENT "Generate one Z_* source and QSS contract per Qt visual component" VERBATIM)
    add_custom_target(d2c_generate_design_component_catalog
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_design_component_catalog.py"
        COMMENT "Generate the Figma public component catalog" VERBATIM)
    add_custom_target(d2c_check_native_components
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_native_components.py" --check
        DEPENDS d2c_generate_native_components
        COMMENT "Check generated Z_* source and QSS contracts" VERBATIM)
    add_custom_target(d2c_check_design_component_catalog
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_design_component_catalog.py" --check
        DEPENDS d2c_generate_design_component_catalog
        COMMENT "Check the Figma public component catalog" VERBATIM)
    add_dependencies(generated_component_sdk d2c_check_native_components
        d2c_check_design_component_catalog)
endif()
