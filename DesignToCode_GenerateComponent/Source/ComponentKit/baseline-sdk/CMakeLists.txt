cmake_minimum_required(VERSION 3.21)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_LIST_DIR)
    project(GeneratedComponentSDK LANGUAGES CXX)
endif()

add_subdirectory(cmake)
