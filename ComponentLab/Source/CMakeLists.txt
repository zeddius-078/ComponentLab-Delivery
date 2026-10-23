cmake_minimum_required(VERSION 3.21)
project(ComponentLab VERSION 5.0.0 LANGUAGES CXX)
file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../output/work" _allowed_build)
file(REAL_PATH "${CMAKE_BINARY_DIR}" _actual_build)
cmake_path(IS_PREFIX _allowed_build "${_actual_build}" NORMALIZE _inside_build)
if(NOT _inside_build OR _actual_build STREQUAL _allowed_build)
    message(FATAL_ERROR "ComponentLab compilation must stay below project/output/work.")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
option(COMPONENTLAB_BUILD_DEMO "Build the component display application" ON)
include(CTest)
include("${CMAKE_CURRENT_SOURCE_DIR}/GeneratedComponentDependency.cmake")
find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets)
if(NOT COMPONENTLAB_BASELINE_SDK)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Svg)
endif()
if(BUILD_TESTING)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Test)
    find_package(Python3 3.9...<3.10 REQUIRED COMPONENTS Interpreter)
endif()

if(MSVC)
    if(DEFINED BUTTON_MSVC_INCLUDE_PREFIX)
        set(CMAKE_CL_SHOWINCLUDES_PREFIX "${BUTTON_MSVC_INCLUDE_PREFIX}")
    endif()
    add_compile_options(/W4 /permissive- /utf-8)
    # Qt 5.15.2 still references STL helpers removed by MSVC 19.51. Reuse the
    # SDK-owned, version-gated compatibility header so all three consumers are
    # compiled against the same toolchain contract.
    if(MSVC_VERSION GREATER_EQUAL 1951 AND Qt5Core_VERSION VERSION_EQUAL 5.15.2)
        add_compile_options("/FI${GENERATED_COMPONENT_SDK_ROOT}/include/MyDesignSystem/qt5152_msvc_compat.h")
    endif()
endif()

