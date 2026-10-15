param(
    [string]$SdkRoot,
    [string]$QtRoot = $env:QT_ROOT,
    [string]$Cmake = $(if ($env:CMAKE_EXE) { $env:CMAKE_EXE } else { 'cmake' }),
    [string]$BuildDirectory
)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if (-not $SdkRoot) { $SdkRoot = Join-Path $projectRoot 'build/sdk' }
$SdkRoot = [IO.Path]::GetFullPath($SdkRoot)
$build = if ($BuildDirectory) {
    [IO.Path]::GetFullPath($BuildDirectory)
} else {
    Join-Path $projectRoot 'build/cache/SDKConsumerTest'
}
$report = Join-Path $projectRoot 'build/reports/sdk-consumer'

if (-not (Test-Path -LiteralPath "$SdkRoot/cmake/GeneratedComponentSDKConfig.cmake")) {
    throw "GeneratedComponentSDK package is missing: $SdkRoot"
}
if (-not $QtRoot -or -not (Test-Path -LiteralPath "$QtRoot/lib/cmake/Qt5")) {
    throw 'Set QT_ROOT or pass -QtRoot for Qt 5.15 MSVC x64.'
}

. "$PSScriptRoot/../NativeTools.ps1"
$env:QT_ROOT = $QtRoot
Initialize-NativeTools
New-Item -ItemType Directory -Path $report -Force | Out-Null

& $Cmake -S $PSScriptRoot -B $build -G Ninja '-DCMAKE_BUILD_TYPE=Release' `
    "-DCMAKE_PREFIX_PATH=$QtRoot" "-DGeneratedComponentSDK_DIR=$SdkRoot/cmake"
if ($LASTEXITCODE) { throw 'SDK consumer configure failed.' }
& $Cmake --build $build --parallel 4
if ($LASTEXITCODE) { throw 'SDK consumer build failed.' }
& $NativeCTest --test-dir $build --output-on-failure --output-junit "$report/ctest.xml"
if ($LASTEXITCODE) { throw 'SDK consumer runtime test failed.' }

Write-Output "$build/sdk_consumer_smoke.exe"

