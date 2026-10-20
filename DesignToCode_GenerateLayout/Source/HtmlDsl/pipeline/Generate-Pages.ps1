# 路线私有静态编译步骤：只处理调用方提供的页面清单，不主动读取任何路线目录。
param(
    [Parameter(Mandatory)][System.Collections.IDictionary]$Pages,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$MappingDirectory,
    [Parameter(Mandatory)][string]$Font,
    [string]$LibraryProfile,
    [string]$ToolRoot,
    [string]$WorkRoot
)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/../../NativeTools.ps1"
Initialize-LayoutRuntime -ToolRoot $ToolRoot -WorkRoot $WorkRoot
$D2CProjectRoot = $env:D2C_WORK_ROOT
$buildRoot = [IO.Path]::GetFullPath("$D2CProjectRoot/output/work")
$resolvedOutput = Assert-BuildOutput $OutputDirectory
if ($resolvedOutput -ne $buildRoot -and -not $resolvedOutput.StartsWith("$buildRoot$([IO.Path]::DirectorySeparatorChar)", [StringComparison]::OrdinalIgnoreCase)) {
    throw 'OUTPUT_MUST_BE_IN_BUILD'
}
$compiler = "$env:D2C_RUNTIME_ROOT/bin/v2_ui_compiler.exe"
# 先要求编译器存在；构建由 Qt 工程负责，解析入口保持为明确的数据转换步骤。
if (-not (Test-Path -LiteralPath $compiler)) { throw '请先执行 Source/Build.ps1' }
$previousPath = $env:PATH
$previousPlatform = $env:QT_QPA_PLATFORM
try {
    $env:QT_QPA_PLATFORM = 'offscreen'
    New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
    # 按路线声明逐页编译；输入和目标前缀由路线提供，路线私有层只负责规则转换与 XML 检查。
    foreach ($name in $Pages.Keys) {
        if ($name -notmatch '^[A-Za-z0-9][A-Za-z0-9_-]*$') { throw 'INVALID_PAGE_NAME' }
        $profileArgs = @()
        if ($LibraryProfile) { $profileArgs = @('--library-profile', $LibraryProfile) }
        & $compiler --raw $Pages[$name] --mapping $MappingDirectory --extensions "$MappingDirectory/extensions.json" --font $Font --output "$OutputDirectory/$name" @profileArgs
        # 单页失败立即停止，避免后续部署使用不完整的一组页面。
        if ($LASTEXITCODE -ne 0) { throw "UI generation failed: $name" }
        & "$env:D2C_RUNTIME_ROOT/bin/uic.exe" "$OutputDirectory/$name.ui" | Out-Null
        # uic 只验证 XML，标准输出被丢弃，编译头文件不进入交付目录。
        if ($LASTEXITCODE -ne 0) { throw "UI XML validation failed: $name" }
    }
} finally {
    $env:PATH = $previousPath
    $env:QT_QPA_PLATFORM = $previousPlatform
}
