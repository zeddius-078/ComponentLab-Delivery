# 构建唯一的 HTML + DSL 静态页面生成链和 XML 运行器。
param([switch]$Test, [switch]$CompileOnly)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/../../../../Source/NativeTools.ps1"
# CompileOnly 只需要 Qt/CMake；完整生成和测试还会执行 HTML + DSL 适配器，
# 因此在进入生成链路前显式初始化 Node/Playwright 模块路径。
if ($CompileOnly) {
    Initialize-NativeTools
} else {
    Initialize-NativeTools -NeedNode
}
$root = [IO.Path]::GetFullPath("$PSScriptRoot/../../../../Source")
$folder = 'HtmlDsl'
$build = "$D2CProjectRoot/build/cache/V2-demo"
& "$D2CSourceRoot/Build.ps1"
$out = Join-Path $D2CProjectRoot 'build/demo/page_examples/V2'
New-Item -ItemType Directory -Force $out, "$D2CProjectRoot/build/reports/v2" | Out-Null

& $NativeCMake -S $PSScriptRoot -B $build -G Ninja '-DCMAKE_BUILD_TYPE=Release' `
    "-DCMAKE_PREFIX_PATH=$env:QT_ROOT" "-DD2C_ROUTE_FOLDER=$folder"
if ($LASTEXITCODE -ne 0) { throw 'Configure failed: HTML + DSL' }
& $NativeCMake --build $build --target layout_generator_demo d2c_v2 v2_tests
if ($LASTEXITCODE -ne 0) { throw 'Build failed: HTML + DSL' }
Copy-Item -LiteralPath "$build/d2c_v2.exe" -Destination "$out/d2c_v2.exe"
Copy-GeneratedComponentRuntime -Destination $out
if ($CompileOnly) { return }

# 统一经过 V2 包装入口，由包装入口显式绑定结果页和复核页 capture manifest。
& "$PSScriptRoot/Generate.ps1"
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
        Copy-Item -LiteralPath "$D2CProjectRoot/build/preview/html-dsl/$sourceName$extension" `
            -Destination "$target/$page$extension"
    }
}
Copy-Item -LiteralPath "$PSScriptRoot/rules/workflow.json", `
    "$D2CProjectRoot/docs/examples/resources/HarmonyOS_Sans_SC_Regular.ttf" -Destination $target

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
