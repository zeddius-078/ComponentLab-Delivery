param(
    [string]$QtRoot = $env:QT_ROOT,
    [string]$BuildDirectory,
    [string]$Cmake = $(if ($env:CMAKE_EXE) { $env:CMAKE_EXE } else { 'cmake' }),
    [string]$Generator = 'Ninja'
)

# Publishes only the scope-locked Qt Q* -> commonWidget::Z_* wrappers.
# It intentionally does not invoke PrepareBuild, ComponentFactory, the V2
# compiler, mapping export, or runtime Token JSON staging.
$ErrorActionPreference = 'Stop'
$sourceRoot = $PSScriptRoot
$projectRoot = Split-Path $sourceRoot -Parent
$packageRoot = Join-Path $sourceRoot 'ComponentKit/baseline-sdk'
$scopeSource = Join-Path $sourceRoot 'ComponentKit/component-definitions/base-sdk-scope.json'
$build = if ($BuildDirectory) { [IO.Path]::GetFullPath($BuildDirectory) } else {
    Join-Path $projectRoot 'output/work/cache/BaselineNativeComponentSDK'
}
$authoritySdk = Join-Path $projectRoot 'build/sdk'

if (-not $QtRoot -or -not (Test-Path -LiteralPath "$QtRoot/lib/cmake/Qt5")) {
    throw 'Set QT_ROOT or pass -QtRoot for the existing Qt 5.15 MSVC x64 installation.'
}
if (-not (Test-Path -LiteralPath $packageRoot) -or -not (Test-Path -LiteralPath $scopeSource)) {
    throw 'BASELINE_NATIVE_PACKAGE_INPUT_MISSING'
}

. "$PSScriptRoot/NativeTools.ps1"
$python = Resolve-ProjectPython
$env:QT_ROOT = $QtRoot
Initialize-NativeTools
$build = Assert-BuildOutput $build
$authoritySdk = Assert-ControlledBuildOutput $authoritySdk
$stageId = [guid]::NewGuid().ToString('N')
$stage = Assert-BuildOutput (Join-Path $projectRoot "output/work/cache/baseline-sdk-stage-$stageId")
$lockPath = Assert-ControlledBuildOutput (Join-Path $projectRoot 'build/.state/operation.lock')
New-Item -ItemType Directory -Path (Split-Path $lockPath) -Force | Out-Null

