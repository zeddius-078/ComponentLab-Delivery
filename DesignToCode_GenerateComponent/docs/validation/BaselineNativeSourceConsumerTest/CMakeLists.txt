cmake_minimum_required(VERSION 3.21)
project(GeneratedComponentSDKBaselineSourceConsumerTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT BaselineNativeComponentPackage OR
   NOT EXISTS "${BaselineNativeComponentPackage}/CMakeLists.txt")
    message(FATAL_ERROR "BaselineNativeComponentPackage must name the source ComponentPackage root.")
endif()

# This is intentionally source consumption, not a producer workspace include.
add_subdirectory("${BaselineNativeComponentPackage}" baseline-native-component-package)

add_executable(baseline_native_source_consumer ../BaselineNativeSDKConsumerTest/main.cpp)
target_link_libraries(baseline_native_source_consumer PRIVATE GeneratedComponentSDK::Components)

enable_testing()
add_test(NAME baseline_native_source_package_header_resource_and_wrappers
    COMMAND baseline_native_source_consumer)
set_tests_properties(baseline_native_source_package_header_resource_and_wrappers PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 30)
