param(
    [ValidateSet('doctor','build','verify','package','clean')][string]$Action = 'doctor',
    [ValidateSet('all','component','layout','gallery')][string]$Target = 'all',
    [ValidateSet('developer','runtime')][string]$Mode = 'developer',
    [string]$Python = $env:D2C_PYTHON_EXE,
    [string]$QtRoot = $env:QT_ROOT, [string]$VsRoot = $env:VS_ROOT,
    [string]$Cmake = $env:CMAKE_EXE, [string]$NinjaRoot = $env:NINJA_ROOT,
    [string]$Node = $env:NODE_EXE, [string]$NodeModules = $env:D2C_NODE_MODULES,
    [string]$SdkRoot, [string]$ReleaseInputs, [string]$DeliveryBatch,
    [switch]$DryRun, [switch]$MarkOnly
)
$ErrorActionPreference = 'Stop'
$workspace = Split-Path $PSScriptRoot -Parent
$component = Join-Path $workspace 'DesignToCode_GenerateComponent'
$layout = Join-Path $workspace 'DesignToCode_GenerateLayout'
$gallery = Join-Path $workspace 'ComponentLab'
. "$gallery/Source/ProjectPython.ps1"
$Python = Resolve-ProjectPython $Python
$env:D2C_PYTHON_EXE = $Python
$env:D2C_SUPPORT_ROOT = $PSScriptRoot
$env:PYTHONDONTWRITEBYTECODE = '1'
$capability = & $Python -B "$PSScriptRoot/AgentCapabilityProfile.py" verify | ConvertFrom-Json
if ($LASTEXITCODE -ne 0 -or $capability.status -ne 'PASS') { throw 'AGENT_CAPABILITY_PROFILE_INVALID' }
$env:D2C_AGENT_CAPABILITY_ID = $capability.canonicalId
function Invoke-PythonStep([string[]]$Arguments) {
    & $Python -B @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Python step failed ($LASTEXITCODE): $Arguments" }
}
if ($Action -eq 'clean') {
    $projects = switch ($Target) {
        'component' { @($component) }; 'layout' { @($layout) }; 'gallery' { @($gallery) }
        default { @($component,$layout,$gallery) }
    }
    foreach ($project in $projects) {
        $arguments = @("$PSScriptRoot/ArtifactLifecycle.py",'clean','--project',$project)
        if ($DeliveryBatch) { $arguments += @('--delivery',$DeliveryBatch) }
        if ($DryRun) { $arguments += '--dry-run' }
        if ($MarkOnly) { $arguments += '--mark-only' }
        Invoke-PythonStep $arguments
    }
    return
}
# Resolve once. CMake and all child tools receive explicit dependencies.
if (-not $SdkRoot) { $SdkRoot = $env:GENERATED_COMPONENT_SDK_ROOT }
if (-not $SdkRoot) { $SdkRoot = Join-Path $component 'build/sdk' }
$SdkRoot = [IO.Path]::GetFullPath($SdkRoot)
$external = $SdkRoot -ne [IO.Path]::GetFullPath("$component/build/sdk")
$env:GENERATED_COMPONENT_SDK_ROOT = $SdkRoot
function Verify-Sdk {
    Invoke-PythonStep @("$PSScriptRoot/SDKContract.py",'verify','--sdk',$SdkRoot,'--configuration','Release')
}
if ($Action -eq 'package') {
    Verify-Sdk
    if (-not $ReleaseInputs) { throw 'RELEASE_INPUTS_REQUIRED' }
    Invoke-PythonStep @("$PSScriptRoot/PackageOffline.py",'--inputs',$ReleaseInputs,'--sdk',$SdkRoot)
    return
}
if ($Mode -eq 'runtime') {
    if ($Action -ne 'doctor') { throw 'Use the controlled Run.cmd for runtime generation or display.' }
    Verify-Sdk
    if ($Target -in @('all','layout')) {
        Invoke-PythonStep @("$PSScriptRoot/ArtifactLifecycle.py",'verify-tools','--root',"$layout/build/tools/layout")
    }
    if ($Target -in @('all','gallery')) {
        Invoke-PythonStep @("$PSScriptRoot/ArtifactLifecycle.py",'verify-tools','--root',"$gallery/build/tools/gallery")
    }
    Write-Output 'Controlled assets verified; no development compiler required.'
    return
}
$problems = [System.Collections.Generic.List[string]]::new()
if (-not $Cmake) { $Cmake = 'cmake' }
try { $Cmake = (Get-Command $Cmake -ErrorAction Stop).Source } catch { $problems.Add("Missing CMake: $Cmake") }
foreach ($pair in @(
    @('Qt5 MSVC x64',"$QtRoot/lib/cmake/Qt5/Qt5Config.cmake"),
    @('Qt uic',"$QtRoot/bin/uic.exe"), @('Qt deploy',"$QtRoot/bin/windeployqt.exe"),
    @('Visual Studio',"$VsRoot/Common7/Tools/Launch-VsDevShell.ps1"),
    @('Ninja',"$NinjaRoot/ninja.exe"))) {
    if (-not (Test-Path -LiteralPath $pair[1] -PathType Leaf)) { $problems.Add("Missing existing $($pair[0]): $($pair[1])") }
}
$needNode = $Target -in @('all','component') -or $Action -in @('verify','package')
if ($needNode) {
    if (-not $Node) { $Node = 'node' }
    try { $Node = (Get-Command $Node -ErrorAction Stop).Source } catch { $problems.Add("Missing Node: $Node") }
    foreach ($module in @('playwright','playwright-core')) {
        if (-not (Test-Path -LiteralPath "$NodeModules/$module/package.json")) { $problems.Add("Missing existing module: $module ($NodeModules)") }
    }
}
if ($problems.Count) {
    foreach ($problem in $problems) { Write-Output "MISSING: $problem" }
    throw 'ENVIRONMENT_INCOMPLETE: no environment change performed.'
}
$env:QT_ROOT = $QtRoot
$env:VS_ROOT = $VsRoot
$env:CMAKE_EXE = $Cmake
$env:NINJA_ROOT = $NinjaRoot
$env:NODE_EXE = $Node
$env:D2C_NODE_MODULES = $NodeModules
& $Python -B -c 'import sys; print((''Python:''), sys.version, sys.executable)'
if ($Action -eq 'doctor') {
    $state = if (Test-Path -LiteralPath "$SdkRoot/manifest/component-sdk.json") { 'published; verify before use' } else { 'not published; not source damage' }
    Write-Output "SDK: $SdkRoot [$state], external/read-only=$external"
    Write-Output "Execution capability baseline: $($capability.canonicalId) [$($capability.executionClass)]"
    Write-Output 'Existing developer prerequisites found. Build assets are protected; output is the execution area.'
    return
}
. "$component/Source/NativeTools.ps1"
Initialize-NativeTools
if ($Action -eq 'build') {
    $supportProjects = switch ($Target) {
        'component' { @($component) }; 'layout' { @($layout) }; 'gallery' { @($gallery) }
        default { @($component,$layout,$gallery) }
    }
    foreach ($project in $supportProjects) {
        Invoke-PythonStep @("$PSScriptRoot/RuntimeInputs.py",'support','--project',$project)
    }
    if ($Target -in @('all','layout') -and $ReleaseInputs) {
        Invoke-PythonStep @("$PSScriptRoot/RuntimeInputs.py",'runtime','--project',$layout,'--inputs',$ReleaseInputs)
    }
    if ($Target -in @('all','component')) {
        if ($external) {
            if ($Target -eq 'component') { throw 'EXTERNAL_SDK_READ_ONLY' }
        } else {
            & "$component/Source/Publish-ComponentSDK.ps1" -QtRoot $QtRoot -Cmake $Cmake `
                -BuildDirectory "$component/output/work/cache/GeneratedComponentSDK"
        }
    }
    Verify-Sdk
    if ($Target -in @('all','layout')) { & "$layout/Source/Build.ps1" -Test -SdkRoot $SdkRoot }
    if ($Target -in @('all','gallery')) {
        & "$gallery/Source/delivery/tools/Build.ps1" -QtDir $QtRoot -VsDir $VsRoot -CMake $Cmake `
            -Python $Python -NinjaDir $NinjaRoot -SdkRoot $SdkRoot `
            -BuildDirectory "$gallery/output/work/cache/gallery-release"
    }
    if ($Target -eq 'all') { Invoke-PythonStep @("$PSScriptRoot/BuildReceipt.py",'record','--sdk',$SdkRoot) }
    return
}
Verify-Sdk
if ($Action -eq 'verify') {
    if ($Target -in @('all','component')) {
        if (-not $external) {
            Invoke-PythonStep @("$component/output/work/generated/Source/ComponentKit/design-assets/DesignSystem/scripts/v2/tests/test_v2.py")
            Invoke-PythonStep @("$component/Source/CheckProject.py")
        } else { Invoke-PythonStep @("$component/Source/CheckProject.py",'--source-only') }
    }
    if ($Target -in @('all','layout')) {
        Invoke-PythonStep @("$layout/Source/CheckProject.py")
        foreach ($directory in @('','layout','formal')) {
            Invoke-PythonStep @('-m','unittest','discover','-s',"$layout/docs/validation/$directory",'-p','test_*.py')
        }
        & "$layout/Source/Build.ps1" -Test
    }
    if ($Target -in @('all','gallery')) {
        & "$gallery/Source/delivery/tools/Build.ps1" -QtDir $QtRoot -VsDir $VsRoot -CMake $Cmake `
            -Python $Python -NinjaDir $NinjaRoot -SdkRoot $SdkRoot `
            -BuildDirectory "$gallery/output/work/cache/gallery-release"
    }
    Invoke-PythonStep @('-m','unittest','discover','-s',"$workspace/docs/validation",'-p','test_*.py')
    if ($Target -eq 'all') { Invoke-PythonStep @("$PSScriptRoot/BuildReceipt.py",'record','--sdk',$SdkRoot,'--verified') }
    Write-Output 'Requested development regression passed; fresh-machine and visual acceptance remain separate.'
    return
}
if (-not $ReleaseInputs) { throw 'RELEASE_INPUTS_REQUIRED' }
Invoke-PythonStep @("$PSScriptRoot/PackageOffline.py",'--inputs',$ReleaseInputs,'--sdk',$SdkRoot)
