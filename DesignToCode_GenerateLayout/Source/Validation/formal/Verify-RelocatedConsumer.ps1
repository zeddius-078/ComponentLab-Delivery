param(
    [Parameter(Mandatory=$true)][string]$Batch,
    [Parameter(Mandatory=$true)][string]$SdkRoot,
    [string]$QtRoot = 'D:\QT\5.15.2\msvc2019_64',
    [string]$CMake = 'D:\QT\Tools\CMake_64\bin\cmake.exe',
    [string]$NinjaRoot = 'D:\QT\Tools\Ninja',
    [string]$VisualStudioRoot = 'D:\VS'
)
$ErrorActionPreference = 'Stop'
if ($Batch -notmatch '^[A-Za-z0-9][A-Za-z0-9_-]*$') { throw 'INVALID_BATCH' }
$project = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$source = Join-Path $project "build\delivery\$Batch"
$report = Join-Path $project "build\reports\formal-delivery\$Batch\relocated"
$isolated = Join-Path $project ('build\cache\consumer-' + [Guid]::NewGuid().ToString('N'))
foreach ($name in @('review','result','horizontal')) {
    if (-not (Test-Path -LiteralPath (Join-Path $source $name))) { throw "MISSING_PAGE: $name" }
}
if (-not (Test-Path -LiteralPath (Join-Path $SdkRoot 'cmake\GeneratedComponentSDKConfig.cmake'))) { throw 'SDK_REQUIRED' }
New-Item -ItemType Directory -Path $isolated,$report -Force | Out-Null
$result = [ordered]@{ status='RUNNING'; batch=$Batch; isolatedRoot=$isolated;
    sourceInputsCopied=$false; traceLogsCopied=$false; originalFontCacheUsed=$false;
    visualParity='NOT_TESTED'; sourcePackageHashes=@();
    sdkManifestSha256=(Get-FileHash -LiteralPath (Join-Path $SdkRoot 'manifest/component-sdk.json') -Algorithm SHA256).Hash }
try {
    # 模拟业务侧手动导入。只复制四件套、资源和发布 SDK，不复制生成器/设计树/日志。
    Copy-Item -LiteralPath $source -Destination (Join-Path $isolated 'pages') -Recurse
    Copy-Item -LiteralPath $SdkRoot -Destination (Join-Path $isolated 'sdk') -Recurse
    New-Item -ItemType Directory -Path (Join-Path $isolated 'consumer') | Out-Null
    foreach ($file in @('CMakeLists.txt','ReviewPageTests.cpp','SelectOptionTests.cpp','SelectOptionFixture.ui')) {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot $file) -Destination (Join-Path $isolated 'consumer')
    }
    $result.sourcePackageHashes = @(Get-ChildItem -LiteralPath $source -Recurse -File | ForEach-Object {
        $relative = [IO.Path]::GetRelativePath($source, $_.FullName)
        $original = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        $copied = (Get-FileHash -LiteralPath (Join-Path "$isolated\pages" $relative) -Algorithm SHA256).Hash
        if ($original -ne $copied) { throw "COPY_HASH_MISMATCH: $relative" }
        @{path=$relative; sha256=$original}
    })
    & "$VisualStudioRoot\Common7\Tools\Launch-VsDevShell.ps1" -VsInstallationPath $VisualStudioRoot -Arch amd64 -HostArch amd64 -SkipAutomaticLocation -NoLogo
    $env:PATH="$QtRoot\bin;$NinjaRoot;"+$env:PATH
    & $CMake -S "$isolated\consumer" -B "$isolated\build" -G Ninja '-DCMAKE_BUILD_TYPE=Release' "-DCMAKE_PREFIX_PATH=$QtRoot" "-DGeneratedComponentSDK_DIR=$isolated/sdk/cmake" "-DFORMAL_PAGE_ROOT=$isolated/pages/review" *> "$report\configure.txt"
    if ($LASTEXITCODE -ne 0) { throw 'RELOCATED_CONFIGURE_FAILED' }
    & $CMake --build "$isolated\build" *> "$report\build.txt"
    if ($LASTEXITCODE -ne 0) { throw 'RELOCATED_BUILD_FAILED' }
    $ninja = Get-Content -Raw -LiteralPath "$isolated\build\build.ninja"
    foreach ($forbidden in @($source,$SdkRoot,(Join-Path $project 'Source\HtmlDsl'))) {
        if ($ninja.Replace('\','/').Contains($forbidden.Replace('\','/'))) { throw "ORIGINAL_PATH_DEPENDENCY: $forbidden" }
    }
    # 此轮验证可移植运行，不把字体回退或未提供设计对照当成视觉一致性通过。
    Remove-Item Env:FORMAL_TEST_FONT,Env:FORMAL_GEOMETRY_REFERENCE -ErrorAction SilentlyContinue
    $env:FORMAL_TEST_REPORT=$report
    $env:QT_QPA_PLATFORM='offscreen'
    & "$isolated\build\formal_page_tests.exe" publicContractAndGeometry businessSignalsAndDisabledState businessLocalStyleOverride nestedLayoutsAndPublicOptions -o "$report\page-tests.txt,txt"
    if ($LASTEXITCODE -ne 0) { throw 'RELOCATED_RUNTIME_FAILED' }
    & "$isolated\build\select_option_tests.exe" -o "$report\select-option-tests.txt,txt"
    if ($LASTEXITCODE -ne 0) { throw 'RELOCATED_SDK_CONTRACT_FAILED' }
    $result.status='PASS'
} catch {
    $result.status='FAIL'; $result.error=$_.Exception.Message
    throw
} finally {
    $result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath "$report\relocation.json" -Encoding utf8
}
Write-Output "PASS: $report\relocation.json"
