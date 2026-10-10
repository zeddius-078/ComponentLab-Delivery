# Build-only SDK configuration. Design parsers and component rules remain route-local.
$D2CSourceRoot = $PSScriptRoot
$D2CProjectRoot = Split-Path $D2CSourceRoot -Parent
function Initialize-NativeTools {
    param([switch]$NeedNode)
    if (-not $env:QT_ROOT -or -not (Test-Path -LiteralPath "$env:QT_ROOT/lib/cmake/Qt5")) {
        throw 'Set QT_ROOT to the Qt5 MSVC x64 SDK.'
    }
    if ($env:VS_ROOT -and -not $env:VCToolsInstallDir) {
        & "$env:VS_ROOT/Common7/Tools/Launch-VsDevShell.ps1" -VsInstallationPath $env:VS_ROOT -Arch amd64 -HostArch amd64 -SkipAutomaticLocation -NoLogo
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
