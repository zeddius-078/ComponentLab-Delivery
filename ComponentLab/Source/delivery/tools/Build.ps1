param(
    [string]$QtDir = $env:QT_ROOT,
    [string]$VsDir = $env:VS_ROOT,
    [string]$CMake = 'cmake',
    [string]$Python = $env:D2C_PYTHON_EXE,
    [string]$SdkRoot,
    [string]$NinjaDir = $env:NINJA_ROOT,
    [string]$BuildDirectory = '',
    [switch]$CompileOnly,
    [switch]$SkipTests,
    [switch]$SkipDeploy
)
$ErrorActionPreference = 'Stop'
$env:PYTHONDONTWRITEBYTECODE = '1'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
. "$root/ProjectTools.ps1"
function Complete-BuildLogRun {
    param([Parameter(Mandatory)][string]$Logs, [Parameter(Mandatory)][string]$Destination)
    $Logs = Assert-ProjectOutput $Logs -Lane logs
    $Destination = Assert-ProjectOutput $Destination -Lane logs
    if ($Logs -eq $Destination) { throw 'LOG_SOURCE_EQUALS_DESTINATION' }
    foreach ($directory in @($Logs, $Destination)) {
        if ((Test-Path -LiteralPath $directory) -and
            -not (Test-Path -LiteralPath $directory -PathType Container)) {
            throw 'LOG_DIRECTORY_REQUIRED'
        }
    }
    # Validate the new run recursively. A previous run is only renamed to a
    # recoverable output/work path; reading every protected child is neither
    # necessary nor always possible on a machine with inherited log ACLs.
    foreach ($item in Get-ChildItem -LiteralPath $Logs -Recurse -Force) {
        [void](Assert-ProjectOutput $item.FullName -Lane logs)
    }
    if (-not (Test-Path -LiteralPath $Logs -PathType Container)) { throw 'LOG_RUN_MISSING' }
    $obsolete = Assert-ProjectOutput (Join-Path $ComponentLabProject ("output/work/obsolete-log-" + [guid]::NewGuid().ToString('N')))
    [void](New-Item -ItemType Directory -Path (Split-Path $Destination -Parent),(Split-Path $obsolete -Parent) -Force)
    $hadPrevious = Test-Path -LiteralPath $Destination
    if ($hadPrevious) {
        try {
            Move-Item -LiteralPath $Destination -Destination $obsolete -ErrorAction Stop
        } catch {
            # Preserve a protected historical run intact. The current run is
            # still a successful, independently named log under the same lane.
            $fallback = Assert-ProjectOutput (Join-Path (Split-Path $Destination -Parent) (
                'run-' + [guid]::NewGuid().ToString('N'))) -Lane logs
            Move-Item -LiteralPath $Logs -Destination $fallback -ErrorAction Stop
            return $fallback
        }
    }
    try {
        Move-Item -LiteralPath $Logs -Destination $Destination -ErrorAction Stop
    } catch {
        if ($hadPrevious -and -not (Test-Path -LiteralPath $Destination)) {
            Move-Item -LiteralPath $obsolete -Destination $Destination -ErrorAction Stop
        }
        throw
    }
    return $Destination
}
$project = $ComponentLabProject
$Python = Resolve-ProjectPython $Python
$lifecycle = Resolve-SupportTool 'ArtifactLifecycle.py'
. (Resolve-SupportTool 'OperationLock.ps1')
$buildOperation = Enter-ProjectOperationLock -ProjectRoot $project
try {
$SdkRoot = & "$root/Verify-GeneratedComponentSDK.ps1" -Package $SdkRoot
$env:GENERATED_COMPONENT_SDK_ROOT = $SdkRoot
$sdkManifest = Get-Content -LiteralPath (Join-Path $SdkRoot 'manifest/component-sdk.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$isBaselineSdk = $sdkManifest.packageKind -eq 'qt-native-baseline'
$sdkManifestHash = (Get-FileHash -LiteralPath (Join-Path $SdkRoot 'manifest/component-sdk.json') -Algorithm SHA256).Hash
$build = Assert-ProjectOutput $(if ($BuildDirectory) { $BuildDirectory } else { Join-Path $project 'output/work/cache/gallery-release' })
$run = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0,8)
$logs = Assert-ProjectOutput (Join-Path $project "output/logs/build-$run") -Lane logs
$stage = Assert-ProjectOutput (Join-Path $project "output/work/stage/gallery-$run")
$success = Assert-ProjectOutput (Join-Path $project 'output/logs/latest-success/gallery-build.json') -Lane logs
$failure = Assert-ProjectOutput (Join-Path $project 'output/logs/latest-failure/gallery-build.json') -Lane logs
$sourceFingerprint = & $Python -B $lifecycle fingerprint --project $project
if ($LASTEXITCODE -ne 0 -or -not $sourceFingerprint) { throw 'SOURCE_FINGERPRINT_FAILED' }
$sourceFingerprint = $sourceFingerprint.Trim()
$CMake = (Get-Command $CMake -ErrorAction Stop).Source
if (-not $QtDir) { throw 'Set QT_ROOT or provide -QtDir; no environment is installed.' }
$cache = Join-Path $build 'CMakeCache.txt'
if (Test-Path -LiteralPath $cache) {
    $record = Select-String -LiteralPath $cache -Pattern '^(D2C_CONFIGURED_SDK_ROOT:INTERNAL|GENERATED_COMPONENT_SDK_ROOT:PATH)=(.+)$'
    foreach ($line in $record) {
        if ([IO.Path]::GetFullPath($line.Matches[0].Groups[2].Value) -ne $SdkRoot) {
            throw 'SDK_CACHE_CONFLICT: use a fresh output/work cache for a different SDK.'
        }
    }
}
[void](New-Item -ItemType Directory -Path $logs,$stage -Force)
function Invoke-Step([string]$Name, [string]$Program, [string[]]$Arguments) {
    & $Program @Arguments 2>&1 | Tee-Object -FilePath (Join-Path $logs "$Name.log")
    if ($LASTEXITCODE -ne 0) { throw "$Name failed, exit $LASTEXITCODE. Evidence: $logs" }
}
Start-Transcript -LiteralPath (Join-Path $logs 'transcript.log')
$published = $false
$failureRecord = $null
try {
    if ($VsDir) { & (Join-Path $VsDir 'Common7/Tools/Launch-VsDevShell.ps1') -VsInstallationPath $VsDir -Arch amd64 -HostArch amd64 -SkipAutomaticLocation }
    $env:PATH = (Join-Path $QtDir 'bin') + ';' + $NinjaDir + ';' + $env:PATH
    $env:VSLANG = '1033'
    $prefix = -join ([char[]]@(0x6ce8,0x610f,0x003a,0x0020,0x5305,0x542b,0x6587,0x4ef6,0x003a))
    if (Test-Path -LiteralPath (Join-Path $env:VCToolsInstallDir 'bin/Hostx64/x64/1033/clui.dll')) { $prefix = 'Note: including file:' }
    Invoke-Step 'configure' $CMake @('-S',$root,'-B',$build,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release',"-DGENERATED_COMPONENT_SDK_ROOT=$SdkRoot","-DCMAKE_PREFIX_PATH=$QtDir","-DPython3_EXECUTABLE=$Python","-DBUTTON_MSVC_INCLUDE_PREFIX=$prefix","-DTEST_OUTPUT_DIR=$logs/tests",'-DCOMPONENTLAB_BUILD_DEMO=ON','-DBUILD_TESTING=ON')
    Invoke-Step 'compile' $CMake @('--build',$build,'--parallel','4')
    Invoke-Step 'install-stage' $CMake @('--install',$build,'--prefix',$stage)
    if ($CompileOnly -or $SkipTests -or $SkipDeploy) {
        $failureRecord = @{status='UNVERIFIED_STAGE';stage=$stage;stableUnchanged=$true;reason='CompileOnly, SkipTests or SkipDeploy prevents promotion';sourceFingerprint=$sourceFingerprint}
        $failureRecord | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $logs 'stage-only.json') -Encoding utf8
        Write-Output "STAGE ONLY; stable gallery unchanged: $stage"
        return
    }
    Invoke-Step 'governance' $Python @('-B',(Join-Path $root 'delivery/tools/CheckProject.py'),'--dist',$stage,'--snapshot',(Join-Path $logs 'validation-inputs.json'))
    Invoke-Step 'ctest' (Join-Path (Split-Path $CMake) 'ctest.exe') @('--test-dir',$build,'-j','1','--output-on-failure','--output-junit',(Join-Path $logs 'ctest.xml'))
    if (-not $isBaselineSdk) {
        $oldPlatform = $env:QT_QPA_PLATFORM
        # Qt probes do not need system fonts; an empty existing font directory avoids
        # the benign "Cannot find font directory" stderr warning that PowerShell 5.1
        # would otherwise turn into a terminating error.
        $emptyFonts = Assert-ProjectOutput (Join-Path $project 'output/work/fonts-empty')
        [void](New-Item -ItemType Directory -Path $emptyFonts -Force)
        $previousFontDir = $env:QT_QPA_FONTDIR
        try {
            $env:QT_QPA_PLATFORM = 'offscreen'
            $env:QT_QPA_FONTDIR = $emptyFonts
            foreach ($profile in @('standard-light','wide-light','standard-dark','wide-dark')) {
                Invoke-Step "token-$profile" (Join-Path $build 'token_probe.exe') @('--token-dir',(Join-Path $SdkRoot 'resources/tokens/default'),'--profile',$profile,'--output',(Join-Path $logs "token/$profile"))
            }
        } finally {
            $env:QT_QPA_PLATFORM = $oldPlatform
            $env:QT_QPA_FONTDIR = $previousFontDir
        }
    }
    $deployArguments = @('--release','--no-translations','--no-compiler-runtime','--dir',$stage,(Join-Path $stage 'component_lab.exe'))
    if ($isBaselineSdk) {
        # A Widgets-only baseline does not need ANGLE, the software OpenGL
        # rasterizer, D3D compiler or the general image/style plugin set.
        # Keep only the plugins exercised by its public QSS/QRC contract.
        $deployArguments = @('--release','--no-translations','--no-compiler-runtime','--no-plugins',
            '--no-opengl-sw','--no-angle','--no-system-d3d-compiler','--dir',$stage,
            (Join-Path $stage 'component_lab.exe'))
    }
    Invoke-Step 'deploy-stage' (Join-Path $QtDir 'bin/windeployqt.exe') $deployArguments
    # Offscreen platform plugin is required by the accepted verification capture
    # path (see Verify.ps1); deploy it from the same Qt kit so the bundle stays
    # self-contained and no external plugin path is used.
    if ($isBaselineSdk) {
        foreach ($plugin in @('platforms/qwindows.dll','platforms/qoffscreen.dll','imageformats/qsvg.dll')) {
            $sourcePlugin = Join-Path $QtDir (Join-Path 'plugins' $plugin)
            $targetPlugin = Join-Path $stage $plugin
            if (-not (Test-Path -LiteralPath $sourcePlugin)) { throw "Missing required baseline Qt plugin: $sourcePlugin" }
            [void](New-Item -ItemType Directory -Path (Split-Path $targetPlugin -Parent) -Force)
            Copy-Item -LiteralPath $sourcePlugin -Destination $targetPlugin -Force
        }
    } else {
        $offscreenPlugin = Join-Path $QtDir 'plugins/platforms/qoffscreen.dll'
        if (-not (Test-Path -LiteralPath $offscreenPlugin)) { throw "Missing Qt offscreen platform plugin: $offscreenPlugin" }
        Copy-Item -LiteralPath $offscreenPlugin -Destination (Join-Path $stage 'platforms') -Force
    }
    $crt = Get-ChildItem -Path (Join-Path $env:VCToolsRedistDir 'x64/Microsoft.VC*.CRT') -Directory | Sort-Object FullName -Descending | Select-Object -First 1
    if ($null -eq $crt) { throw 'VC runtime unavailable; ask the user, do not install.' }
    Get-ChildItem -LiteralPath $crt.FullName -Filter '*.dll' | Copy-Item -Destination $stage
    $requiredRuntime = @('Qt5Core.dll','Qt5Gui.dll','Qt5Widgets.dll','platforms/qwindows.dll','platforms/qoffscreen.dll','vcruntime140.dll','msvcp140.dll','Run.cmd')
    if ($isBaselineSdk) { $requiredRuntime += 'imageformats/qsvg.dll' }
    if (-not $isBaselineSdk) { $requiredRuntime += 'Qt5Svg.dll' }
    foreach ($file in $requiredRuntime) {
        if (-not (Test-Path -LiteralPath (Join-Path $stage $file))) { throw "Missing runtime: $file" }
    }
    & "$PSScriptRoot/Verify.ps1" -Dist $stage -ReportDirectory (Join-Path $logs 'native')
    if ($LASTEXITCODE -ne 0) { throw 'Native staged validation failed' }
    if ((Get-FileHash -LiteralPath (Join-Path $SdkRoot 'manifest/component-sdk.json') -Algorithm SHA256).Hash -ne $sdkManifestHash) {
        throw 'SDK_CHANGED_DURING_BUILD: do not combine new mappings with old binary.'
    }
    $acceptanceChecks = if ($isBaselineSdk) { 'ctest,native' } else { 'ctest,token,native' }
    Invoke-Step 'acceptance' $Python @('-B',$lifecycle,'receipt','--project',$project,'--stage',$stage,'--sdk',$SdkRoot,'--source-fingerprint',$sourceFingerprint,'--checks',$acceptanceChecks,'--output',$success)
    Invoke-Step 'publish-tools' $Python @('-B',$lifecycle,'publish-tools','--project',$project,'--stage',$stage,'--name','gallery','--receipt',$success)
    $published = $true
    Write-Output "STABLE GALLERY: $(Join-Path $project 'build/tools/gallery')"
} catch {
    $failureRecord = @{status='FAIL';stage=$stage;error=$_.Exception.Message;sourceFingerprint=$sourceFingerprint}
    throw
} finally {
    try { Stop-Transcript } finally {
        $lane = if ($published) { 'latest-success' } else { 'latest-failure' }
        $logs = Complete-BuildLogRun -Logs $logs -Destination (Join-Path $project "output/logs/$lane/run")
        if ($null -ne $failureRecord) {
            $failureRecord.logs = $logs
            $failureRecord | ConvertTo-Json | Set-Content -LiteralPath $failure -Encoding utf8
        }
        Write-Output "BUILD LOGS: $logs"
    }
}
} finally { Exit-ProjectOperationLock -Handle $buildOperation }
