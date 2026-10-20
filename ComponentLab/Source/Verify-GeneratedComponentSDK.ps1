param([string]$Package, [ValidateSet('Release','Debug')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/ProjectTools.ps1"
$python = Resolve-ProjectPython
$contract = Resolve-SupportTool 'SDKContract.py'
$workspace = Split-Path $ComponentLabProject -Parent
$source = if ($Package) {
    [IO.Path]::GetFullPath($Package)
} elseif ($env:GENERATED_COMPONENT_SDK_ROOT) {
    [IO.Path]::GetFullPath($env:GENERATED_COMPONENT_SDK_ROOT)
} else {
    $folder = if ($Configuration -eq 'Debug') { 'build/sdk-debug' } else { 'build/sdk' }
    [IO.Path]::GetFullPath((Join-Path $workspace "DesignToCode_GenerateComponent/$folder"))
}
& $python -B $contract verify --sdk $source --configuration $Configuration | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'SDK_VERIFICATION_FAILED' }
Write-Output (Resolve-Path -LiteralPath $source).Path
