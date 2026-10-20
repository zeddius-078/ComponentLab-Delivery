cmake_minimum_required(VERSION 3.21)
# Stage the self-contained ComponentLab Source SDK package.
#
# Invoked from the DesignToCode_GenerateComponent build:
#   cmake -DSDK_SOURCE_DIR=<ComponentKit> -DSDK_OUTPUT_DIR=<dest>
#         -DSDK_TEMPLATE_DIR=<ComponentKit>/source-sdk
#         -P <ComponentKit>/source-sdk/stage_source_sdk.cmake
#
# The staged package mirrors the generated_component_sdk target (same
# sources, same public headers, same qrc resource contract). Nothing outside
# the package is referenced afterwards: no absolute paths, no gen-comp
# private directories.
if(NOT DEFINED SDK_SOURCE_DIR OR NOT DEFINED SDK_OUTPUT_DIR OR NOT DEFINED SDK_TEMPLATE_DIR)
    message(FATAL_ERROR "stage_source_sdk: SDK_SOURCE_DIR / SDK_OUTPUT_DIR / SDK_TEMPLATE_DIR are required")
endif()

get_filename_component(SDK_SOURCE_DIR "${SDK_SOURCE_DIR}" ABSOLUTE)
get_filename_component(SDK_OUTPUT_DIR "${SDK_OUTPUT_DIR}" ABSOLUTE)
get_filename_component(SDK_TEMPLATE_DIR "${SDK_TEMPLATE_DIR}" ABSOLUTE)

set(SDK_RUNTIME_DIR "${SDK_SOURCE_DIR}/runtime")
set(SDK_QRC "${SDK_RUNTIME_DIR}/component-resources.qrc")
if(NOT EXISTS "${SDK_QRC}")
    message(FATAL_ERROR "stage_source_sdk: missing qrc ${SDK_QRC}")
endif()

# The one source file that lives in runtime/component but is NOT part of the
# binary SDK target (and must not be part of the source package).
set(SDK_EXCLUDED_FILES "NativeWidgetWrappers.cpp" "NativeWidgetWrappers.h")

function(stage_copy_file SRC DST)
    get_filename_component(_dst_dir "${DST}" DIRECTORY)
    file(MAKE_DIRECTORY "${_dst_dir}")
    file(COPY_FILE "${SRC}" "${DST}")
endfunction()

# ---------------------------------------------------------------- cleanup
get_filename_component(_project "${SDK_TEMPLATE_DIR}/../../../../../.." ABSOLUTE)
file(REAL_PATH "${_project}/output/work" _allowed)
file(REAL_PATH "${SDK_OUTPUT_DIR}" _output)
cmake_path(IS_PREFIX _allowed "${_output}" NORMALIZE _safe)
if(NOT _safe OR _output STREQUAL _allowed)
    message(FATAL_ERROR "Source SDK staging must be a child of the project output/work directory")
endif()
file(REMOVE_RECURSE "${SDK_OUTPUT_DIR}")
file(MAKE_DIRECTORY "${SDK_OUTPUT_DIR}")

# ------------------------------------------------------------- sources
# Full original layout is preserved (component/component/native/surface/
# theme/designer) because several translation units use relative includes
# such as "../surface/Surface.h" or "native/Z_SpinBox.h". Headers staged
# here are the single physical copy of every public header.
foreach(SDK_SRC_DIR
        "runtime/component"
        "runtime/component/native"
        "runtime/surface"
        "runtime/theme"
        "runtime/designer")
    file(GLOB SDK_SRC_FILES RELATIVE "${SDK_SOURCE_DIR}"
        "${SDK_SOURCE_DIR}/${SDK_SRC_DIR}/*")
    list(SORT SDK_SRC_FILES)
    foreach(SDK_SRC_FILE ${SDK_SRC_FILES})
        get_filename_component(SDK_SRC_NAME "${SDK_SRC_FILE}" NAME)
        if(SDK_SRC_NAME IN_LIST SDK_EXCLUDED_FILES)
            continue()
        endif()
        get_filename_component(SDK_SRC_EXT "${SDK_SRC_FILE}" EXT)
        if(NOT SDK_SRC_EXT STREQUAL ".cpp" AND NOT SDK_SRC_EXT STREQUAL ".h")
            continue()
        endif()
        string(REPLACE "runtime/" "src/" SDK_SRC_DEST "${SDK_SRC_FILE}")
        stage_copy_file("${SDK_SOURCE_DIR}/${SDK_SRC_FILE}"
             "${SDK_OUTPUT_DIR}/${SDK_SRC_DEST}")
        if(SDK_SRC_EXT STREQUAL ".h")
            set("SDK_HEADER_MAP_${SDK_SRC_NAME}" "${SDK_SRC_DEST}")
            list(APPEND SDK_PUBLIC_HEADER_NAMES "${SDK_SRC_NAME}")
        endif()
    endforeach()
