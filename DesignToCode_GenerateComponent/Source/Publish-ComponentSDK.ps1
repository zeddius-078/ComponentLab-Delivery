<#
Compatibility entry point for the only publishable component SDK.

The historical full runtime remains source evidence only.  It is deliberately
not reachable from this command: `build/sdk` is the scope-locked Qt-native
baseline package, not a carrier for Token JSON, mappings or composites.
#>
param(
    [string]$QtRoot = $env:QT_ROOT,
    [string]$BuildDirectory,
    [string]$Cmake = $(if ($env:CMAKE_EXE) { $env:CMAKE_EXE } else { 'cmake' }),
    [string]$Generator = 'Ninja'
)

& "$PSScriptRoot/Publish-BaselineNativeSDK.ps1" @PSBoundParameters
exit $LASTEXITCODE
