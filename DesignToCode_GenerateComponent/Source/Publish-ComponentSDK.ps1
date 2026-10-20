param(
    [string]$QtRoot = $env:QT_ROOT,
    [string]$BuildDirectory,
    [string]$Cmake = $(if ($env:CMAKE_EXE) { $env:CMAKE_EXE } else { 'cmake' }),
    [string]$Generator = 'Ninja'
)
$ErrorActionPreference = 'Stop'
$sourceRoot = $PSScriptRoot
$projectRoot = Split-Path $sourceRoot -Parent
$kit = Join-Path $projectRoot 'output/work/generated/Source/ComponentKit'
$build = if ($BuildDirectory) { [IO.Path]::GetFullPath($BuildDirectory) } else { Join-Path $projectRoot 'output/work/cache/GeneratedComponentSDK' }
$authoritySdk = Join-Path $projectRoot 'build/sdk'
if (-not $QtRoot -or -not (Test-Path -LiteralPath "$QtRoot/lib/cmake/Qt5")) {
    throw 'Set QT_ROOT or pass -QtRoot for Qt 5.15 MSVC x64.'
}
. "$PSScriptRoot/NativeTools.ps1"
$python = Resolve-ProjectPython
$env:QT_ROOT = $QtRoot
Initialize-NativeTools
$build = Assert-BuildOutput $build
$authoritySdk = Assert-ControlledBuildOutput $authoritySdk
$stageId = [guid]::NewGuid().ToString('N')
$sdk = Assert-BuildOutput (Join-Path $projectRoot "output/work/cache/sdk-stage-$stageId")
$lockPath = Assert-ControlledBuildOutput (Join-Path $projectRoot 'build/.state/operation.lock')
New-Item -ItemType Directory -Path (Split-Path $lockPath) -Force | Out-Null
# FileShare.None is released by the OS even when the publisher process crashes.
# Keep the file itself: deleting a lock filename can race another publisher.
try {
    $publishLock = [IO.File]::Open($lockPath, [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
} catch {
    throw "SDK publication is already locked; no output was changed: $lockPath"
}
try {
New-Item -ItemType Directory -Path (Split-Path $sdk) -Force | Out-Null
$snapshot = Assert-BuildOutput (Join-Path $projectRoot "output/work/cache/sdk-input-$stageId.json")
& $python -B "$sourceRoot/SDKPackage.py" fingerprint --project-root $projectRoot |
    Set-Content -LiteralPath $snapshot -Encoding utf8
if ($LASTEXITCODE) { throw 'SDK source fingerprint failed.' }
& $python -B "$sourceRoot/PrepareBuild.py"
if ($LASTEXITCODE) { throw 'Component build preparation failed.' }
& $Cmake -S $kit -B $build -G $Generator '-DCMAKE_BUILD_TYPE=Release' `
    "-DCMAKE_PREFIX_PATH=$QtRoot" "-DPython3_EXECUTABLE=$python" '-DD2C_BUILD_TOKEN_TOOLS=ON' `
    '-DBUTTON_MSVC_INCLUDE_PREFIX=注意: 包含文件:' '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL'
if ($LASTEXITCODE) { throw 'SDK configure failed.' }
& $Cmake --build $build --config Release --target d2c_generate_tokens
if ($LASTEXITCODE) { throw 'Token generation failed.' }
& $Cmake --build $build --config Release --target d2c_check_tokens
if ($LASTEXITCODE) { throw 'Token check failed.' }
& $Cmake --build $build --config Release --target generated_component_sdk --parallel 4
if ($LASTEXITCODE) { throw 'SDK build failed.' }
& $Cmake --install $build --config Release --prefix $sdk
if ($LASTEXITCODE) { throw 'SDK install failed.' }

$config = @'
if(TARGET GeneratedComponentSDK::Components)
  return()
endif()
include(CMakeFindDependencyMacro)
find_dependency(Qt5 5.15 COMPONENTS Widgets Svg)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(FATAL_ERROR "This SDK is Release /MD. A verified complete Debug /MDd SDK is required for Debug.")
endif()
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
Copy-Item -LiteralPath "$projectRoot/docs/examples/MappingWorkbench/mapping/qt-component-map.json" -Destination "$sdk/resources/mappings/qt-component-map.json"
Copy-Item -LiteralPath "$projectRoot/docs/examples/MappingWorkbench/mapping/qt-native-catalog.json" -Destination "$sdk/resources/mappings/qt-native-catalog.json"
Copy-Item -LiteralPath "$kit/component-definitions/generated-component-source-map.json" -Destination "$sdk/resources/mappings/component-source-map.json"
Copy-Item -LiteralPath "$kit/component-definitions/generated-design-component-catalog.json" -Destination "$sdk/resources/mappings/design-component-catalog.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-gallery-layouts.json" -Destination "$sdk/resources/mappings/figma-gallery-layouts.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-checkbox-gallery.json" -Destination "$sdk/resources/mappings/figma-checkbox-gallery.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-button-visuals.json" -Destination "$sdk/resources/mappings/figma-button-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-progress-visuals.json" -Destination "$sdk/resources/mappings/figma-progress-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-radio-visuals.json" -Destination "$sdk/resources/mappings/figma-radio-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-spinbox-visuals.json" -Destination "$sdk/resources/mappings/figma-spinbox-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-datetime-visuals.json" -Destination "$sdk/resources/mappings/figma-datetime-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-slider-visuals.json" -Destination "$sdk/resources/mappings/figma-slider-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-scrollbar-visuals.json" -Destination "$sdk/resources/mappings/figma-scrollbar-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-dial-visuals.json" -Destination "$sdk/resources/mappings/figma-dial-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-lineedit-visuals.json" -Destination "$sdk/resources/mappings/figma-lineedit-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-shortcut-visuals.json" -Destination "$sdk/resources/mappings/figma-shortcut-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-combo-visuals.json" -Destination "$sdk/resources/mappings/figma-combo-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-commandlink-visuals.json" -Destination "$sdk/resources/mappings/figma-commandlink-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-toolbutton-visuals.json" -Destination "$sdk/resources/mappings/figma-toolbutton-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-dialogactions-visuals.json" -Destination "$sdk/resources/mappings/figma-dialogactions-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-tabs-visuals.json" -Destination "$sdk/resources/mappings/figma-tabs-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-toolbox-visuals.json" -Destination "$sdk/resources/mappings/figma-toolbox-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-dockwidget-visuals.json" -Destination "$sdk/resources/mappings/figma-dockwidget-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-mdi-visuals.json" -Destination "$sdk/resources/mappings/figma-mdi-visuals.json"
Copy-Item -LiteralPath "$kit/component-definitions/figma-pageflow-visuals.json" -Destination "$sdk/resources/mappings/figma-pageflow-visuals.json"
# This file is generated by the V2 compiler from source/figma-bindings and
# source/component-recipes.  Never publish the legacy figma-audit snapshot as
# the runtime map: ComponentLab consumes this artifact directly.
$v2MapPath = "$kit/design-assets/DesignSystem/config/figma-component-map-v2.json"
$v2MapText = Get-Content -Raw -LiteralPath $v2MapPath
if ($v2MapText -match 'qtlab|stoken') {
    throw "V2 component map contains legacy token names: $v2MapPath"
}
Copy-Item -LiteralPath $v2MapPath -Destination "$sdk/resources/mappings/figma-component-map-v2.json"
Copy-Item -LiteralPath "$kit/design-assets/DesignSystem/config/figma-audit-v2.json" -Destination "$sdk/resources/mappings/figma-audit-v2.json"
Copy-Item -LiteralPath "$kit/design-assets/DesignSystem/config/reference-mapping.json" -Destination "$sdk/resources/mappings/reference-mapping.json"
Copy-Item -LiteralPath "$kit/design-assets/DesignSystem/config/manifest.json" -Destination "$sdk/resources/mappings/token-manifest.json"
Copy-Item -LiteralPath "$kit/design-assets/DesignSystem/source/component-registry.json" -Destination "$sdk/resources/mappings/component-registry.json"
# Never partially update sdk-debug: its library and all mappings must be built
# and verified together by a separate Debug publisher.
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
    designComponentCatalog = 'resources/mappings/design-component-catalog.json'
    v2ComponentMap = 'resources/mappings/figma-component-map-v2.json'
    v2ComponentRegistry = 'resources/mappings/component-registry.json'
    tokenManifest = 'resources/mappings/token-manifest.json'
    profiles = @('standard-light','wide-light','standard-dark','wide-dark')
    files = @($files | ForEach-Object {
        [ordered]@{path=$_.FullName.Substring($sdk.Length+1).Replace('\','/'); sha256=(Get-FileHash -LiteralPath $_.FullName).Hash; bytes=$_.Length}
    })
}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$sdk/manifest/component-sdk.json" -Encoding utf8
& $python -B "$sourceRoot/SDKPackage.py" stamp --sdk $sdk --snapshot $snapshot `
    --qt-root $QtRoot --cmake $Cmake --build-directory $build
if ($LASTEXITCODE) { throw 'SDK staged package verification failed; previous SDK is unchanged.' }
# Consumer compiles and runs against the staged package, not producer internals.
& "$projectRoot/docs/validation/SDKConsumerTest/Build.ps1" -SdkRoot $sdk -QtRoot $QtRoot `
    -Cmake $Cmake -BuildDirectory (Join-Path $projectRoot 'output/work/cache/SDKConsumerTest')
& $python -B "$sourceRoot/SDKPackage.py" promote --stage $sdk --destination $authoritySdk
if ($LASTEXITCODE) { throw 'SDK promotion failed; previous package was retained/restored.' }
Write-Output $authoritySdk
} finally {
    $publishLock.Dispose()
}
