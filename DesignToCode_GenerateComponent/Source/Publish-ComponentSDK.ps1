param(
    [string]$QtRoot = $env:QT_ROOT,
    [string]$BuildDirectory,
    [string]$Cmake = $(if ($env:CMAKE_EXE) { $env:CMAKE_EXE } else { 'cmake' }),
    [string]$Generator = 'Ninja'
)
$ErrorActionPreference = 'Stop'
$sourceRoot = $PSScriptRoot
$projectRoot = Split-Path $sourceRoot -Parent
$kit = Join-Path $sourceRoot 'ComponentKit'
$build = if ($BuildDirectory) { [IO.Path]::GetFullPath($BuildDirectory) } else { Join-Path $projectRoot 'build/cache/GeneratedComponentSDK' }
$sdk = Join-Path $projectRoot 'build/sdk'
if (-not $QtRoot -or -not (Test-Path -LiteralPath "$QtRoot/lib/cmake/Qt5")) {
    throw 'Set QT_ROOT or pass -QtRoot for Qt 5.15 MSVC x64.'
}
. "$PSScriptRoot/NativeTools.ps1"
$env:QT_ROOT = $QtRoot
Initialize-NativeTools
& $Cmake -S $kit -B $build -G $Generator '-DCMAKE_BUILD_TYPE=Release' `
    "-DCMAKE_PREFIX_PATH=$QtRoot" '-DD2C_BUILD_TOKEN_TOOLS=ON' `
    '-DBUTTON_MSVC_INCLUDE_PREFIX=注意: 包含文件:'
