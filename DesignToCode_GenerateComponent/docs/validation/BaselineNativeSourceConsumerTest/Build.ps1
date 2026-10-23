param(
    [string]$PackageRoot,
    [string]$QtRoot = $env:QT_ROOT,
    [string]$Cmake = $(if ($env:CMAKE_EXE) { $env:CMAKE_EXE } else { 'cmake' }),
    [string]$BuildDirectory
)
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
if (-not $PackageRoot) { $PackageRoot = Join-Path $projectRoot 'Source/ComponentKit/baseline-sdk' }
$PackageRoot = [IO.Path]::GetFullPath($PackageRoot)
$build = if ($BuildDirectory) {
    [IO.Path]::GetFullPath($BuildDirectory)
} else {
    Join-Path $projectRoot 'output/work/cache/BaselineNativeSourceConsumerTest'
}
$report = Join-Path $projectRoot 'output/logs/baseline-native-source-consumer'

if (-not (Test-Path -LiteralPath "$PackageRoot/CMakeLists.txt")) {
    throw "Baseline source ComponentPackage is missing: $PackageRoot"
}
if (-not $QtRoot -or -not (Test-Path -LiteralPath "$QtRoot/lib/cmake/Qt5")) {
    throw 'Set QT_ROOT or pass -QtRoot for Qt 5.15 MSVC x64.'
}

. "$PSScriptRoot/../../../Source/NativeTools.ps1"
$env:QT_ROOT = $QtRoot
Initialize-NativeTools
$build = Assert-BuildOutput $build
New-Item -ItemType Directory -Path $report -Force | Out-Null

& $Cmake -S $PSScriptRoot -B $build -G Ninja '-DCMAKE_BUILD_TYPE=Release' `
    "-DCMAKE_PREFIX_PATH=$QtRoot" "-DBaselineNativeComponentPackage=$PackageRoot"
if ($LASTEXITCODE) { throw 'Baseline source consumer configure failed.' }
& $Cmake --build $build --parallel 4
if ($LASTEXITCODE) { throw 'Baseline source consumer build failed.' }
& $NativeCTest --test-dir $build --output-on-failure --output-junit "$report/ctest.xml"
if ($LASTEXITCODE) { throw 'Baseline source consumer runtime test failed.' }

Write-Output "$build/baseline_native_source_consumer.exe"
