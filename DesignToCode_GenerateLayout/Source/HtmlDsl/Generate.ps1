# 璺嚎 B2锛氱粨鏋滈〉鐢?HTML 甯冨眬鍜屽悓婧?DSL 韬唤鍚堟垚锛涘叆鍙ｉ〉娌跨敤鏄惧紡澹版槑鐨勫畬鏁?DSL銆?
# 椤甸潰缁撴瀯鍜岀粍浠惰韩浠藉潎浠庢湰鐩綍鐨勫悓婧愯緭鍏ョ敓鎴愶紝涓嶈鍙栧叾浠栬矾绾裤€?
param(
    [string]$OutputDirectory = "",
    [switch]$Formal,
    [switch]$Preview,
    [string]$CaptureManifest,
    [string]$ReviewCaptureManifest,
    [string]$MappingDirectory,
    [string]$Font,
    [string]$Batch,
    [string]$Page,
    [string]$LibraryProfile,
    [ValidateSet('strict','structure-only')][string]$GenerationMode = 'strict',
    [string]$RawDsl,
    [string[]]$ExpandInstance,
    [string[]]$ExpandComponent,
    [string]$ToolRoot,
    [string]$WorkRoot
)
$ErrorActionPreference = 'Stop'
if ($Formal -and $Preview) { throw 'FORMAL_AND_PREVIEW_ARE_EXCLUSIVE' }
if ($GenerationMode -eq 'structure-only') {
    $Formal = $false
    if ($PSBoundParameters.ContainsKey('Formal') -or $Preview) { throw 'STRUCTURE_ONLY_CANNOT_COMBINE_FORMAL_OR_PREVIEW' }
    $hasRaw = $RawDsl -and (Test-Path -LiteralPath $RawDsl -PathType Leaf)
    $hasCapture = $CaptureManifest -and (Test-Path -LiteralPath $CaptureManifest -PathType Leaf)
    if ([bool]$hasRaw -eq [bool]$hasCapture) { throw 'STRUCTURE_ONLY_ONE_INPUT_SOURCE_REQUIRED' }
    if (-not $MappingDirectory -or -not (Test-Path -LiteralPath "$MappingDirectory/component-mapping.json")) { throw 'STRUCTURE_ONLY_MAPPING_REQUIRED' }
    if (-not $Font -or -not (Test-Path -LiteralPath $Font -PathType Leaf)) { throw 'STRUCTURE_ONLY_FONT_REQUIRED' }
    if (-not $OutputDirectory) { throw 'STRUCTURE_ONLY_OUTPUT_REQUIRED' }
    if ($PSBoundParameters.ContainsKey('ReviewCaptureManifest')) { throw 'STRUCTURE_ONLY_REVIEW_CAPTURE_NOT_USED' }
    if ($hasCapture -and (-not $Batch -or -not $Page)) { throw 'STRUCTURE_ONLY_BATCH_AND_PAGE_REQUIRED' }
}
if ($Preview) {
    foreach ($name in @('Batch','Page')) {
        if ($PSBoundParameters.ContainsKey($name)) { throw 'PREVIEW_DOES_NOT_ACCEPT_FORMAL_ARGUMENTS' }
    }
    if (-not $CaptureManifest -or -not $ReviewCaptureManifest) { throw 'PREVIEW_CAPTURE_MANIFESTS_REQUIRED' }
    if (-not $MappingDirectory -or -not $Font) { throw 'PREVIEW_MAPPING_AND_FONT_REQUIRED' }
}
# 榛樿鍙敓鎴愭寮忎氦浠橈紱棰勮涔熷繀椤绘樉寮忓０鏄庢瘡涓〉闈㈢殑閲囬泦娓呭崟銆?
# 淇濈暀 Formal 寮€鍏冲吋瀹瑰墠杞凡璁板綍鍛戒护锛屼笉鍐嶆妸鏃犲紑鍏宠皟鐢ㄨВ閲婁负鏃ч瑙堛€?
$Formal = if ($GenerationMode -eq 'structure-only') { $false } else { -not $Preview }
if ($Formal) {
    # 姝ｅ紡杈撳嚭浣嶇疆鐢卞伐鍏峰喅瀹氾紝涓氬姟浠撳簱涓嶆槸鍚堟硶鍙傛暟銆?
    if ($PSBoundParameters.ContainsKey('OutputDirectory')) { throw 'FORMAL_OUTPUT_IS_TOOL_OWNED' }
    foreach ($value in @($Batch,$Page)) {
        if (-not $value -or $value -notmatch '^[A-Za-z0-9][A-Za-z0-9_-]*$') { throw 'FORMAL_BATCH_AND_PAGE_REQUIRED' }
    }
    if ($PSBoundParameters.ContainsKey('ReviewCaptureManifest')) { throw 'FORMAL_REVIEW_CAPTURE_NOT_SUPPORTED' }
    foreach ($file in @($CaptureManifest,$Font)) {
        if (-not $file -or -not (Test-Path -LiteralPath $file -PathType Leaf)) { throw 'FORMAL_INPUT_REQUIRED' }
    }
    if (-not $MappingDirectory -or -not (Test-Path -LiteralPath "$MappingDirectory/component-mapping.json")) { throw 'FORMAL_MAPPING_REQUIRED' }
    # 杩愯鍓栭潰鐢辫璁′晶 Profile 鏍囪鎴栨槧灏勫０鏄庤В鏋愶紝鍏ュ彛涓嶅啀寮哄埗鍥哄畾鍓栭潰銆?
}
. "$PSScriptRoot/../NativeTools.ps1"
$python = Resolve-ProjectPython
$previousNodePath = $env:NODE_PATH
Initialize-LayoutRuntime -ToolRoot $ToolRoot -WorkRoot $WorkRoot -NeedNode:([bool]$CaptureManifest -or $Formal -or $Preview)
$D2CProjectRoot = $env:D2C_WORK_ROOT
$toolScripts = "$env:D2C_RUNTIME_ROOT/scripts"
$compiler = "$env:D2C_RUNTIME_ROOT/bin/v2_ui_compiler.exe"
$expansionArgs = @()
foreach ($value in @($ExpandInstance)) { if ($value) { $expansionArgs += @('--expand-instance', $value) } }
foreach ($value in @($ExpandComponent)) { if ($value) { $expansionArgs += @('--expand-component', $value) } }
if ($GenerationMode -eq 'structure-only') {
    $resolvedOutput = Assert-BuildOutput $OutputDirectory
    $workRoot = [IO.Path]::GetFullPath("$D2CProjectRoot/output/work")
    if ($resolvedOutput -ne $workRoot -and -not $resolvedOutput.StartsWith("$workRoot$([IO.Path]::DirectorySeparatorChar)", [StringComparison]::OrdinalIgnoreCase)) { throw 'STRUCTURE_ONLY_OUTPUT_MUST_BE_IN_WORK' }
    if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) { throw '璇峰厛鎵ц Source/Build.ps1' }
    $extensions = Join-Path $MappingDirectory 'extensions.json'
    if (-not (Test-Path -LiteralPath $extensions -PathType Leaf)) { throw 'STRUCTURE_ONLY_EXTENSIONS_REQUIRED' }
    $previousPlatform = $env:QT_QPA_PLATFORM
    $scratch = $null
    try {
        $env:QT_QPA_PLATFORM = 'offscreen'
        $profileArgs = @()
        if ($LibraryProfile) { $profileArgs = @('--library-profile', $LibraryProfile) }
        $sourceRaw = $RawDsl
        if ($CaptureManifest) {
            # 璺緞涓€锛圚TML+DSL锛夛細鍏堟牎楠岄噰闆嗘竻鍗曪紝鍐嶇敤娴忚鍣ㄦ祴閲忕敓鎴愮粨鏋勮緭鍏ャ€?
            $manifestPath = [IO.Path]::GetFullPath($CaptureManifest)
            & $python -B "$toolScripts/capture/capture_manifest.py" verify --manifest $manifestPath --mode formal
            if ($LASTEXITCODE -ne 0) { throw "CAPTURE_MANIFEST_INVALID: $manifestPath" }
            $scratch = [IO.Path]::GetFullPath("$D2CProjectRoot/output/work/cache/structure-only-" + [guid]::NewGuid().ToString('N'))
            $report = "$D2CProjectRoot/output/logs/structure-only/$Batch/$Page/adaptation"
            & $NativeNode "$toolScripts/tools/adapt_v2.cjs" $scratch $report $manifestPath
            if ($LASTEXITCODE -ne 0) { throw 'STRUCTURE_ONLY_CAPTURE_ADAPTATION_FAILED' }
            $sourceRaw = "$scratch/hybrid-result.json"
        }
        # `structure-only` is the public PowerShell generation mode.  The
        # compiler has its own, explicit switch name: `--layout-only`.
        # Do not pass the wrapper mode name through to the native binary.
        # 结构模式允许缺失的数值 gap/padding 以确定性的 0 表达，并将字段写入
        # trace；strict 才要求设计侧显式提供每一个布局数值。
        & $compiler --raw ([IO.Path]::GetFullPath($sourceRaw)) --mapping ([IO.Path]::GetFullPath($MappingDirectory)) --extensions ([IO.Path]::GetFullPath($extensions)) --font ([IO.Path]::GetFullPath($Font)) --output $resolvedOutput --layout-only @profileArgs @expansionArgs
        if ($LASTEXITCODE -ne 0) { throw "STRUCTURE_ONLY_GENERATION_FAILED: $resolvedOutput" }
        & "$env:D2C_RUNTIME_ROOT/bin/uic.exe" "$resolvedOutput.ui" | Out-Null
        if ($LASTEXITCODE -ne 0) { throw 'STRUCTURE_ONLY_UI_INVALID' }
        return
    } finally {
        $env:QT_QPA_PLATFORM = $previousPlatform
        if ($scratch -and (Test-Path -LiteralPath $scratch)) {
            $resolved = (Resolve-Path -LiteralPath $scratch).Path
            if ([IO.Path]::GetDirectoryName($resolved) -eq [IO.Path]::GetFullPath("$D2CProjectRoot/output/work/cache") -and
                [IO.Path]::GetFileName($resolved) -like 'structure-only-*') {
                Remove-Item -LiteralPath $resolved -Recurse -Force
            }
        }
    }
}
$previewRoot = [IO.Path]::GetFullPath("$D2CProjectRoot/output/work/preview")
if ($Preview) {
    if (-not $OutputDirectory) { $OutputDirectory = Join-Path $previewRoot 'html-dsl' }
    $resolvedPreviewOutput = Assert-BuildOutput $OutputDirectory
    if ($resolvedPreviewOutput -ne $previewRoot -and -not $resolvedPreviewOutput.StartsWith("$previewRoot$([IO.Path]::DirectorySeparatorChar)", [StringComparison]::OrdinalIgnoreCase)) {
        throw 'PREVIEW_OUTPUT_MUST_BE_IN_BUILD'
    }
}
$pipeline = [IO.Path]::GetFullPath("$PSScriptRoot/pipeline")
$node = $NativeNode
$manifestChecker = "$toolScripts/capture/capture_manifest.py"
$manifestMode = if ($Formal) { 'formal' } else { 'preview' }
# 鐢熸垚鍓嶆牎楠岃法宸ュ叿杈撳叆鍚堝悓锛涢€傞厤鍣ㄥ彧鎺ユ敹宸查€氳繃璺緞銆佹牴鑺傜偣銆佽祫婧愬拰鍝堝笇妫€鏌ョ殑娓呭崟銆?
foreach ($manifest in @($CaptureManifest, $(if ($Preview) { $ReviewCaptureManifest })) | Where-Object { $_ }) {
    $manifestPath = [IO.Path]::GetFullPath($manifest)
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) { throw 'CAPTURE_MANIFEST_REQUIRED' }
    & $python -B $manifestChecker verify --manifest $manifestPath --mode $manifestMode
    if ($LASTEXITCODE -ne 0) { throw "CAPTURE_MANIFEST_INVALID: $manifestPath" }
}
$CaptureManifest = [IO.Path]::GetFullPath($CaptureManifest)
if ($Preview) { $ReviewCaptureManifest = [IO.Path]::GetFullPath($ReviewCaptureManifest) }
if ($Formal) {
    # 閫傞厤鍣ㄤ篃浼氬啓缂撳瓨鍜屾姤鍛婏紱鍦ㄥ畠鍚姩鍓嶆鏌ヨ矾寰勶紝涓嶈兘鍙緷璧栧悗娈?C++ 鍙戝竷妫€鏌ャ€?
    foreach ($relative in @('output/work/cache', "output/logs/formal-delivery/$Batch/$Page/adaptation", "output/delivery/$Batch")) {
        $cursor = [IO.Path]::GetFullPath($D2CProjectRoot)
        foreach ($part in $relative.Split('/')) {
            $cursor = Join-Path $cursor $part
            if (Test-Path -LiteralPath $cursor) {
                $item = Get-Item -LiteralPath $cursor -Force
                if (-not $item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'FORMAL_OUTPUT_REDIRECTED' }
            } else { New-Item -ItemType Directory -Path $cursor | Out-Null }
        }
    }
    if (Test-Path -LiteralPath "$D2CProjectRoot/output/delivery/$Batch/$Page") { throw 'FORMAL_OUTPUT_ALREADY_EXISTS' }
}
$scratch = [IO.Path]::GetFullPath("$D2CProjectRoot/output/work/cache/DesignToCodeHTML-transient-" + [guid]::NewGuid().ToString('N'))
try {
    if ($Formal) {
        $report = "$D2CProjectRoot/output/logs/formal-delivery/$Batch/$Page/adaptation"
        if (-not (Test-Path -LiteralPath $compiler)) { throw '璇峰厛鎵ц Source/Build.ps1' }
    & $node "$toolScripts/tools/adapt_v2.cjs" $scratch $report $CaptureManifest
        if ($LASTEXITCODE -ne 0) { throw 'FORMAL_CAPTURE_ADAPTATION_FAILED' }
        $previousPlatform = $env:QT_QPA_PLATFORM
        try {
            $env:QT_QPA_PLATFORM = 'offscreen'
            $profileArgs = @()
            if ($LibraryProfile) { $profileArgs = @('--library-profile', $LibraryProfile) }
            $formalOutput = "$D2CProjectRoot/output/delivery/$Batch/$Page"
            $formalLogRoot = "$D2CProjectRoot/output/logs/formal-delivery/$Batch/$Page"
            $compilerLog = "$formalLogRoot/compiler.log"
            $runRecord = "$formalLogRoot/formal-run.json"
            $startedAtUtc = [DateTime]::UtcNow.ToString('o')
            # 严格模式只调用受控原生编译器。保留它的输出和退出码，失败后不降级、
            # 不调用其他发射器，也不把诊断文件登记为正式交付。
            & $compiler --raw "$scratch/hybrid-result.json" --mapping $MappingDirectory --extensions "$MappingDirectory/extensions.json" --font $Font --output $formalOutput --strict-layout --formal @profileArgs @expansionArgs 2>&1 | Tee-Object -FilePath $compilerLog
            $compilerExitCode = $LASTEXITCODE
            [ordered]@{
                schema = 'formal-generation-run/v1'
                entry = $PSCommandPath
                compiler = $compiler
                captureManifest = $CaptureManifest
                rawDsl = "$scratch/hybrid-result.json"
                mappingDirectory = [IO.Path]::GetFullPath($MappingDirectory)
                font = [IO.Path]::GetFullPath($Font)
                batch = $Batch
                page = $Page
                mode = 'strict'
                libraryProfile = $LibraryProfile
                output = $formalOutput
                compilerExitCode = $compilerExitCode
                startedAtUtc = $startedAtUtc
                finishedAtUtc = [DateTime]::UtcNow.ToString('o')
                compilerLog = $compilerLog
            } | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath $runRecord -Encoding utf8
            if ($compilerExitCode -ne 0) { throw "FORMAL_GENERATION_FAILED: exitCode=$compilerExitCode; page=$Page; compilerLog=$compilerLog" }
        } finally { $env:QT_QPA_PLATFORM = $previousPlatform }
        return
    }
    $resultScratch = Join-Path $scratch 'result'
    $reviewScratch = Join-Path $scratch 'review'
    $resultReport = "$D2CProjectRoot/output/logs/v2/html-dsl-adaptation/result"
    $reviewReport = "$D2CProjectRoot/output/logs/v2/html-dsl-adaptation/review"
    & $node "$toolScripts/tools/adapt_v2.cjs" $resultScratch $resultReport $CaptureManifest
    # 缁撴灉椤甸€傞厤澶辫触鏃跺仠姝㈢紪璇戯紝閬垮厤鎶婃棫娣峰悎杈撳叆澶嶅埗杩涙柊棰勮銆?
    if ($LASTEXITCODE -ne 0) { throw 'HTML + DSL result adaptation failed' }
    & $node "$toolScripts/tools/adapt_v2.cjs" $reviewScratch $reviewReport $ReviewCaptureManifest
    # 澶嶆牳椤甸€傞厤澶辫触鏃跺悓鏍峰仠姝紝淇濊瘉涓や釜棰勮椤垫潵鑷湰杞槑纭竻鍗曘€?
    if ($LASTEXITCODE -ne 0) { throw 'HTML + DSL review adaptation failed' }
    $pages = [ordered]@{
        review = "$reviewScratch/hybrid-result.json"
        'result-html' = "$resultScratch/hybrid-result.json"
    }
    & "$pipeline/Generate-Pages.ps1" -Pages $pages -OutputDirectory $OutputDirectory -MappingDirectory $MappingDirectory -Font $Font -LibraryProfile $LibraryProfile
} finally {
    $env:NODE_PATH = $previousNodePath
    # 鍙竻鐞嗙敱褰撳墠璋冪敤鍒涘缓鐨勭紦瀛樼洰褰曪紱瑙ｆ瀽缁濆璺緞鍚庨檺鍒剁埗鐩綍銆佸墠缂€鍜岄摼鎺ュ睘鎬с€?
    if (Test-Path -LiteralPath $scratch) {
        $resolved = (Resolve-Path -LiteralPath $scratch).Path
        if ([IO.Path]::GetDirectoryName($resolved) -ne [IO.Path]::GetFullPath("$D2CProjectRoot/output/work/cache") -or [IO.Path]::GetFileName($resolved) -notlike 'DesignToCodeHTML-transient-*') { throw 'Unsafe temporary path' }
        if ((Get-Item -LiteralPath $resolved).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Temporary path is a link' }
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}
