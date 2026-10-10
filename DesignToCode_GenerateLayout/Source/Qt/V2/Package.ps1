# Windows 离线包：只发布 HTML + DSL 路线的运行器与静态页面数据。
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/../../NativeTools.ps1"
Initialize-NativeTools
& "$PSScriptRoot/Build.ps1" -Test
$out = Join-Path $D2CProjectRoot 'build/demo/page_examples/V2'
& "$env:QT_ROOT/bin/windeployqt.exe" --release --no-translations --no-opengl-sw `
    --no-system-d3d-compiler --no-compiler-runtime "$out/d2c_v2.exe"
if ($LASTEXITCODE -ne 0) { throw 'Qt deployment failed: HTML + DSL' }
Copy-Item "$env:VCToolsRedistDir/x64/Microsoft.VC*.CRT/*.dll" -Destination $out
Copy-GeneratedComponentRuntime -Destination $out
Copy-Item -LiteralPath "$PSScriptRoot/Run-V2.cmd", "$PSScriptRoot/Run-HTML-DSL.cmd", `
    "$PSScriptRoot/../V1/qt.conf" -Destination $out
Write-Output $out
