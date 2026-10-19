# ComponentLabSourceSDK consumer build (self-contained source delivery).
#
# Consumed two ways:
#   1) Standalone: cmake -S <this dir> -B <cache> && cmake --build <cache>
#   2) Embedded:   add_subdirectory(<this dir> ComponentLabSourceSDK)
#
# The component sources are compiled locally into the consuming project.
# No precompiled library is shipped. Do not link the binary SDK
# (GeneratedComponentSDK::Components from find_package) in the same target
# graph as this package: both define the same component symbols.
cmake_minimum_required(VERSION 3.21)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_LIST_DIR)
    project(ComponentLabSourceSDK LANGUAGES CXX)
    set(COMPONENTLAB_SOURCE_SDK_STANDALONE ON)
else()
    set(COMPONENTLAB_SOURCE_SDK_STANDALONE OFF)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets Svg)

# The explicit list mirrors the generated_component_sdk target of
# DesignToCode_GenerateComponent. NativeWidgetWrappers.cpp is intentionally
# excluded (it is not part of that target either). The native Z_* set is
# generated source and is globbed, same as the binary SDK target.
file(GLOB COMPONENTLAB_SOURCE_SDK_NATIVE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/src/component/native/Z_*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/component/native/Z_*.h")

set(COMPONENTLAB_SOURCE_SDK_SOURCES
    src/component/ComponentFactory.cpp
    src/component/DesignComponentCatalog.cpp
    src/component/DateTimeAppearance.cpp
    src/component/SpinBoxAppearance.cpp
    src/component/SliderAppearance.cpp
    src/component/ScrollBarAppearance.cpp
    src/component/DialAppearance.cpp
    src/component/LineEditAppearance.cpp
    src/component/ShortcutAppearance.cpp
    src/component/ComboBoxAppearance.cpp
    src/component/CommandLinkAppearance.cpp
    src/component/ToolButtonAppearance.cpp
    src/component/DialogButtonBoxAppearance.cpp
    src/component/TabAppearance.cpp
    src/component/ToolBoxAppearance.cpp
    src/component/DockWidgetAppearance.cpp
    src/component/MdiAppearance.cpp
    src/component/LabelAppearance.cpp
    src/component/ProgressAppearance.cpp
    src/component/RadioAppearance.cpp
    src/component/NavigationAppearance.cpp
    src/component/PageFlowAppearance.cpp
    src/component/Z_PushButton.cpp
    src/component/Z_CheckBox.cpp
    src/component/Z_FoundationComponents.cpp
    src/component/Z_DesignComponents.cpp
    src/component/Z_InputField.cpp
    src/component/Z_Select.cpp
    src/component/Z_OtpInput.cpp
    src/component/IconAssetCatalog.cpp
    src/component/VisualAssetCatalog.cpp
    src/component/IconSource.cpp
    src/component/FocusIndicator.cpp
    src/component/native/NativeComponentRegistry.cpp
    src/surface/Surface.cpp
    src/surface/FieldChrome.cpp
    src/surface/ProfileContext.cpp
    src/surface/Typography.cpp
    src/surface/TokenResources.cpp
    src/theme/ComponentTheme.cpp
    src/designer/DesignerWidgets.cpp
    ${COMPONENTLAB_SOURCE_SDK_NATIVE_SOURCES}
    runtime/component-resources.qrc
)

add_library(componentlab_source_sdk STATIC ${COMPONENTLAB_SOURCE_SDK_SOURCES})
add_library(GeneratedComponentSDK::Components ALIAS componentlab_source_sdk)
add_library(componentlab_source_sdk_runtime INTERFACE)
add_library(GeneratedComponentSDK::Runtime ALIAS componentlab_source_sdk_runtime)
target_link_libraries(componentlab_source_sdk_runtime INTERFACE GeneratedComponentSDK::Components)

set_target_properties(componentlab_source_sdk PROPERTIES
    OUTPUT_NAME ComponentLabSourceSDK
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF)

# Include paths mirror the binary SDK target's include layout, relocated
# into this package (public headers under include/MyDesignSystem).
target_include_directories(componentlab_source_sdk PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${CMAKE_CURRENT_LIST_DIR}/include/MyDesignSystem"
    "${CMAKE_CURRENT_LIST_DIR}/src/component"
    "${CMAKE_CURRENT_LIST_DIR}/src/component/native"
    "${CMAKE_CURRENT_LIST_DIR}/src/surface"
    "${CMAKE_CURRENT_LIST_DIR}/src/theme"
    "${CMAKE_CURRENT_LIST_DIR}/src/designer")
target_link_libraries(componentlab_source_sdk PUBLIC Qt5::Widgets Qt5::Svg)
target_compile_features(componentlab_source_sdk PUBLIC cxx_std_17)
if(MSVC)
    target_compile_options(componentlab_source_sdk PUBLIC /utf-8 PRIVATE /W4 /permissive-)
    if(MSVC_VERSION GREATER_EQUAL 1951 AND Qt5Core_VERSION VERSION_EQUAL 5.15.2)
        target_compile_options(componentlab_source_sdk PUBLIC
            "/FI${CMAKE_CURRENT_LIST_DIR}/include/MyDesignSystem/qt5152_msvc_compat.h")
    endif()
endif()

set(COMPONENTLAB_SOURCE_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}")
