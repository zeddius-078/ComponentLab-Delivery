cmake_minimum_required(VERSION 3.16)
project(PixsoNativePageConsumer LANGUAGES CXX)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_BINARY_DIR}" _consumer_binary)
if(NOT _consumer_binary MATCHES "/output/work/pixso-native/")
  message(FATAL_ERROR "Consumer compilation must stay in output/work/pixso-native")
endif()
if(NOT PAGE_ROOT OR NOT PAGE_NAME OR NOT GeneratedComponentSDK_DIR OR NOT Qt5_DIR)
  message(FATAL_ERROR "Explicit PAGE_ROOT, PAGE_NAME, GeneratedComponentSDK_DIR and Qt5_DIR required")
endif()
if(NOT PAGE_NAME MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
  message(FATAL_ERROR "Invalid page class identifier")
endif()
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  message(FATAL_ERROR "Consumer probe requires the selected Release SDK")
endif()
if(NOT EXISTS "${Qt5_DIR}/Qt5Config.cmake"
   OR NOT EXISTS "${GeneratedComponentSDK_DIR}/GeneratedComponentSDKConfig.cmake")
  message(FATAL_ERROR "Explicit dependency configuration does not exist; fallback is forbidden")
endif()
find_package(Qt5 5.15.2 EXACT REQUIRED COMPONENTS Widgets Svg)
find_package(GeneratedComponentSDK CONFIG REQUIRED)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${PAGE_ROOT}/form")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
add_executable(native_page_consumer
  main.cpp
  "${PAGE_ROOT}/include/${PAGE_NAME}.h"
  "${PAGE_ROOT}/src/${PAGE_NAME}.cpp"
  "${PAGE_ROOT}/form/${PAGE_NAME}.ui"
  "${PAGE_ROOT}/view/res/${PAGE_NAME}.qrc"
)
target_include_directories(native_page_consumer PRIVATE "${PAGE_ROOT}/include")
# Qt 5 moc may encode a non-ASCII relative include path using the active code
# page. The page header already has an explicit include directory; use its
# portable basename without altering generated page files or the environment.
set_property(TARGET native_page_consumer PROPERTY AUTOMOC_MOC_OPTIONS "-f${PAGE_NAME}.h")
target_compile_definitions(native_page_consumer PRIVATE
  PAGE_HEADER="${PAGE_NAME}.h" PAGE_TYPE=${PAGE_NAME})
target_link_libraries(native_page_consumer PRIVATE GeneratedComponentSDK::Components Qt5::Svg)
if(MSVC)
  target_compile_options(native_page_consumer PRIVATE /utf-8)
endif()
