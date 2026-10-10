cmake_minimum_required(VERSION 3.21)
project(ComponentLab VERSION 5.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
option(COMPONENTLAB_BUILD_DEMO "Build the component display application" ON)
include(CTest)
find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets Svg)
if(BUILD_TESTING)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Test)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()
include("${CMAKE_CURRENT_SOURCE_DIR}/GeneratedComponentDependency.cmake")

if(MSVC)
    if(DEFINED BUTTON_MSVC_INCLUDE_PREFIX)
        set(CMAKE_CL_SHOWINCLUDES_PREFIX "${BUTTON_MSVC_INCLUDE_PREFIX}")
    endif()
    add_compile_options(/W4 /permissive- /utf-8)
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
    delivery/demo/ButtonPages.cpp delivery/demo/FormPages.cpp
    delivery/demo/ComponentLab.ui)

if(COMPONENTLAB_BUILD_DEMO)
    add_executable(component_lab WIN32 delivery/demo/main.cpp ${COMPONENT_LAB_DEMO_SOURCES})
    target_include_directories(component_lab PRIVATE delivery/demo)
    target_link_libraries(component_lab PRIVATE component_lab_presentation)
endif()

if(BUILD_TESTING)
    set(TEST_OUTPUT_DIR "${CMAKE_BINARY_DIR}/evidence/tests" CACHE PATH "Test reports")
    file(MAKE_DIRECTORY "${TEST_OUTPUT_DIR}")
    add_executable(checkbox_tests delivery/tests/ContractTests.cpp)
    target_link_libraries(checkbox_tests PRIVATE component_lab_presentation Qt5::Test)
    add_executable(forms_tests delivery/tests/FormsTests.cpp)
    target_link_libraries(forms_tests PRIVATE component_lab_presentation Qt5::Test)
    add_executable(solid_inner_shadow_tests delivery/tests/SolidInnerShadowTests.cpp)
    target_link_libraries(solid_inner_shadow_tests PRIVATE component_lab_presentation Qt5::Test)
    add_executable(composition_tests delivery/tests/CompositionTests.cpp ${COMPONENT_LAB_DEMO_SOURCES})
    target_include_directories(composition_tests PRIVATE delivery/demo)
    target_link_libraries(composition_tests PRIVATE component_lab_presentation Qt5::Test)
    add_executable(token_probe delivery/tests/TokenProbe.cpp)
    target_compile_definitions(token_probe PRIVATE
        COMPONENT_SDK_DESIGN_MAP="${GENERATED_COMPONENT_SDK_ROOT}/resources/mappings/design-component-map.json")
    target_link_libraries(token_probe PRIVATE component_lab_presentation Qt5::Test)

    add_test(NAME directory_structure COMMAND "${Python3_EXECUTABLE}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/delivery/tests/test_directory_structure.py")
    foreach(PROFILE light-comfortable light-compact dark-comfortable dark-compact)
        add_test(NAME ${PROFILE} COMMAND checkbox_tests -o ${TEST_OUTPUT_DIR}/${PROFILE}.txt,txt)
        add_test(NAME forms_${PROFILE} COMMAND forms_tests -o ${TEST_OUTPUT_DIR}/forms-${PROFILE}.txt,txt)
        add_test(NAME solid_inner_shadow_${PROFILE} COMMAND solid_inner_shadow_tests -o ${TEST_OUTPUT_DIR}/solid-inner-shadow-${PROFILE}.txt,txt)
        add_test(NAME composition_${PROFILE} COMMAND composition_tests -o ${TEST_OUTPUT_DIR}/composition-${PROFILE}.txt,txt)
        foreach(TEST_NAME ${PROFILE} forms_${PROFILE} solid_inner_shadow_${PROFILE} composition_${PROFILE})
            set_tests_properties(${TEST_NAME} PROPERTIES
                ENVIRONMENT "QT_QPA_PLATFORM=offscreen;DEMO_PROFILE=${PROFILE};DEMO_EVIDENCE=${TEST_OUTPUT_DIR}/${TEST_NAME}"
                TIMEOUT 180)
        endforeach()
    endforeach()
endif()

if(COMPONENTLAB_BUILD_DEMO)
    install(TARGETS component_lab RUNTIME DESTINATION .)
    install(FILES delivery/qt.conf DESTINATION .)
    install(FILES delivery/tools/RunOffline.cmd DESTINATION . RENAME Run.cmd)
    install(DIRECTORY "${GENERATED_COMPONENT_SDK_ROOT}/licenses/" DESTINATION licenses)
    install(DIRECTORY "${GENERATED_COMPONENT_SDK_ROOT}/resources/tokens/default/" DESTINATION TokenSets/default
        PATTERN "*.h" EXCLUDE)
endif()
