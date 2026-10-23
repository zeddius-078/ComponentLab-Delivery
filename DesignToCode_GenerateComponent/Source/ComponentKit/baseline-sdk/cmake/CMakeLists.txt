include_guard(GLOBAL)

get_filename_component(Z_NATIVE_PACKAGE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets)

set(CMAKE_AUTORCC ON)
add_library(Components STATIC
    "${Z_NATIVE_PACKAGE_ROOT}/SRC/ZNativeComponents.cpp"
    "${Z_NATIVE_PACKAGE_ROOT}/View/z_native_components.qrc")
add_library(GeneratedComponentSDK::Components ALIAS Components)

set_target_properties(Components PROPERTIES
    OUTPUT_NAME GeneratedComponentSDK
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES)
target_include_directories(Components PUBLIC
    "$<BUILD_INTERFACE:${Z_NATIVE_PACKAGE_ROOT}/Include>"
    "$<INSTALL_INTERFACE:include>")
target_link_libraries(Components PUBLIC Qt5::Widgets)
if(MSVC)
    target_compile_options(Components PUBLIC /utf-8 PRIVATE /W4 /permissive-)
    if(MSVC_VERSION GREATER_EQUAL 1951 AND Qt5Core_VERSION VERSION_EQUAL 5.15.2)
        target_compile_options(Components PRIVATE
            "/FI${Z_NATIVE_PACKAGE_ROOT}/Include/MyDesignSystem/qt5152_msvc_compat.h")
    endif()
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
if(PROJECT_IS_TOP_LEVEL OR Z_NATIVE_COMPONENTS_INSTALL)
    install(TARGETS Components EXPORT GeneratedComponentSDKTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(DIRECTORY "${Z_NATIVE_PACKAGE_ROOT}/Include/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    # The binary SDK links the QRC (QSS, SVG and HarmonyOS fonts) into the
    # static library.  Do not duplicate the 20 MiB SC font as loose files in
    # the binary package.  The separately delivered source package preserves
    # Form/View/Include/SRC/cmake/manifest for QSS or source-level migration.
    install(FILES "${Z_NATIVE_PACKAGE_ROOT}/manifest/base-sdk-scope.json"
        DESTINATION "manifest")
    install(EXPORT GeneratedComponentSDKTargets
        FILE GeneratedComponentSDKTargets.cmake
        NAMESPACE GeneratedComponentSDK::
        DESTINATION "cmake")
    configure_package_config_file(
        "${CMAKE_CURRENT_LIST_DIR}/GeneratedComponentSDKConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/GeneratedComponentSDKConfig.cmake"
        INSTALL_DESTINATION "cmake")
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/GeneratedComponentSDKConfigVersion.cmake"
        VERSION 2.0.0
        COMPATIBILITY SameMajorVersion)
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/GeneratedComponentSDKConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/GeneratedComponentSDKConfigVersion.cmake"
        DESTINATION "cmake")
endif()