try {
    # FileShare.None is released by Windows if a prior publisher terminates.
    $publishLock = [IO.File]::Open($lockPath, [IO.FileMode]::OpenOrCreate,
        [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
} catch {
    throw "SDK publication is already locked; no output was changed: $lockPath"
}

try {
    New-Item -ItemType Directory -Path (Split-Path $stage) -Force | Out-Null
    $snapshot = Assert-BuildOutput (Join-Path $projectRoot "output/work/cache/baseline-sdk-input-$stageId.json")
    & $python -B "$sourceRoot/ComponentKit/tools/check_base_sdk_scope.py"
    if ($LASTEXITCODE) { throw 'Base SDK scope validation failed.' }
    & $python -B "$sourceRoot/ComponentKit/tools/check_baseline_source_package.py"
    if ($LASTEXITCODE) { throw 'Baseline package boundary validation failed.' }
    & $python -B "$sourceRoot/SDKPackage.py" fingerprint --project-root $projectRoot `
        --package-kind qt-native-baseline | Set-Content -LiteralPath $snapshot -Encoding utf8
    if ($LASTEXITCODE) { throw 'Baseline SDK source fingerprint failed.' }

    & $Cmake -S $packageRoot -B $build -G $Generator '-DCMAKE_BUILD_TYPE=Release' `
        "-DCMAKE_PREFIX_PATH=$QtRoot" '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL'
    if ($LASTEXITCODE) { throw 'Baseline SDK configure failed.' }
    & $Cmake --build $build --config Release --target Components --parallel 4
    if ($LASTEXITCODE) { throw 'Baseline SDK build failed.' }
    & $Cmake --install $build --config Release --prefix $stage
    if ($LASTEXITCODE) { throw 'Baseline SDK install failed.' }

    Copy-Item -LiteralPath $scopeSource -Destination "$stage/manifest/base-sdk-scope.json"
    $scopeHash = (Get-FileHash -LiteralPath "$stage/manifest/base-sdk-scope.json" -Algorithm SHA256).Hash.ToLowerInvariant()
    $readme = @'
# GeneratedComponentSDK — Qt Native Baseline

This is the portable baseline package: 59 direct Qt 5 `Q*` →
`commonWidget::Z_*` wrappers only.  It contains no ComponentFactory, Figma
mapping, runtime Token JSON, composite widget, or producer-workspace path.

## Source consumption

Keep the package structure unchanged and add it as a CMake subdirectory:

```cmake
add_subdirectory(path/to/ComponentPackage)
target_link_libraries(my_app PRIVATE GeneratedComponentSDK::Components)
```

## Binary consumption

```cmake
set(GeneratedComponentSDK_DIR "<ComponentPackage>/cmake")
find_package(GeneratedComponentSDK 2 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE GeneratedComponentSDK::Components)
```

After constructing `QApplication`, call
`commonWidget::initializeNativeComponentResources()` and apply the returned
`commonWidget::nativeComponentStyleSheet()` at the chosen application or page
boundary.  The QSS contains fixed visual values; it reads no JSON at runtime.

The published binary is Release `/MD`.  Debug requires an independently
verified `/MDd` package and is deliberately rejected by this release config.
'@
    $readme | Set-Content -LiteralPath "$stage/README.md" -Encoding utf8
    $manifest = [ordered]@{
        schemaVersion = 1
        package = 'GeneratedComponentSDK'
        owner = 'DesignToCode_GenerateComponent'
        packageKind = 'qt-native-baseline'
        publicationState = 'baseline-native'
        componentAuthority = $true
        tokenAuthority = $false
        visualQssAuthority = $true
        runtimeTokenJson = $false
        legacyRuntime = $false
        linkage = 'static'
        cmakeTarget = 'GeneratedComponentSDK::Components'
        umbrellaHeader = 'include/MyDesignSystem/MyDesignSystem.h'
        nativeScope = 'manifest/base-sdk-scope.json'
        scopeHash = $scopeHash
        nativeComponentCount = 59
        profiles = @('standard-light')
        files = @()
    }
    $manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$stage/manifest/component-sdk.json" -Encoding utf8

    & $python -B "$sourceRoot/SDKPackage.py" stamp --sdk $stage --snapshot $snapshot `
        --qt-root $QtRoot --cmake $Cmake --build-directory $build
    if ($LASTEXITCODE) { throw 'Baseline SDK staged-package verification failed; previous SDK is unchanged.' }

    # Source and binary consumers are independent acceptance routes.  Neither
    # includes any producer runtime source outside ComponentPackage.
    & "$projectRoot/docs/validation/BaselineNativeSourceConsumerTest/Build.ps1" `
        -PackageRoot $packageRoot -QtRoot $QtRoot -Cmake $Cmake `
        -BuildDirectory (Join-Path $projectRoot 'output/work/cache/BaselineNativeSourceConsumerTest')
    if ($LASTEXITCODE) { throw 'Baseline SDK source consumer test failed; previous SDK is unchanged.' }
    & "$projectRoot/docs/validation/BaselineNativeSDKConsumerTest/Build.ps1" `
        -SdkRoot $stage -QtRoot $QtRoot -Cmake $Cmake `
        -BuildDirectory (Join-Path $projectRoot 'output/work/cache/BaselineNativeSDKConsumerTest')
    if ($LASTEXITCODE) { throw 'Baseline SDK independent consumer test failed; previous SDK is unchanged.' }

    & $python -B "$sourceRoot/SDKPackage.py" promote --stage $stage --destination $authoritySdk
    if ($LASTEXITCODE) { throw 'Baseline SDK promotion failed; previous package was retained/restored.' }
    $publishedManifest = Get-Content -Raw -LiteralPath "$authoritySdk/manifest/component-sdk.json" |
        ConvertFrom-Json
    $delivery = Assert-DeliveryOutput (Join-Path $projectRoot (
        "output/delivery/baseline-native-sdk-" + $publishedManifest.releaseId))
    New-Item -ItemType Directory -Path $delivery -Force | Out-Null
    $acceptance = [ordered]@{
        releaseId = $publishedManifest.releaseId
        packageKind = $publishedManifest.packageKind
        packageVersion = $publishedManifest.packageVersion
        manifestSha256 = (Get-FileHash -LiteralPath "$authoritySdk/manifest/component-sdk.json" -Algorithm SHA256).Hash.ToLowerInvariant()
        nativeComponentCount = $publishedManifest.nativeComponentCount
        sourceConsumption = 'PASS: add_subdirectory independent consumer test'
        binaryConsumption = 'PASS: find_package independent consumer test'
        qrcClosure = 'PASS: check_baseline_source_package.py'
        runtimeTokenJson = $publishedManifest.runtimeTokenJson
        legacyRuntime = $publishedManifest.legacyRuntime
        toolchain = $publishedManifest.toolchain
        knownLimitations = @(
            'Release /MD package only; Debug requires a separately built and verified /MDd package.',
            'Only the frozen Qt-native Q* to Z_* scope is published; composite, self-painted, page and experimental components remain outside this SDK.')
        logs = @(
            'output/logs/baseline-native-source-consumer/ctest.xml',
            'output/logs/baseline-native-sdk-consumer/ctest.xml')
    }
    $acceptance | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath "$delivery/acceptance.json" -Encoding utf8
    Write-Output $authoritySdk
} finally {
    $publishLock.Dispose()
}