# The current production SDK is deliberately a small Qt-native package.  Its
# gallery has a single deep consumer seam: public Z_* headers plus the package
# QRC/QSS.  Historical Figma/Factory pages remain below for legacy packages;
# they are not compiled or installed for this baseline SDK.
if(COMPONENTLAB_BASELINE_SDK)
    set(COMPONENT_LAB_BASELINE_SOURCES
        delivery/baseline/BaselineComponentLab.cpp
        delivery/baseline/BaselineGallery.cpp
        delivery/baseline/BaselineGallery.h
        delivery/baseline/BaselineGallery.qrc)
    if(COMPONENTLAB_BUILD_DEMO)
        add_executable(component_lab WIN32 ${COMPONENT_LAB_BASELINE_SOURCES})
        target_link_libraries(component_lab PRIVATE
            GeneratedComponentSDK::Components Qt5::Core Qt5::Gui Qt5::Widgets)
    endif()
    if(BUILD_TESTING)
        add_executable(baseline_component_lab_tests
            delivery/baseline/BaselineConsumerTests.cpp
            delivery/baseline/BaselineGallery.cpp
            delivery/baseline/BaselineGallery.h
            delivery/baseline/BaselineGallery.qrc)
        target_link_libraries(baseline_component_lab_tests PRIVATE
            GeneratedComponentSDK::Components Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Test)
        add_test(NAME baseline_component_lab_consumer COMMAND baseline_component_lab_tests)
        set_tests_properties(baseline_component_lab_consumer PROPERTIES
            # CTest does not inherit the launcher path on Windows.  The SDK
            # contract is a normal Qt consumer contract, so load Qt from the
            # selected kit rather than relying on a machine-global PATH.
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen;PATH=$<TARGET_FILE_DIR:Qt5::Core>")
        add_test(NAME directory_structure COMMAND "${Python3_EXECUTABLE}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/delivery/tests/test_directory_structure.py")
    endif()
    if(COMPONENTLAB_BUILD_DEMO)
        install(TARGETS component_lab RUNTIME DESTINATION .)
        install(FILES delivery/qt.conf DESTINATION .)
        install(FILES delivery/tools/RunOffline.cmd DESTINATION . RENAME Run.cmd)
        install(FILES "${GENERATED_COMPONENT_SDK_ROOT}/manifest/component-sdk.json"
            DESTINATION manifest)
    endif()
    return()
endif()

# Presentation code may arrange generated controls and report their state. The implementation
# of Button, Checkbox, Input, Select, Token resolution and appearance remains inside the SDK.
add_library(component_lab_presentation STATIC
    presentation/DemoRuntime.h presentation/DemoRuntime.cpp
    presentation/CheckBoxGroupController.h presentation/CheckBoxGroupController.cpp)
target_include_directories(component_lab_presentation PUBLIC presentation)
target_link_libraries(component_lab_presentation PUBLIC GeneratedComponentSDK::Components)

set(COMPONENT_LAB_DEMO_SOURCES
    delivery/demo/DemoWindow.h delivery/demo/DemoWindow.cpp
    delivery/demo/NativeGalleryPages.h delivery/demo/NativeGalleryPages.cpp
    delivery/demo/NavigationGallery.h delivery/demo/NavigationGallery.cpp
    delivery/demo/FigmaGalleryView.h delivery/demo/FigmaGalleryView.cpp
    delivery/demo/FigmaLayoutEvidence.h delivery/demo/FigmaLayoutEvidence.cpp
    delivery/demo/CheckboxGallery.h delivery/demo/CheckboxGallery.cpp
    delivery/demo/ButtonGallery.h delivery/demo/ButtonGallery.cpp
    delivery/demo/DisplayGallery.h delivery/demo/DisplayGallery.cpp
    delivery/demo/ActionGallery.h delivery/demo/ActionGallery.cpp
    delivery/demo/TabsGallery.h delivery/demo/TabsGallery.cpp
    delivery/demo/ToolBoxGallery.h delivery/demo/ToolBoxGallery.cpp
    delivery/demo/DockWidgetGallery.h delivery/demo/DockWidgetGallery.cpp
    delivery/demo/MdiGallery.h delivery/demo/MdiGallery.cpp
    delivery/demo/SelectionGallery.h delivery/demo/SelectionGallery.cpp
    delivery/demo/InputGallery.h delivery/demo/InputGallery.cpp
    delivery/demo/PageFlowGallery.h delivery/demo/PageFlowGallery.cpp
    delivery/demo/CompositeGallery.h delivery/demo/CompositeGallery.cpp
    delivery/demo/DataViewsGallery.h delivery/demo/DataViewsGallery.cpp
    delivery/demo/WindowsGallery.h delivery/demo/WindowsGallery.cpp
    delivery/demo/ContainersGallery.h delivery/demo/ContainersGallery.cpp
    delivery/demo/NativeDisplayGallery.h delivery/demo/NativeDisplayGallery.cpp
    delivery/demo/EvidenceGallery.h delivery/demo/EvidenceGallery.cpp
    delivery/demo/V2ComponentMapPage.h delivery/demo/V2ComponentMapPage.cpp
    delivery/demo/ButtonPages.cpp delivery/demo/FormPages.cpp
    delivery/demo/ComponentLab.ui)

if(COMPONENTLAB_BUILD_DEMO)
    add_executable(component_lab WIN32 delivery/demo/main.cpp ${COMPONENT_LAB_DEMO_SOURCES})
    target_include_directories(component_lab PRIVATE delivery/demo)
    target_compile_definitions(component_lab PRIVATE
        V2_COMPONENT_MAP_PATH="${GENERATED_COMPONENT_SDK_ROOT}/resources/mappings/figma-component-map-v2.json"
        V2_COMPONENT_REGISTRY_PATH="${GENERATED_COMPONENT_SDK_ROOT}/resources/mappings/component-registry.json")
    target_link_libraries(component_lab PRIVATE component_lab_presentation)
endif()

if(BUILD_TESTING)
    set(TEST_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../output/logs/tests" CACHE PATH "Test reports")
    get_filename_component(_test_output "${TEST_OUTPUT_DIR}" ABSOLUTE)
    get_filename_component(_test_root "${CMAKE_CURRENT_SOURCE_DIR}/../output/logs" ABSOLUTE)
    cmake_path(IS_PREFIX _test_root "${_test_output}" NORMALIZE _test_in_logs)
    if(NOT _test_in_logs)
        message(FATAL_ERROR "TEST_OUTPUT_DIR must stay below project/output/logs.")
    endif()
    file(MAKE_DIRECTORY "${TEST_OUTPUT_DIR}")
    add_executable(checkbox_tests delivery/tests/ContractTests.cpp)
    target_link_libraries(checkbox_tests PRIVATE component_lab_presentation Qt5::Test)
    add_executable(forms_tests delivery/tests/FormsTests.cpp)
    target_link_libraries(forms_tests PRIVATE component_lab_presentation Qt5::Test)
    add_executable(composition_tests delivery/tests/CompositionTests.cpp ${COMPONENT_LAB_DEMO_SOURCES})
    target_include_directories(composition_tests PRIVATE delivery/demo)
    target_compile_definitions(composition_tests PRIVATE
        V2_COMPONENT_MAP_PATH="${GENERATED_COMPONENT_SDK_ROOT}/resources/mappings/figma-component-map-v2.json"
        V2_COMPONENT_REGISTRY_PATH="${GENERATED_COMPONENT_SDK_ROOT}/resources/mappings/component-registry.json")
    target_link_libraries(composition_tests PRIVATE component_lab_presentation Qt5::Test)
    add_executable(token_probe delivery/tests/TokenProbe.cpp)
    target_compile_definitions(token_probe PRIVATE
        COMPONENT_SDK_COMPONENT_MAP="${GENERATED_COMPONENT_SDK_ROOT}/resources/mappings/qt-component-map.json")
    target_link_libraries(token_probe PRIVATE component_lab_presentation Qt5::Test)
    add_executable(table_token_tests delivery/tests/TableTokenTests.cpp
        delivery/demo/DataViewsGallery.h delivery/demo/DataViewsGallery.cpp
        delivery/demo/FigmaGalleryView.h delivery/demo/FigmaGalleryView.cpp)
    target_include_directories(table_token_tests PRIVATE delivery/demo)
    target_link_libraries(table_token_tests PRIVATE component_lab_presentation)

    add_test(NAME directory_structure COMMAND "${Python3_EXECUTABLE}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/delivery/tests/test_directory_structure.py")
    foreach(PROFILE standard-light wide-light standard-dark wide-dark)
        add_test(NAME ${PROFILE} COMMAND checkbox_tests -o ${TEST_OUTPUT_DIR}/${PROFILE}.txt,txt)
        add_test(NAME forms_${PROFILE} COMMAND forms_tests -o ${TEST_OUTPUT_DIR}/forms-${PROFILE}.txt,txt)
        add_test(NAME composition_${PROFILE} COMMAND composition_tests -o ${TEST_OUTPUT_DIR}/composition-${PROFILE}.txt,txt)
        add_test(NAME table_${PROFILE} COMMAND table_token_tests)
        foreach(TEST_NAME ${PROFILE} forms_${PROFILE} composition_${PROFILE} table_${PROFILE})
            file(MAKE_DIRECTORY "${TEST_OUTPUT_DIR}/${TEST_NAME}")
            set_tests_properties(${TEST_NAME} PROPERTIES
                # CTest launches test executables without the Qt launcher
                # environment used by the demo.  Add the imported Qt bin
                # directory explicitly; otherwise Windows shows the hidden
                # missing-Qt-DLL dialog and the test appears to hang until
                # its timeout.
                ENVIRONMENT "QT_QPA_PLATFORM=offscreen;DEMO_PROFILE=${PROFILE};DEMO_EVIDENCE=${TEST_OUTPUT_DIR}/${TEST_NAME};PATH=$<TARGET_FILE_DIR:Qt5::Core>"
                # The full native gallery regression currently takes about 165-250 s on
                # the supported MSVC/Qt runner; leave headroom so concurrent OS activity
                # does not turn a completed assertion run into a false timeout.
                # The composition suite builds large Figma state galleries
                # incrementally; a 20-case input/navigation slice already
                # exceeds five minutes on the supported offscreen runner.
                TIMEOUT 900)
        endforeach()
    endforeach()
endif()

if(COMPONENTLAB_BUILD_DEMO)
    install(TARGETS component_lab RUNTIME DESTINATION .)
    install(FILES delivery/qt.conf DESTINATION .)
    install(FILES delivery/tools/RunOffline.cmd DESTINATION . RENAME Run.cmd)
    install(DIRECTORY "${GENERATED_COMPONENT_SDK_ROOT}/licenses/" DESTINATION licenses)
    install(FILES
        "${GENERATED_COMPONENT_SDK_ROOT}/resources/mappings/figma-component-map-v2.json"
        "${GENERATED_COMPONENT_SDK_ROOT}/resources/mappings/component-registry.json"
        DESTINATION resources/mappings)
    install(DIRECTORY "${GENERATED_COMPONENT_SDK_ROOT}/resources/tokens/default/" DESTINATION TokenSets/default
        PATTERN "*.h" EXCLUDE)
endif()
