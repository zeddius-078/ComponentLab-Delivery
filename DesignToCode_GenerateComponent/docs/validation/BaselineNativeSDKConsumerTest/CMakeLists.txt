cmake_minimum_required(VERSION 3.21)
project(GeneratedComponentSDKBaselineConsumerTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets)
find_package(GeneratedComponentSDK 2 CONFIG REQUIRED)

add_executable(baseline_native_sdk_consumer main.cpp)
target_link_libraries(baseline_native_sdk_consumer PRIVATE GeneratedComponentSDK::Components)

enable_testing()
add_test(NAME baseline_native_sdk_header_resource_and_wrappers
    COMMAND baseline_native_sdk_consumer)
set_tests_properties(baseline_native_sdk_header_resource_and_wrappers PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 30)