endforeach()

# ------------------------------------------- public include forwarding
# include/MyDesignSystem mirrors the binary SDK's installed header set, but
# every entry is a tiny forwarding header pointing at the single physical
# copy under src/. This keeps <MyDesignSystem/X.h> working while making sure
# one translation unit can never see two copies of the same header (which
# would break #pragma once and re-declare the commonWidget enums).
file(MAKE_DIRECTORY "${SDK_OUTPUT_DIR}/include/MyDesignSystem")
list(REMOVE_DUPLICATES SDK_PUBLIC_HEADER_NAMES)
foreach(SDK_PUBLIC_HEADER ${SDK_PUBLIC_HEADER_NAMES})
    set(SDK_PUBLIC_TARGET "${SDK_HEADER_MAP_${SDK_PUBLIC_HEADER}}")
    file(WRITE "${SDK_OUTPUT_DIR}/include/MyDesignSystem/${SDK_PUBLIC_HEADER}"
"#pragma once
// ComponentLab Source SDK forwarding header. The single physical copy of
// this header lives in the src/ tree and is the one that gets compiled.
#include \"../../${SDK_PUBLIC_TARGET}\"
")
endforeach()
# Umbrella: rewrite each include to point at the physical copy in src/.
file(READ "${SDK_SOURCE_DIR}/MyDesignSystem.h" SDK_UMBRELLA_CONTENT)
string(REPLACE "\r\n" "\n" SDK_UMBRELLA_CONTENT "${SDK_UMBRELLA_CONTENT}")
string(REPLACE "\n" ";" SDK_UMBRELLA_LINES "${SDK_UMBRELLA_CONTENT}")
set(SDK_UMBRELLA_REWRITTEN "")
foreach(SDK_UMBRELLA_LINE ${SDK_UMBRELLA_LINES})
    string(STRIP "${SDK_UMBRELLA_LINE}" SDK_UMBRELLA_LINE)
    if(SDK_UMBRELLA_LINE MATCHES "^#include \"native/([^\"]+)\"")
        set(SDK_UMBRELLA_REL "src/component/native/${CMAKE_MATCH_1}")
        string(REGEX REPLACE "^#include \"[^\"]+\""
            "#include \"../../${SDK_UMBRELLA_REL}\""
            SDK_UMBRELLA_LINE "${SDK_UMBRELLA_LINE}")
    elseif(SDK_UMBRELLA_LINE MATCHES "^#include \"([^\"]+)\"")
        set(SDK_UMBRELLA_REL "src/component/${CMAKE_MATCH_1}")
        if(DEFINED SDK_HEADER_MAP_${CMAKE_MATCH_1})
            set(SDK_UMBRELLA_REL "${SDK_HEADER_MAP_${CMAKE_MATCH_1}}")
        endif()
        string(REGEX REPLACE "^#include \"[^\"]+\""
            "#include \"../../${SDK_UMBRELLA_REL}\""
            SDK_UMBRELLA_LINE "${SDK_UMBRELLA_LINE}")
    endif()
    string(APPEND SDK_UMBRELLA_REWRITTEN "${SDK_UMBRELLA_LINE}\n")
endforeach()
file(WRITE "${SDK_OUTPUT_DIR}/include/MyDesignSystem/MyDesignSystem.h"
     "${SDK_UMBRELLA_REWRITTEN}")
stage_copy_file("${SDK_SOURCE_DIR}/qt5152_msvc_compat.h"
     "${SDK_OUTPUT_DIR}/include/MyDesignSystem/qt5152_msvc_compat.h")

# ---------------------------------------------------- resources (qrc)
# The qrc keeps its exact relative references, so the referenced files are
# staged with the same relative layout (runtime/assets, component-
# definitions, design-assets/...).
stage_copy_file("${SDK_QRC}" "${SDK_OUTPUT_DIR}/runtime/component-resources.qrc")
file(READ "${SDK_QRC}" SDK_QRC_CONTENT)
set(SDK_QRC_FILES)
string(REGEX MATCHALL "<file[^>]*>[^<]+</file>" SDK_QRC_ENTRIES "${SDK_QRC_CONTENT}")
foreach(SDK_QRC_ENTRY ${SDK_QRC_ENTRIES})
    string(REGEX REPLACE "^<file[^>]*>" "" SDK_QRC_PATH "${SDK_QRC_ENTRY}")
    string(REGEX REPLACE "</file>$" "" SDK_QRC_PATH "${SDK_QRC_PATH}")
    string(STRIP "${SDK_QRC_PATH}" SDK_QRC_PATH)
    get_filename_component(SDK_QRC_ABS
        "${SDK_RUNTIME_DIR}/${SDK_QRC_PATH}" ABSOLUTE)
    # Safety: resources must stay inside the ComponentKit source tree.
    file(RELATIVE_PATH SDK_QRC_REL "${SDK_SOURCE_DIR}" "${SDK_QRC_ABS}")
    if(SDK_QRC_REL MATCHES "^\\.\\.")
        message(FATAL_ERROR "stage_source_sdk: qrc entry escapes the source tree: ${SDK_QRC_PATH}")
    endif()
    if(NOT EXISTS "${SDK_QRC_ABS}")
        message(FATAL_ERROR "stage_source_sdk: qrc entry missing: ${SDK_QRC_PATH}")
    endif()
    # Stage with the same layout relative to the qrc's own directory, so the
    # delivered qrc keeps its exact relative references (path contract).
    file(RELATIVE_PATH SDK_QRC_DEST "${SDK_RUNTIME_DIR}" "${SDK_QRC_ABS}")
    get_filename_component(SDK_QRC_DEST_ABS
        "${SDK_OUTPUT_DIR}/runtime/${SDK_QRC_DEST}" ABSOLUTE)
    stage_copy_file("${SDK_QRC_ABS}" "${SDK_QRC_DEST_ABS}")
    list(APPEND SDK_QRC_FILES "${SDK_QRC_REL}")
endforeach()
list(LENGTH SDK_QRC_FILES SDK_QRC_FILE_COUNT)

# ------------------------------------------------------------- licenses
file(COPY "${SDK_SOURCE_DIR}/licenses" DESTINATION "${SDK_OUTPUT_DIR}")

# ------------------------------------------------- CMakeLists + README
stage_copy_file("${SDK_TEMPLATE_DIR}/consumer-template.cmake"
     "${SDK_OUTPUT_DIR}/CMakeLists.txt")
stage_copy_file("${SDK_TEMPLATE_DIR}/package-README.md"
     "${SDK_OUTPUT_DIR}/README.md")

# Verify that every explicitly listed source of the delivered CMakeLists
# exists in the staged package (drift guard between the two delivery paths).
file(STRINGS "${SDK_OUTPUT_DIR}/CMakeLists.txt" SDK_TEMPLATE_LINES)
foreach(SDK_TEMPLATE_LINE ${SDK_TEMPLATE_LINES})
    string(REGEX MATCH "src/[A-Za-z0-9_/]+\\.cpp" SDK_TEMPLATE_SRC "${SDK_TEMPLATE_LINE}")
    if(SDK_TEMPLATE_SRC AND NOT EXISTS "${SDK_OUTPUT_DIR}/${SDK_TEMPLATE_SRC}")
        message(FATAL_ERROR
            "stage_source_sdk: delivered CMakeLists lists missing source ${SDK_TEMPLATE_SRC}")
    endif()
endforeach()

# ------------------------------------------------------------- manifest
file(SHA256 "${SDK_OUTPUT_DIR}/runtime/component-resources.qrc" SDK_QRC_SHA256)
file(SHA256 "${SDK_OUTPUT_DIR}/design-assets/DesignSystem/generated/runtime/tokens_qt.json"
     SDK_TOKENS_QT_SHA256)
set(SDK_TOKEN_SOURCE_HASH "")
set(SDK_TOKEN_MANIFEST "${SDK_SOURCE_DIR}/design-assets/DesignSystem/config/manifest.json")
if(EXISTS "${SDK_TOKEN_MANIFEST}")
    file(READ "${SDK_TOKEN_MANIFEST}" SDK_TOKEN_MANIFEST_CONTENT)
    string(JSON SDK_TOKEN_SOURCE_HASH GET "${SDK_TOKEN_MANIFEST_CONTENT}" "sourceHash")
endif()

set(SDK_MANIFEST_FILES "")
file(GLOB_RECURSE SDK_ALL_STAGED LIST_DIRECTORIES FALSE RELATIVE "${SDK_OUTPUT_DIR}"
    "${SDK_OUTPUT_DIR}/*")
list(SORT SDK_ALL_STAGED)
foreach(SDK_STAGED_FILE ${SDK_ALL_STAGED})
    string(REPLACE "\\" "/" SDK_STAGED_FILE "${SDK_STAGED_FILE}")
    file(SHA256 "${SDK_OUTPUT_DIR}/${SDK_STAGED_FILE}" SDK_FILE_SHA256)
    file(SIZE "${SDK_OUTPUT_DIR}/${SDK_STAGED_FILE}" SDK_FILE_BYTES)
    list(APPEND SDK_MANIFEST_FILES
        "{\"path\":\"${SDK_STAGED_FILE}\",\"sha256\":\"${SDK_FILE_SHA256}\",\"bytes\":${SDK_FILE_BYTES}}")
endforeach()
string(JOIN "," SDK_MANIFEST_FILES_JSON ${SDK_MANIFEST_FILES})

set(SDK_MANIFEST_JSON
"{
  \"schemaVersion\": 1,
  \"package\": \"ComponentLabSourceSDK\",
  \"owner\": \"DesignToCode_GenerateComponent\",
  \"version\": \"1.0.0\",
  \"componentAuthority\": true,
  \"tokenAuthority\": true,
  \"linkage\": \"source\",
  \"cmakeTarget\": \"GeneratedComponentSDK::Components\",
  \"umbrellaHeader\": \"include/MyDesignSystem/MyDesignSystem.h\",
  \"profiles\": [\"standard-light\",\"wide-light\",\"standard-dark\",\"wide-dark\"],
  \"provenance\": {
    \"tokenSourceHash\": \"${SDK_TOKEN_SOURCE_HASH}\",
    \"qrcSha256\": \"${SDK_QRC_SHA256}\",
    \"tokensQtSha256\": \"${SDK_TOKENS_QT_SHA256}\",
    \"qrcResourceFileCount\": ${SDK_QRC_FILE_COUNT}
  },
  \"files\": [${SDK_MANIFEST_FILES_JSON}]
}")
file(WRITE "${SDK_OUTPUT_DIR}/manifest.json" "${SDK_MANIFEST_JSON}")

message(STATUS "stage_source_sdk: staged ${SDK_OUTPUT_DIR}")
message(STATUS "stage_source_sdk: qrc resources ${SDK_QRC_FILE_COUNT}, tokenSourceHash=${SDK_TOKEN_SOURCE_HASH}")
