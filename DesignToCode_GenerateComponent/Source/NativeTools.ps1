# Build-only SDK configuration. Design parsers and component rules remain route-local.
$D2CSourceRoot = $PSScriptRoot
$D2CProjectRoot = Split-Path $D2CSourceRoot -Parent
function Assert-BuildOutput {
    param([Parameter(Mandatory)][string]$Path)
    $resolved = [IO.Path]::GetFullPath($Path)
    $buildRoot = [IO.Path]::GetFullPath("$D2CProjectRoot/output/work")
    if (-not $resolved.StartsWith($buildRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'OUTPUT_MUST_BE_IN_WORK' }
    $cursor = $resolved
    while ($cursor) {
        if ((Test-Path -LiteralPath $cursor) -and ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'OUTPUT_PATH_IS_LINK' }
        $cursor = Split-Path $cursor -Parent
    }
    return $resolved
}
function Assert-ControlledBuildOutput {
    param([Parameter(Mandatory)][string]$Path)
    $resolved = [IO.Path]::GetFullPath($Path)
    $root = [IO.Path]::GetFullPath("$D2CProjectRoot/build")
    if (-not $resolved.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'CONTROLLED_OUTPUT_MUST_BE_IN_BUILD' }
    $cursor = $resolved
    while ($cursor) {
        if ((Test-Path -LiteralPath $cursor) -and ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'OUTPUT_PATH_IS_LINK' }
        $cursor = Split-Path $cursor -Parent
    }
    return $resolved
}
function Assert-DeliveryOutput {
    param([Parameter(Mandatory)][string]$Path)
    $resolved = [IO.Path]::GetFullPath($Path)
    $root = [IO.Path]::GetFullPath("$D2CProjectRoot/output/delivery")
    if (-not $resolved.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'DELIVERY_MUST_BE_IN_OUTPUT_DELIVERY' }
    $cursor = $resolved
    while ($cursor) {
        if ((Test-Path -LiteralPath $cursor) -and ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'OUTPUT_PATH_IS_LINK' }
        $cursor = Split-Path $cursor -Parent
    }
    return $resolved
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
    $version = & $candidate -B -c 'import sys; print(''%d.%d'' % sys.version_info[:2])'
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
