# Build-only SDK configuration. Design parsers and component rules remain route-local.
$D2CSourceRoot = $PSScriptRoot
$D2CProjectRoot = Split-Path $D2CSourceRoot -Parent
$D2CWorkspaceRoot = Split-Path $D2CProjectRoot -Parent
function Assert-BuildOutput {
    param([Parameter(Mandatory)][string]$Path)
    $resolved = [IO.Path]::GetFullPath($Path)
    $buildRoot = [IO.Path]::GetFullPath("$D2CProjectRoot/build")
    if (-not $resolved.StartsWith($buildRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'OUTPUT_MUST_BE_IN_BUILD'
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
$GeneratedComponentSDKRoot = if ($env:GENERATED_COMPONENT_SDK_ROOT) {
    [IO.Path]::GetFullPath($env:GENERATED_COMPONENT_SDK_ROOT)
} else {
    [IO.Path]::GetFullPath((Join-Path $D2CWorkspaceRoot 'DesignToCode_GenerateComponent/build/sdk'))
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
            'D:\DevTools\ComponentLabNode\node_modules'
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
