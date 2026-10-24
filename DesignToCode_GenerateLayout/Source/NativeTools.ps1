# Build-only SDK configuration. Design parsers and component rules remain route-local.
$D2CSourceRoot = $PSScriptRoot
$D2CProjectRoot = Split-Path $D2CSourceRoot -Parent
$D2CWorkspaceRoot = Split-Path $D2CProjectRoot -Parent
function Assert-BuildOutput {
    param([Parameter(Mandatory)][string]$Path)
    $resolved = [IO.Path]::GetFullPath($Path)
    $buildRoot = [IO.Path]::GetFullPath("$D2CProjectRoot/output/work")
    if (-not $resolved.StartsWith($buildRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'OUTPUT_MUST_BE_IN_WORK'
    }
    $cursor = $resolved
    while ($cursor -and $cursor.StartsWith($buildRoot, [StringComparison]::OrdinalIgnoreCase)) {
        if ((Test-Path -LiteralPath $cursor) -and ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw 'OUTPUT_PATH_IS_LINK'
        }
        $cursor = Split-Path $cursor -Parent
    }
    return $resolved
}
function Resolve-SupportRoot {
    # An explicit override is authoritative; do not silently replace a broken selection.
    $candidates = if ($env:D2C_SUPPORT_ROOT) {
        @($env:D2C_SUPPORT_ROOT)
    } else {
        @((Join-Path $D2CWorkspaceRoot 'Source'), (Join-Path $D2CProjectRoot 'build/tools/support'))
    }
    foreach ($support in $candidates) {
        if ((Test-Path -LiteralPath "$support/SDKContract.py" -PathType Leaf) -and
            (Test-Path -LiteralPath "$support/ArtifactLifecycle.py" -PathType Leaf)) {
            return [IO.Path]::GetFullPath($support)
        }
    }
    throw 'WORKSPACE_SUPPORT_MISSING: specify D2C_SUPPORT_ROOT with the published support tools.'
}
function Assert-LayoutToolMatchesCurrentSource {
    param(
        [Parameter(Mandatory)][string]$ToolRoot,
        [Parameter(Mandatory)][string]$Python
    )
    # Source entrypoints must never silently execute a controlled tool built from
    # different inputs.  This is deliberately not part of the portable Run.cmd
    # path: a standalone package has no editable workspace Source to compare.
    $manifestPath = Join-Path $ToolRoot 'tool-manifest.json'
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $published = [string]$manifest.acceptance.sourceFingerprint
    if (-not $published) {
        throw 'LAYOUT_TOOL_SOURCE_PROVENANCE_MISSING: publish layout from the current Source in the fixed development environment.'
    }
    $support = Resolve-SupportRoot
    $current = & $Python -B "$support/ArtifactLifecycle.py" fingerprint --project $D2CProjectRoot
    if ($LASTEXITCODE -ne 0 -or -not $current) { throw 'LAYOUT_SOURCE_FINGERPRINT_FAILED' }
    $current = $current.Trim()
    if ($published -ne $current) {
        throw "LAYOUT_TOOL_STALE_SOURCE: published=$published current=$current. Run Source/Workspace.ps1 -Action build -Target layout in the fixed development environment; do not edit DSL, switch Profile, or create a fallback generator."
    }
    if (-not $manifest.acceptance.executionCapabilityProfile) {
        throw 'LAYOUT_TOOL_AGENT_BASELINE_MISSING: republish layout from the current Source before using a low-capability Agent.'
    }
}
function Initialize-LayoutRuntime {
    param([string]$ToolRoot, [string]$WorkRoot, [switch]$NeedNode)
    if (-not $ToolRoot) { $ToolRoot = if ($env:D2C_RUNTIME_ROOT) { $env:D2C_RUNTIME_ROOT } else { "$D2CProjectRoot/build/tools/layout" } }
    if (-not $WorkRoot) { $WorkRoot = if ($env:D2C_WORK_ROOT) { $env:D2C_WORK_ROOT } else { $D2CProjectRoot } }
    $env:D2C_RUNTIME_ROOT = [IO.Path]::GetFullPath($ToolRoot)
    $env:D2C_WORK_ROOT = [IO.Path]::GetFullPath($WorkRoot)
    if (-not (Test-Path -LiteralPath "$ToolRoot/bin/v2_ui_compiler.exe") -or -not (Test-Path -LiteralPath "$ToolRoot/tool-manifest.json")) {
        throw 'LAYOUT_TOOL_NOT_PUBLISHED: run workspace build layout in the fixed development environment.'
    }
    $python = Resolve-ProjectPython
    & $python -B "$ToolRoot/scripts/ArtifactLifecycle.py" verify-tools --root $ToolRoot | Out-Host
    if ($LASTEXITCODE) { throw 'LAYOUT_TOOL_INTEGRITY_FAILED' }
    Assert-LayoutToolMatchesCurrentSource -ToolRoot $ToolRoot -Python $python
    $env:QT_QPA_PLATFORM_PLUGIN_PATH = "$ToolRoot/bin/platforms"
    $env:QT_PLUGIN_PATH = "$ToolRoot/bin"
    $env:QT_QPA_PLATFORM = 'offscreen'
    $env:PYTHONDONTWRITEBYTECODE = '1'
    if ($NeedNode) {
        if (-not $env:NODE_EXE -or -not $env:D2C_NODE_MODULES -or -not $env:D2C_BROWSER_EXE) {
            throw 'CONTROLLED_RUNTIME_REQUIRED: use the workspace runtime launcher; NODE_EXE, D2C_NODE_MODULES and D2C_BROWSER_EXE must select the fixed runtime.'
        }
        foreach ($required in @($env:NODE_EXE, "$env:D2C_NODE_MODULES/playwright", $env:D2C_BROWSER_EXE)) {
            if (-not (Test-Path -LiteralPath $required)) { throw "CONTROLLED_RUNTIME_MISSING: $required" }
        }
        $script:NativeNode = $env:NODE_EXE
        $env:NODE_PATH = $env:D2C_NODE_MODULES
    }
}
$GeneratedComponentSDKRoot = if ($env:GENERATED_COMPONENT_SDK_ROOT) {
    [IO.Path]::GetFullPath($env:GENERATED_COMPONENT_SDK_ROOT)
} else {
    [IO.Path]::GetFullPath((Join-Path $D2CWorkspaceRoot 'DesignToCode_GenerateComponent/build/sdk'))
}
function Resolve-ProjectPython {
    # Select an existing installation only; never fall back to a newer Python.
    $candidate = $env:D2C_PYTHON_EXE
    if (-not $candidate) {
        $launcher = Get-Command py -ErrorAction SilentlyContinue
        if (-not $launcher) { throw 'PYTHON39_REQUIRED: set D2C_PYTHON_EXE to an existing Python 3.9 executable; ask before environment changes.' }
        $candidate = & $launcher.Source -3.9 -B -c 'import sys; print(sys.executable)'
        if ($LASTEXITCODE -ne 0 -or -not $candidate) { throw 'PYTHON39_REQUIRED: existing Python 3.9 not found; ask the user.' }
        $candidate = $candidate.Trim()
    }
    if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) { throw 'PYTHON39_EXECUTABLE_MISSING' }
    $version = & $candidate -B -c 'import sys; print(sys.version[:3])'
    if ($LASTEXITCODE -ne 0 -or $version -ne '3.9') { throw 'PYTHON39_REQUIRED: refusing a different interpreter version.' }
    return (Resolve-Path -LiteralPath $candidate).Path
}
function Initialize-NativeTools {
    param([switch]$NeedNode)
    if (-not $env:QT_ROOT -or -not (Test-Path -LiteralPath "$env:QT_ROOT/lib/cmake/Qt5")) {
        throw 'Set QT_ROOT to the Qt5 MSVC x64 SDK.'
    }
    if ($env:VS_ROOT -and -not $env:VCToolsInstallDir) {
        & "$env:VS_ROOT/Common7/Tools/Launch-VsDevShell.ps1" -VsInstallationPath $env:VS_ROOT -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
    }
    $env:PATH = "$env:QT_ROOT/bin;$env:NINJA_ROOT;" + $env:PATH
    $env:PYTHONDONTWRITEBYTECODE = '1'
    $script:NativeCMake = (Get-Command $(if($env:CMAKE_EXE){$env:CMAKE_EXE}else{'cmake'}) -ErrorAction Stop).Source
    $script:NativeCTest = Join-Path (Split-Path $NativeCMake) 'ctest.exe'
    if ($NeedNode) {
        $script:NativeNode = (Get-Command $(if($env:NODE_EXE){$env:NODE_EXE}else{'node'}) -ErrorAction Stop).Source
        $nodeModules = if ($env:D2C_NODE_MODULES) {
            [IO.Path]::GetFullPath($env:D2C_NODE_MODULES)
        } else {
            throw 'CONTROLLED_NODE_MODULES_REQUIRED: set D2C_NODE_MODULES to the approved runtime; no package installation or fallback.'
        }
        if (-not (Test-Path -LiteralPath (Join-Path $nodeModules 'playwright'))) {
            throw "Playwright module missing. Set D2C_NODE_MODULES to its node_modules directory: $nodeModules"
        }
        $env:NODE_PATH = $nodeModules
    }
}
function Deploy-NativeQt {
    param([Parameter(Mandatory)][string]$Exe)
    $folder = Split-Path ([IO.Path]::GetFullPath($Exe)) -Parent
    & "$env:QT_ROOT/bin/windeployqt.exe" --release --no-translations --no-opengl-sw --no-system-d3d-compiler --no-compiler-runtime $Exe
    if ($LASTEXITCODE) { throw "Qt deployment failed: $Exe" }
    if (-not $env:VCToolsRedistDir) { throw 'MSVC redistributables unavailable; initialize the developer environment.' }
    $crt = Get-ChildItem "$env:VCToolsRedistDir/x64/Microsoft.VC*.CRT" -Directory | Select-Object -First 1
    if (-not $crt) { throw 'MSVC CRT directory missing.' }
    Get-ChildItem -LiteralPath $crt.FullName -Filter '*.dll' | Copy-Item -Destination $folder
    Copy-Item -LiteralPath "$env:QT_ROOT/plugins/platforms/qoffscreen.dll" -Destination "$folder/platforms/qoffscreen.dll"
    foreach ($required in @('Qt5Core.dll','Qt5Gui.dll','Qt5Widgets.dll','platforms/qwindows.dll','msvcp140.dll','vcruntime140.dll')) {
        if (-not (Test-Path -LiteralPath (Join-Path $folder $required))) { throw "Missing runtime: $required" }
    }
}
function Copy-GeneratedComponentRuntime {
    param([Parameter(Mandatory)][string]$Destination)
    $manifestPath = Join-Path $GeneratedComponentSDKRoot 'manifest/component-sdk.json'
    $runtimePath = Join-Path $GeneratedComponentSDKRoot 'lib/GeneratedComponentSDK.lib'
    if (-not (Test-Path -LiteralPath $manifestPath) -or -not (Test-Path -LiteralPath $runtimePath)) {
        throw 'Authoritative GeneratedComponentSDK is missing. Publish it from DesignToCode_GenerateComponent or set GENERATED_COMPONENT_SDK_ROOT.'
    }
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if ($manifest.owner -ne 'DesignToCode_GenerateComponent' -or -not $manifest.componentAuthority) {
        throw 'The shared package is not an authorized GenerateComponent publication.'
    }
    New-Item -ItemType Directory -Force $Destination | Out-Null
    # Component code is linked from the shared static SDK; the EXE needs no component DLL.
    $licenseTarget = Join-Path $Destination 'licenses'
    New-Item -ItemType Directory -Force $licenseTarget | Out-Null
    Copy-Item -Path (Join-Path $GeneratedComponentSDKRoot 'licenses/*.txt') -Destination $licenseTarget -Force
}
