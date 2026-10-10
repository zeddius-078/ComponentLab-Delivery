# 构建唯一的 HTML + DSL 静态页面生成链和 XML 运行器。
param([switch]$Test, [switch]$CompileOnly)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/../../NativeTools.ps1"
Initialize-NativeTools
$root = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$folder = 'HtmlDsl'
$build = "$D2CProjectRoot/build/cache/V2-html-dsl"
$out = Join-Path $D2CProjectRoot 'build/demo/page_examples/V2'
New-Item -ItemType Directory -Force $out, "$D2CProjectRoot/build/reports/v2" | Out-Null

& $NativeCMake -S $PSScriptRoot -B $build -G Ninja '-DCMAKE_BUILD_TYPE=Release' `
    "-DCMAKE_PREFIX_PATH=$env:QT_ROOT" "-DD2C_ROUTE_FOLDER=$folder"
if ($LASTEXITCODE -ne 0) { throw 'Configure failed: HTML + DSL' }
& $NativeCMake --build $build --target v2_ui_compiler d2c_v2 v2_tests
if ($LASTEXITCODE -ne 0) { throw 'Build failed: HTML + DSL' }
Copy-Item -LiteralPath "$build/d2c_v2.exe" -Destination "$out/d2c_v2.exe"
Copy-GeneratedComponentRuntime -Destination $out
if ($CompileOnly) { return }

& "$root/$folder/Generate.ps1" -Preview
$target = "$out/data/html-dsl"
New-Item -ItemType Directory -Force $target | Out-Null
# 生成器内部保留 result-html 文件名；部署层统一为 result，运行时无需理解采集格式。
foreach ($page in @('review','result','instrument_host')) {
    $sourceName = if ($page -eq 'result') { 'result-html' } else { $page }
    $extensions = if ($page -eq 'instrument_host') {
        @('.ui','.qss','.components.json')
    } else {
        @('.ui','.qss','.components.json','.design-tree.json')
    }
    foreach ($extension in $extensions) {
        Copy-Item -LiteralPath "$root/$folder/generated/$sourceName$extension" `
            -Destination "$target/$page$extension"
    }
}
Copy-Item -LiteralPath "$PSScriptRoot/rules/workflow.json", `
    "$root/$folder/pipeline/resources/HarmonyOS_Sans_SC_Regular.ttf" -Destination $target

if ($Test) {
    $savedData=$env:D2C_V2_DATA; $savedResult=$env:D2C_V2_RESULT
    $savedReport=$env:D2C_V2_REPORT; $savedPlatform=$env:QT_QPA_PLATFORM
    try {
        $env:QT_QPA_PLATFORM='offscreen'
        $env:D2C_V2_DATA=$target
        $env:D2C_V2_RESULT="$target/result"
        $env:D2C_V2_REPORT="$D2CProjectRoot/build/reports/v2/html-dsl"
        New-Item -ItemType Directory -Force $env:D2C_V2_REPORT | Out-Null
        & "$build/v2_tests.exe" -o "$env:D2C_V2_REPORT/qt-results.xml,xml"
        if ($LASTEXITCODE -ne 0) { throw 'Tests failed: HTML + DSL' }
    } finally {
        $env:D2C_V2_DATA=$savedData; $env:D2C_V2_RESULT=$savedResult
        $env:D2C_V2_REPORT=$savedReport; $env:QT_QPA_PLATFORM=$savedPlatform
    }
}
