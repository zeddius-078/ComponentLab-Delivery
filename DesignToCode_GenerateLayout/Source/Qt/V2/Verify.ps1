# 完整验证：构建 HTML + DSL 页面，执行测试，再进行 Windows 离线启动。
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/../../NativeTools.ps1"
Initialize-NativeTools -NeedNode
& "$PSScriptRoot/Package.ps1"
$root = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$report = "$D2CProjectRoot/build/reports/v2"
$savedPath=$env:PATH; $savedPlatform=$env:QT_QPA_PLATFORM
$savedPlugin=$env:QT_PLUGIN_PATH; $savedPlatformPlugin=$env:QT_QPA_PLATFORM_PLUGIN_PATH
try {
    $env:QT_QPA_PLATFORM='windows'
    $env:QT_PLUGIN_PATH=''; $env:QT_QPA_PLATFORM_PLUGIN_PATH=''
    $env:PATH="$env:SystemRoot/System32;$env:SystemRoot"
    $destination="$report/offline-html-dsl"
    $p=Start-Process -FilePath "$D2CProjectRoot/build/demo/page_examples/V2/d2c_v2.exe" `
        -ArgumentList @('--data', "$D2CProjectRoot/build/demo/page_examples/V2/data/html-dsl", `
        '--smoke-report', $destination) -WorkingDirectory $env:TEMP -WindowStyle Hidden -PassThru
    if(-not $p.WaitForExit(30000)) { $p.Kill(); throw 'Offline timeout: HTML + DSL' }
    if($p.ExitCode -ne 0) { throw "Offline failed: HTML + DSL ($($p.ExitCode))" }
} finally {
    $env:PATH=$savedPath; $env:QT_QPA_PLATFORM=$savedPlatform
    $env:QT_PLUGIN_PATH=$savedPlugin; $env:QT_QPA_PLATFORM_PLUGIN_PATH=$savedPlatformPlugin
}
& $NativeNode "$root/Validation/check_v2.cjs"
# 汇总检查还需验证输入、生成结果与运行副本，失败时阻止整体通过。
if($LASTEXITCODE -ne 0) { throw 'Result checks failed' }