if ($LASTEXITCODE) { throw 'SDK configure failed.' }
& $Cmake --build $build --config Release --target d2c_generate_tokens
if ($LASTEXITCODE) { throw 'Token generation failed.' }
& $Cmake --build $build --config Release --target d2c_check_tokens
if ($LASTEXITCODE) { throw 'Token check failed.' }
& $Cmake --build $build --config Release --target generated_component_sdk --parallel 4
if ($LASTEXITCODE) { throw 'SDK build failed.' }
if (Test-Path -LiteralPath $sdk) {
    $resolved = [IO.Path]::GetFullPath($sdk)
    $allowed = [IO.Path]::GetFullPath((Join-Path $projectRoot 'build/sdk'))
    if ($resolved -ne $allowed) {
        throw "Unsafe SDK output path: $resolved"
    }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
& $Cmake --install $build --config Release --prefix $sdk
if ($LASTEXITCODE) { throw 'SDK install failed.' }

$config = @'
if(TARGET GeneratedComponentSDK::Components)
  return()
endif()
include(CMakeFindDependencyMacro)
find_dependency(Qt5 5.15 COMPONENTS Widgets Svg)
get_filename_component(_GENERATED_COMPONENT_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
add_library(GeneratedComponentSDK::Components STATIC IMPORTED)
set_target_properties(GeneratedComponentSDK::Components PROPERTIES
  IMPORTED_LOCATION "${_GENERATED_COMPONENT_SDK_ROOT}/lib/GeneratedComponentSDK.lib"
  INTERFACE_INCLUDE_DIRECTORIES "${_GENERATED_COMPONENT_SDK_ROOT}/include;${_GENERATED_COMPONENT_SDK_ROOT}/include/MyDesignSystem"
  INTERFACE_LINK_LIBRARIES "Qt5::Widgets;Qt5::Svg")
if(MSVC AND MSVC_VERSION GREATER_EQUAL 1951 AND Qt5Core_VERSION VERSION_EQUAL 5.15.2)
  set_property(TARGET GeneratedComponentSDK::Components APPEND PROPERTY
    INTERFACE_COMPILE_OPTIONS "/FI${_GENERATED_COMPONENT_SDK_ROOT}/include/MyDesignSystem/qt5152_msvc_compat.h")
endif()
if(MSVC)
  set_property(TARGET GeneratedComponentSDK::Components APPEND PROPERTY
    INTERFACE_COMPILE_OPTIONS "/utf-8")
endif()
add_library(GeneratedComponentSDK::Runtime INTERFACE IMPORTED)
set_property(TARGET GeneratedComponentSDK::Runtime PROPERTY
  INTERFACE_LINK_LIBRARIES GeneratedComponentSDK::Components)
set(GENERATED_COMPONENT_SDK_ROOT "${_GENERATED_COMPONENT_SDK_ROOT}")
set(GENERATED_COMPONENT_SDK_MANIFEST "${_GENERATED_COMPONENT_SDK_ROOT}/manifest/component-sdk.json")
'@
$cmakePackage = "$sdk/cmake"
New-Item -ItemType Directory -Path $cmakePackage, "$sdk/manifest" -Force | Out-Null
$config | Set-Content -LiteralPath "$cmakePackage/GeneratedComponentSDKConfig.cmake" -Encoding utf8
$version = @'
set(PACKAGE_VERSION "1.0.0")
if(PACKAGE_FIND_VERSION VERSION_GREATER PACKAGE_VERSION)
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
  if(PACKAGE_FIND_VERSION VERSION_EQUAL PACKAGE_VERSION)
    set(PACKAGE_VERSION_EXACT TRUE)
  endif()
endif()
'@
$version | Set-Content -LiteralPath "$cmakePackage/GeneratedComponentSDKConfigVersion.cmake" -Encoding utf8
$readme = @'
# GeneratedComponentSDK

这是供 Qt 5 C++ 开发项目使用的组件 SDK。目录内不包含演示程序。

```cmake
set(GeneratedComponentSDK_DIR "<SDK目录>/cmake")
find_package(GeneratedComponentSDK 1.0 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE GeneratedComponentSDK::Components)
```

```cpp
#include <MyDesignSystem/MyDesignSystem.h>
```

组件样式、状态和默认 Token 资源已编入静态库；外部 Token Profile 可通过 `ComponentTheme` 加载。
'@
$readme | Set-Content -LiteralPath "$sdk/README.md" -Encoding utf8
New-Item -ItemType Directory -Path "$sdk/resources/tokens/default", "$sdk/resources/mappings" -Force | Out-Null
Get-ChildItem -LiteralPath "$kit/design-assets/generated" -Force |
    Copy-Item -Destination "$sdk/resources/tokens/default" -Recurse -Force
Copy-Item -LiteralPath "$sourceRoot/Qt/MappingWorkbench/mapping/qt-component-map.json" -Destination "$sdk/resources/mappings/qt-component-map.json"
Copy-Item -LiteralPath "$sourceRoot/Qt/MappingWorkbench/mapping/qt-native-catalog.json" -Destination "$sdk/resources/mappings/qt-native-catalog.json"
Copy-Item -LiteralPath "$kit/component-definitions/generated-component-source-map.json" -Destination "$sdk/resources/mappings/component-source-map.json"
Copy-Item -LiteralPath "$kit/design-assets/tokens/figma-mapping.json" -Destination "$sdk/resources/mappings/design-component-map.json"
$files = Get-ChildItem -LiteralPath $sdk -File -Recurse | Where-Object { $_.Name -ne 'component-sdk.json' }
$manifest = [ordered]@{
    schemaVersion = 1
    package = 'GeneratedComponentSDK'
    owner = 'DesignToCode_GenerateComponent'
    componentAuthority = $true
    tokenAuthority = $true
    linkage = 'static'
    cmakeTarget = 'GeneratedComponentSDK::Components'
    umbrellaHeader = 'include/MyDesignSystem/MyDesignSystem.h'
    componentMap = 'resources/mappings/qt-component-map.json'
    nativeCapabilityCatalog = 'resources/mappings/qt-native-catalog.json'
    componentSourceMap = 'resources/mappings/component-source-map.json'
    designMap = 'resources/mappings/design-component-map.json'
    profiles = @('light-comfortable','light-compact','dark-comfortable','dark-compact')
    files = @($files | ForEach-Object {
        [ordered]@{path=$_.FullName.Substring($sdk.Length+1).Replace('\','/'); sha256=(Get-FileHash -LiteralPath $_.FullName).Hash; bytes=$_.Length}
    })
}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$sdk/manifest/component-sdk.json" -Encoding utf8
Write-Output $sdk
