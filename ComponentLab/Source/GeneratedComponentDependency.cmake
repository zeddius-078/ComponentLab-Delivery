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
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_required_crt "MDd")
else()
    set(_required_crt "MD")
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
    file(TO_CMAKE_PATH "${GENERATED_COMPONENT_SDK_ROOT}/lib/GeneratedComponentSDK.lib" _EXPECTED_COMPONENT_SDK_LIB)
    if(NOT _ACTIVE_COMPONENT_SDK_LIB STREQUAL _EXPECTED_COMPONENT_SDK_LIB)
        message(FATAL_ERROR
            "GeneratedComponentSDK target is already bound to '${_ACTIVE_COMPONENT_SDK_LIB}', "
            "but this configure requests '${_EXPECTED_COMPONENT_SDK_LIB}'. "
            "Use a fresh build directory when switching SDK configurations.")
    endif()
endif()
find_package(GeneratedComponentSDK 1.0 CONFIG REQUIRED
    PATHS "${GENERATED_COMPONENT_SDK_ROOT}/cmake" NO_DEFAULT_PATH)
