# 路线 B2：结果页由 HTML 布局和同源 DSL 身份合成；入口页沿用显式声明的完整 DSL。
# 页面结构和组件身份均从本目录的同源输入生成，不读取其他路线。
param(
    [string]$OutputDirectory = "$PSScriptRoot/generated",
    [switch]$Formal,
    [switch]$Preview,
    [string]$CaptureManifest,
    [string]$MappingDirectory,
    [string]$Font,
    [string]$Batch,
    [string]$Page
)
$ErrorActionPreference = 'Stop'
if ($Formal -and $Preview) { throw 'FORMAL_AND_PREVIEW_ARE_EXCLUSIVE' }
if ($Preview) {
    foreach ($name in @('CaptureManifest','MappingDirectory','Font','Batch','Page')) {
        if ($PSBoundParameters.ContainsKey($name)) { throw 'PREVIEW_DOES_NOT_ACCEPT_FORMAL_ARGUMENTS' }
    }
}
# 默认只生成正式交付；历史 QUiLoader 演示必须显式选择 Preview。
# 保留 Formal 开关兼容前轮已记录命令，不再把无开关调用解释为旧预览。
$Formal = -not $Preview
if ($Formal) {
    # 正式输出位置由工具决定，业务仓库不是合法参数。
    if ($PSBoundParameters.ContainsKey('OutputDirectory')) { throw 'FORMAL_OUTPUT_IS_TOOL_OWNED' }
    foreach ($value in @($Batch,$Page)) {
        if (-not $value -or $value -notmatch '^[A-Za-z0-9][A-Za-z0-9_-]*$') { throw 'FORMAL_BATCH_AND_PAGE_REQUIRED' }
    }
    foreach ($file in @($CaptureManifest,$Font)) {
        if (-not $file -or -not (Test-Path -LiteralPath $file -PathType Leaf)) { throw 'FORMAL_INPUT_REQUIRED' }
    }
    if (-not $MappingDirectory -or -not (Test-Path -LiteralPath "$MappingDirectory/component-mapping.json")) { throw 'FORMAL_MAPPING_REQUIRED' }
    $mapping = Get-Content -Raw -LiteralPath "$MappingDirectory/component-mapping.json" | ConvertFrom-Json
    if ($mapping.profile -ne 'light-compact') { throw 'FORMAL_REQUIRES_LIGHT_COMPACT_MAPPING' }
}
. "$PSScriptRoot/../NativeTools.ps1"
$previousNodePath = $env:NODE_PATH
Initialize-NativeTools -NeedNode
$pipeline = [IO.Path]::GetFullPath("$PSScriptRoot/pipeline")
$node = $NativeNode
if ($Formal) {
    # 适配器也会写缓存和报告；在它启动前检查路径，不能只依赖后段 C++ 发布检查。
    foreach ($relative in @('build/cache', "build/reports/formal-delivery/$Batch/$Page/adaptation", "build/delivery/$Batch")) {
        $cursor = [IO.Path]::GetFullPath($D2CProjectRoot)
        foreach ($part in $relative.Split('/')) {
            $cursor = Join-Path $cursor $part
            if (Test-Path -LiteralPath $cursor) {
                $item = Get-Item -LiteralPath $cursor -Force
                if (-not $item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'FORMAL_OUTPUT_REDIRECTED' }
            } else { New-Item -ItemType Directory -Path $cursor | Out-Null }
        }
    }
    if (Test-Path -LiteralPath "$D2CProjectRoot/build/delivery/$Batch/$Page") { throw 'FORMAL_OUTPUT_ALREADY_EXISTS' }
}
$scratch = [IO.Path]::GetFullPath("$D2CProjectRoot/build/cache/DesignToCodeHTML-transient-" + [guid]::NewGuid().ToString('N'))
try {
    if ($Formal) {
        $report = "$D2CProjectRoot/build/reports/formal-delivery/$Batch/$Page/adaptation"
        $compiler = "$D2CProjectRoot/build/cache/V2-html-dsl/v2_ui_compiler.exe"
        if (-not (Test-Path -LiteralPath $compiler)) { throw '请先执行 Source/Qt/V2/Build.ps1 -CompileOnly' }
        & $node "$PSScriptRoot/tools/adapt_v2.cjs" $scratch $report $CaptureManifest
        if ($LASTEXITCODE -ne 0) { throw 'FORMAL_CAPTURE_ADAPTATION_FAILED' }
        $previousPlatform = $env:QT_QPA_PLATFORM
        try {
            $env:QT_QPA_PLATFORM = 'offscreen'
            & $compiler --raw "$scratch/hybrid-result.json" --mapping $MappingDirectory --extensions "$MappingDirectory/extensions.json" --font $Font --output "$D2CProjectRoot/build/delivery/$Batch/$Page" --strict-layout --formal
            if ($LASTEXITCODE -ne 0) { throw "FORMAL_GENERATION_FAILED: $Page" }
        } finally { $env:QT_QPA_PLATFORM = $previousPlatform }
        return
    }
    & $node "$PSScriptRoot/tools/adapt_v2.cjs" $scratch
    # HTML 测量失败时停止编译，避免把缺字体或节点对应错误的数据用于 Qt 布局。
    if ($LASTEXITCODE -ne 0) { throw 'HTML + DSL adaptation failed' }
    $pages = [ordered]@{
        review = "$pipeline/design-inputs/review-page/canonical.raw.json"
        'result-html' = "$scratch/hybrid-result.json"
    }
    & "$pipeline/Generate-Pages.ps1" -Pages $pages -OutputDirectory $OutputDirectory
} finally {
    $env:NODE_PATH = $previousNodePath
    # 只清理由当前调用创建的缓存目录；解析绝对路径后限制父目录、前缀和链接属性。
    if (Test-Path -LiteralPath $scratch) {
        $resolved = (Resolve-Path -LiteralPath $scratch).Path
        if ([IO.Path]::GetDirectoryName($resolved) -ne [IO.Path]::GetFullPath("$D2CProjectRoot/build/cache") -or [IO.Path]::GetFileName($resolved) -notlike 'DesignToCodeHTML-transient-*') { throw 'Unsafe temporary path' }
        if ((Get-Item -LiteralPath $resolved).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Temporary path is a link' }
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}

# 仪表宿主随本路线复制，保证另一条路线的模板调整不会改变本次生成。
Copy-Item "$pipeline/compiler/instrument_host.*" -Destination $OutputDirectory
