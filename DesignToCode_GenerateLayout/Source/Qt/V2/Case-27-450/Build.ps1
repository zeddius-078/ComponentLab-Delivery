# 本实验完整复现入口：读取已保存输入，测量 HTML，生成静态 XML，再编译和部署原生窗口。
param([switch]$Verify)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/../../../NativeTools.ps1"
Initialize-NativeTools -NeedNode
$case = $PSScriptRoot
$d2c = [IO.Path]::GetFullPath("$case/../../..")
$cache = "$D2CProjectRoot/build/cache/Case-27-450"
$build = "$cache/build"
$working = "$cache/working"
$out = "$D2CProjectRoot/build/demo/page_examples/Case-27-450"
$node = $NativeNode
$previousPath=$env:PATH; $previousNode=$env:NODE_PATH; $previousPlatform=$env:QT_QPA_PLATFORM
try {
    $env:QT_QPA_PLATFORM='offscreen'
    New-Item -ItemType Directory -Force "$out/data", "$case/generated", "$D2CProjectRoot/build/reports/Case-27-450" | Out-Null
    & $NativeCMake -S $case -B $build -G Ninja '-DCMAKE_BUILD_TYPE=Release' "-DCMAKE_PREFIX_PATH=$env:QT_ROOT"
    # 每一步失败即停止，防止旧文件被误当成本轮输出。
    if($LASTEXITCODE -ne 0) { throw 'Configure failed' }
    & $NativeCMake --build $build --target v2_ui_compiler case_27_450
    if($LASTEXITCODE -ne 0) { throw 'Build failed' }
    & $node "$case/tools/adapt.cjs" $working
    if($LASTEXITCODE -ne 0) { throw 'HTML adaptation failed' }
    $compiler="$build/v2/v2_ui_compiler.exe"
    $font="$case/inputs/assets/HarmonyOS_Sans_SC_Regular.ttf"
    # 两次编译使用完全相同输入，第二次输出留在缓存中，仅用于字节比较。
    foreach($prefix in @("$case/generated/page","$working/repeat")) {
        & $compiler --raw "$working/hybrid-result.json" --mapping "$d2c/HtmlDsl/pipeline/mapping" --extensions "$d2c/HtmlDsl/pipeline/mapping/extensions.json" --font $font --output $prefix
        if($LASTEXITCODE -ne 0) { throw 'Static generation failed' }
    }
    $hashes=@{}
    # 四类生成物须同时保持一致，避免只比较 XML 而遗漏状态与样式。
    foreach($ext in @('.ui','.qss','.components.json','.design-tree.json')) {
        $hash=(Get-FileHash -LiteralPath "$case/generated/page$ext").Hash
        if($hash -ne (Get-FileHash -LiteralPath "$working/repeat$ext").Hash) { throw "Nondeterministic output: $ext" }
        $hashes[$ext]=$hash
        Copy-Item -LiteralPath "$case/generated/page$ext" -Destination "$out/data"
    }
    & "$env:QT_ROOT/bin/uic.exe" "$case/generated/page.ui" | Out-Null
    if($LASTEXITCODE -ne 0) { throw 'Invalid UI XML' }
    Copy-Item -LiteralPath $font -Destination "$out/data"
    $layout=Get-Content -LiteralPath "$D2CProjectRoot/build/reports/Case-27-450/html-result.json" -Raw | ConvertFrom-Json
    # 运行检查只需要测量结果和来源摘要，去掉构建期 URL 与本地资源定位信息。
    @{bindings=$layout.bindings;sourceHtmlSha256=$layout.sourceHtmlSha256;sourceDslSha256=$layout.sourceDslSha256} |
        ConvertTo-Json -Depth 12 | Set-Content -LiteralPath "$out/data/layout-check.json" -Encoding utf8
    @{status='PASS';repeatRuns=2;hashes=$hashes} |
        ConvertTo-Json -Depth 5 | Set-Content -LiteralPath "$D2CProjectRoot/build/reports/Case-27-450/generation.json" -Encoding utf8
    Copy-Item -LiteralPath "$build/case_27_450.exe" -Destination $out
    Copy-GeneratedComponentRuntime -Destination $out
    & "$env:QT_ROOT/bin/windeployqt.exe" --release --no-translations --no-opengl-sw --no-system-d3d-compiler --no-compiler-runtime "$out/case_27_450.exe"
    if($LASTEXITCODE -ne 0) { throw 'Qt deployment failed' }
    Copy-Item "$env:VCToolsRedistDir/x64/Microsoft.VC*.CRT/*.dll" -Destination $out
    Copy-Item -LiteralPath "$case/Run.cmd","$d2c/Qt/V1/qt.conf" -Destination $out
    # 验证时清空开发 PATH 和插件路径，以部署文件启动 Windows 平台原生程序。
    if($Verify) {
        $oldPlugin=$env:QT_PLUGIN_PATH; $oldPlatformPlugin=$env:QT_QPA_PLATFORM_PLUGIN_PATH
        try {
            $env:PATH="$env:SystemRoot/System32;$env:SystemRoot"
            $env:QT_QPA_PLATFORM='windows'
            $env:QT_PLUGIN_PATH=''; $env:QT_QPA_PLATFORM_PLUGIN_PATH=''
            $p=Start-Process -FilePath "$out/case_27_450.exe" -ArgumentList @('--verify', "$D2CProjectRoot/build/reports/Case-27-450") -WorkingDirectory $env:TEMP -WindowStyle Hidden -PassThru
            # 仅结束本次验证进程，避免失败时干扰其他演示窗口。
            if(-not $p.WaitForExit(30000)) { $p.Kill(); throw 'Native verification timed out' }
            if($p.ExitCode -ne 0) { throw "Native verification failed: $($p.ExitCode)" }
        } finally {
            $env:QT_PLUGIN_PATH=$oldPlugin; $env:QT_QPA_PLATFORM_PLUGIN_PATH=$oldPlatformPlugin
        }
        & $node "$case/tools/verify.cjs"
        if($LASTEXITCODE -ne 0) { throw 'Evidence verification failed' }
    }
} finally {
    $env:PATH=$previousPath; $env:NODE_PATH=$previousNode; $env:QT_QPA_PLATFORM=$previousPlatform
}
