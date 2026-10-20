param(
    [string]$QtDir = $env:QT_ROOT,
    [string]$VsDir = $env:VS_ROOT,
    [string]$CMake = 'cmake',
    [string]$Python = 'python',
    [string]$NinjaDir = $env:NINJA_ROOT,
    [string]$BuildDirectory = '',
    [switch]$SkipTests,
    [switch]$SkipDeploy
)
$ErrorActionPreference = 'Stop'
$env:PYTHONDONTWRITEBYTECODE = '1'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$project = Split-Path $root -Parent
$build = if ($BuildDirectory) { [IO.Path]::GetFullPath($BuildDirectory) } else { Join-Path $project 'build/cache/main' }
$logs = Join-Path $project "build/reports/build-$stamp"
$CMake = (Get-Command $CMake -ErrorAction Stop).Source
$Python = (Get-Command $Python -ErrorAction Stop).Source
if (-not $QtDir) { throw 'Set QT_ROOT or provide -QtDir.' }
[void](New-Item -ItemType Directory -Path $logs -Force)
function Invoke-Step([string]$Name, [string]$Program, [string[]]$Arguments) {
    & $Program @Arguments 2>&1 | Tee-Object -FilePath (Join-Path $logs "$Name.log")
    if ($LASTEXITCODE -ne 0) { throw "$Name failed, exit $LASTEXITCODE. Evidence: $logs" }
}
Start-Transcript -LiteralPath (Join-Path $logs 'transcript.log')
try {
    if ($VsDir) { & (Join-Path $VsDir 'Common7\Tools\Launch-VsDevShell.ps1') -VsInstallationPath $VsDir -Arch amd64 -HostArch amd64 -SkipAutomaticLocation }
    $env:PATH = (Join-Path $QtDir 'bin') + ';' + $NinjaDir + ';' + $env:PATH
    $env:VSLANG = '1033'
    $prefix = -join ([char[]]@(0x6ce8,0x610f,0x003a,0x0020,0x5305,0x542b,0x6587,0x4ef6,0x003a))
    if (Test-Path -LiteralPath (Join-Path $env:VCToolsInstallDir 'bin\Hostx64\x64\1033\clui.dll')) { $prefix = 'Note: including file:' }
    Invoke-Step 'configure' $CMake @('-S',$root,'-B',$build,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release',"-DCMAKE_PREFIX_PATH=$QtDir","-DPython3_EXECUTABLE=$Python","-DBUTTON_MSVC_INCLUDE_PREFIX=$prefix","-DTEST_OUTPUT_DIR=$logs/tests",'-DCOMPONENTLAB_BUILD_DEMO=ON','-DBUILD_TESTING=ON')
    Invoke-Step 'compile' $CMake @('--build',$build,'--parallel','4')
    Invoke-Step 'governance' $Python @((Join-Path $root 'delivery/tools/CheckProject.py'),'--snapshot',(Join-Path $logs 'validation-inputs.json'))
    if (-not $SkipTests) { Invoke-Step 'tests' (Join-Path (Split-Path $CMake) 'ctest.exe') @('--test-dir',$build,'-j','1','--output-on-failure','--output-junit',(Join-Path $logs 'ctest.xml')) }
    @{status='passed';testsExecuted=(-not $SkipTests);exeSha256=(Get-FileHash -LiteralPath (Join-Path $build 'component_lab.exe') -Algorithm SHA256).Hash;validationInputs=(Get-Content -LiteralPath (Join-Path $logs 'validation-inputs.json') -Raw | ConvertFrom-Json)} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $logs 'build-verification.json') -Encoding utf8
    if ($SkipDeploy) { return }
    $dist = Join-Path $project 'build/demo/component_gallery'
    Invoke-Step 'install' $CMake @('--install',$build,'--prefix',$dist)
    Invoke-Step 'deploy' (Join-Path $QtDir 'bin/windeployqt.exe') @('--release','--no-translations','--compiler-runtime','--dir',$dist,(Join-Path $dist 'component_lab.exe'))
    $crt = Get-ChildItem -Path (Join-Path $env:VCToolsRedistDir 'x64/Microsoft.VC*.CRT') -Directory | Sort-Object FullName -Descending | Select-Object -First 1
    if ($null -eq $crt) { throw 'VC runtime unavailable' }
    Get-ChildItem -LiteralPath $crt.FullName -Filter '*.dll' | Copy-Item -Destination $dist
    $installer = Join-Path $dist 'vc_redist.x64.exe'
    if (Test-Path -LiteralPath $installer) { Remove-Item -LiteralPath $installer }
    foreach ($file in @('Qt5Core.dll','Qt5Gui.dll','Qt5Widgets.dll','Qt5Svg.dll','platforms/qwindows.dll','vcruntime140.dll','msvcp140.dll','Run.cmd')) {
        if (-not (Test-Path -LiteralPath (Join-Path $dist $file))) { throw "Missing runtime: $file" }
    }
    if (-not $SkipTests) {
        Invoke-Step 'delivery-manifest' $Python @('-B',(Join-Path $root 'delivery/tools/DeliveryManifest.py'),'--results',$logs)
    } else {
        Write-Output 'UNVERIFIED DEPLOY: tests and delivery-manifest refresh were skipped.'
    }
    Write-Output "OFFLINE: $dist"
} finally { Stop-Transcript }
