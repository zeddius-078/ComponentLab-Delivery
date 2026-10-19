# ComponentLab displays the published component SDK; it does not build component sources.
if(NOT GENERATED_COMPONENT_SDK_ROOT)
    if(DEFINED ENV{GENERATED_COMPONENT_SDK_ROOT})
        set(GENERATED_COMPONENT_SDK_ROOT "$ENV{GENERATED_COMPONENT_SDK_ROOT}")
    else()
        get_filename_component(_WORKSPACE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
        # The static SDK must use the same MSVC iterator/debug runtime as the
        # consumer. Prefer the generated Debug package for a Debug configure;
        # fall back to the release package only when that package is absent so
        # first-time Release checkouts remain zero-config.
        if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND EXISTS "${_WORKSPACE_ROOT}/DesignToCode_GenerateComponent/build/sdk-debug/cmake/GeneratedComponentSDKConfig.cmake")
            set(GENERATED_COMPONENT_SDK_ROOT "${_WORKSPACE_ROOT}/DesignToCode_GenerateComponent/build/sdk-debug")
        else()
            set(GENERATED_COMPONENT_SDK_ROOT "${_WORKSPACE_ROOT}/DesignToCode_GenerateComponent/build/sdk")
        endif()
    endif()
endif()
get_filename_component(GENERATED_COMPONENT_SDK_ROOT "${GENERATED_COMPONENT_SDK_ROOT}" ABSOLUTE)
set(_GENERATED_COMPONENT_CONFIG "${GENERATED_COMPONENT_SDK_ROOT}/cmake/GeneratedComponentSDKConfig.cmake")
if(NOT EXISTS "${_GENERATED_COMPONENT_CONFIG}")
    message(FATAL_ERROR "Authoritative GeneratedComponentSDK is missing. Publish it from DesignToCode_GenerateComponent or set GENERATED_COMPONENT_SDK_ROOT.")
endif()

# The V2 map is read by ComponentLab at runtime.  Catch a stale Release or
# Debug companion during configure instead of allowing the gallery to build
# successfully and display an older token binding.
set(_V2_SOURCE_MAP "${CMAKE_CURRENT_LIST_DIR}/../../DesignToCode_GenerateComponent/Source/ComponentKit/design-assets/DesignSystem/config/figma-component-map-v2.json")
set(_V2_CONSUMER_MAP "${GENERATED_COMPONENT_SDK_ROOT}/resources/mappings/figma-component-map-v2.json")
if(EXISTS "${_V2_SOURCE_MAP}" AND EXISTS "${_V2_CONSUMER_MAP}")
    file(SHA256 "${_V2_SOURCE_MAP}" _V2_SOURCE_HASH)
    file(SHA256 "${_V2_CONSUMER_MAP}" _V2_CONSUMER_HASH)
    if(NOT _V2_SOURCE_HASH STREQUAL _V2_CONSUMER_HASH)
        message(FATAL_ERROR
            "Stale V2 component map: '${_V2_CONSUMER_MAP}' does not match "
            "the authoritative source map. Republish the SDK before building ComponentLab.")
    endif()
endif()

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
