param(
    [string]$QtRoot = $env:QT_ROOT,
    [string]$BuildDirectory,
    [string]$Cmake = 'cmake',
    [string]$Generator = 'Ninja',
    [switch]$Deploy,
    [switch]$Test
)
$ErrorActionPreference = 'Stop'
$SourceRoot = $PSScriptRoot
$ProjectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../..'))
$env:QT_ROOT = $QtRoot
. "$PSScriptRoot/../../../../Source/NativeTools.ps1"
Initialize-NativeTools
# 所有工具路径由调用方或开发环境提供，脚本可随项目迁移到另一台机器。
if (-not $QtRoot -or -not (Test-Path -LiteralPath (Join-Path $QtRoot 'lib/cmake/Qt5'))) {
    throw 'Pass -QtRoot or set QT_ROOT to a Qt 5.15 MSVC installation.'
}
if (-not $BuildDirectory) {
    $BuildDirectory = Join-Path $ProjectRoot 'build/cache/ReferencePage'
}
$BuildDirectory = Assert-BuildOutput $BuildDirectory
# 将产物限定在源码目录之外，避免 CMake 缓存和部署 DLL 混入维护文件。
if ($BuildDirectory -eq $SourceRoot -or $BuildDirectory.StartsWith($SourceRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'BuildDirectory must be outside Source.'
}
& $Cmake -S $SourceRoot -B $BuildDirectory -G $Generator `
    -DCMAKE_BUILD_TYPE=Release "-DCMAKE_PREFIX_PATH=$QtRoot" `
    "-DBUILD_TESTING=$(if($Test){'ON'}else{'OFF'})" -DREFERENCE_BUILD_APP=ON `
    "-DTEST_OUTPUT_DIR=$ProjectRoot/build/reports/ReferencePage" '-DBUTTON_MSVC_INCLUDE_PREFIX=注意: 包含文件:'
if ($LASTEXITCODE -ne 0) { throw 'Reference configure failed.' }
& $Cmake --build $BuildDirectory --config Release --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Reference build failed.' }
if ($Test) {
    & $NativeCTest --test-dir $BuildDirectory -C Release -j 4 --output-on-failure --output-junit "$ProjectRoot/build/reports/ReferencePage/ctest.xml"
    if ($LASTEXITCODE) { throw 'Reference tests failed.' }
}
& $Cmake --install $BuildDirectory --config Release --prefix (Join-Path $ProjectRoot 'build/demo/page_examples')
if ($LASTEXITCODE -ne 0) { throw 'Reference install failed.' }
$Exe = Join-Path $ProjectRoot 'build/demo/page_examples/ReferencePage/reference_app.exe'
Copy-GeneratedComponentRuntime -Destination (Split-Path $Exe -Parent)
# 部署属于打包动作：把匹配的 Qt 插件和运行库放到 EXE 旁边，运行时无需开发环境。
if ($Deploy) {
    Deploy-NativeQt -Exe $Exe
}
Write-Output $Exe
