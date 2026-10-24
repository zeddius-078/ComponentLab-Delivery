# ComponentLab displays the published component SDK; it does not build component sources.
if(NOT GENERATED_COMPONENT_SDK_ROOT)
    message(FATAL_ERROR "GENERATED_COMPONENT_SDK_ROOT_REQUIRED: use the verified build entry or pass an explicit -DGENERATED_COMPONENT_SDK_ROOT.")
endif()
get_filename_component(GENERATED_COMPONENT_SDK_ROOT "${GENERATED_COMPONENT_SDK_ROOT}" ABSOLUTE)
if(DEFINED D2C_CONFIGURED_SDK_ROOT AND NOT D2C_CONFIGURED_SDK_ROOT STREQUAL GENERATED_COMPONENT_SDK_ROOT)
    message(FATAL_ERROR "SDK_CACHE_CONFLICT: choose a new build directory.")
endif()
set(D2C_CONFIGURED_SDK_ROOT "${GENERATED_COMPONENT_SDK_ROOT}" CACHE INTERNAL "SDK bound to this build" FORCE)
set(_GENERATED_COMPONENT_CONFIG "${GENERATED_COMPONENT_SDK_ROOT}/cmake/GeneratedComponentSDKConfig.cmake")
if(NOT EXISTS "${_GENERATED_COMPONENT_CONFIG}")
    message(FATAL_ERROR "Authoritative GeneratedComponentSDK is missing. Publish it from DesignToCode_GenerateComponent or set GENERATED_COMPONENT_SDK_ROOT.")
endif()
file(READ "${GENERATED_COMPONENT_SDK_ROOT}/manifest/component-sdk.json" _sdk_manifest)
string(JSON _sdk_schema GET "${_sdk_manifest}" schemaVersion)
string(JSON _sdk_configuration GET "${_sdk_manifest}" toolchain configuration)
string(JSON _sdk_crt GET "${_sdk_manifest}" toolchain crt)
string(JSON _sdk_kind ERROR_VARIABLE _sdk_kind_error GET "${_sdk_manifest}" packageKind)
set(COMPONENTLAB_BASELINE_SDK OFF)
if(NOT _sdk_kind_error AND _sdk_kind STREQUAL "qt-native-baseline")
    set(COMPONENTLAB_BASELINE_SDK ON)
    string(JSON _sdk_state GET "${_sdk_manifest}" publicationState)
    string(JSON _sdk_component_count GET "${_sdk_manifest}" nativeComponentCount)
    string(JSON _sdk_visual_delivery GET "${_sdk_manifest}" visualResourceDelivery)
    if(NOT _sdk_state STREQUAL "baseline-native" OR NOT _sdk_component_count EQUAL 58 OR
       NOT _sdk_visual_delivery STREQUAL "static-qrc")
        message(FATAL_ERROR "SDK_BASELINE_SCOPE_INVALID: expected the published 58-foundation baseline.")
    endif()
    foreach(_baseline_file IN ITEMS
            "include/MyDesignSystem/ZNativeComponents.h"
            "manifest/base-sdk-scope.json")
        if(NOT EXISTS "${GENERATED_COMPONENT_SDK_ROOT}/${_baseline_file}")
            message(FATAL_ERROR "SDK_BASELINE_FILE_MISSING: ${_baseline_file}")
        endif()
    endforeach()
    if(EXISTS "${GENERATED_COMPONENT_SDK_ROOT}/resources")
        message(FATAL_ERROR "SDK_BASELINE_MIDDLEWARE_PRESENT: ComponentLab must not consume runtime Token or mapping resources.")
    endif()
endif()
if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_required_crt "MDd")
    else()
        set(_required_crt "MD")
    endif()
else()
    set(_required_crt "gnu")
endif()
if(NOT _sdk_schema EQUAL 2 OR NOT _sdk_configuration STREQUAL CMAKE_BUILD_TYPE OR NOT _sdk_crt STREQUAL _required_crt)
    message(FATAL_ERROR "SDK_ABI_MISMATCH: configuration and CRT must match; no Release fallback.")
endif()

# Consumers inspect the selected publication only. Producer-source freshness is
# a producer/workspace verification concern, never a private-source dependency.

# CMake keeps imported targets alive across an in-place reconfigure.  Without
# this check, changing GENERATED_COMPONENT_SDK_ROOT can silently leave the
# old Release SDK target attached to a Debug build (or vice versa), producing
# opaque MSVC LNK2038 errors much later at link time.
if(TARGET GeneratedComponentSDK::Components)
    get_target_property(_ACTIVE_COMPONENT_SDK_LIB GeneratedComponentSDK::Components IMPORTED_LOCATION)
    file(TO_CMAKE_PATH "${_ACTIVE_COMPONENT_SDK_LIB}" _ACTIVE_COMPONENT_SDK_LIB)
    if(WIN32)
        set(_EXPECTED_COMPONENT_SDK_LIB "${GENERATED_COMPONENT_SDK_ROOT}/lib/GeneratedComponentSDK.lib")
    else()
        set(_EXPECTED_COMPONENT_SDK_LIB "${GENERATED_COMPONENT_SDK_ROOT}/lib/libGeneratedComponentSDK.so")
    endif()
    if(NOT _ACTIVE_COMPONENT_SDK_LIB STREQUAL _EXPECTED_COMPONENT_SDK_LIB)
        message(FATAL_ERROR
            "GeneratedComponentSDK target is already bound to '${_ACTIVE_COMPONENT_SDK_LIB}', "
            "but this configure requests '${_EXPECTED_COMPONENT_SDK_LIB}'. "
            "Use a fresh build directory when switching SDK configurations.")
    endif()
endif()
if(COMPONENTLAB_BASELINE_SDK)
    find_package(GeneratedComponentSDK 2 CONFIG REQUIRED
        PATHS "${GENERATED_COMPONENT_SDK_ROOT}/cmake" NO_DEFAULT_PATH)
else()
    find_package(GeneratedComponentSDK 1.0 CONFIG REQUIRED
        PATHS "${GENERATED_COMPONENT_SDK_ROOT}/cmake" NO_DEFAULT_PATH)
endif()
