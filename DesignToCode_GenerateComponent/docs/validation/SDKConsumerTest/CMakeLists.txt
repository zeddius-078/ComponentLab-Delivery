cmake_minimum_required(VERSION 3.21)
project(GeneratedComponentSDKConsumerTest LANGUAGES CXX)
if(MSVC AND DEFINED BUTTON_MSVC_INCLUDE_PREFIX)
    set(CMAKE_CL_SHOWINCLUDES_PREFIX "${BUTTON_MSVC_INCLUDE_PREFIX}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Svg)
find_package(GeneratedComponentSDK 1.0 CONFIG REQUIRED)

add_executable(sdk_consumer_smoke main.cpp)
target_link_libraries(sdk_consumer_smoke PRIVATE GeneratedComponentSDK::Components)

enable_testing()
add_test(NAME sdk_header_and_instance COMMAND sdk_consumer_smoke)
set_tests_properties(sdk_header_and_instance PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 30)

