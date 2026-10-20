param([string]$Batch, [string]$OutputPath, [string]$Receipt, [switch]$Unverified)
$ErrorActionPreference = 'Stop'
$source = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
. "$source/ProjectTools.ps1"
$python = Resolve-ProjectPython
if ($Unverified) { throw 'UNVERIFIED_RELEASE_NOT_SUPPORTED: keep incomplete work in output/work.' }
if (-not $OutputPath) {
    if (-not $Batch -or $Batch -notmatch '^[A-Za-z0-9][A-Za-z0-9_-]*$') { throw 'BATCH_REQUIRED' }
    $OutputPath = Join-Path $ComponentLabProject "output/delivery/$Batch/ComponentLab.zip"
}
$OutputPath = Assert-ProjectOutput $OutputPath -Lane delivery
$arguments = @('-B', (Join-Path $PSScriptRoot 'DeliveryManifest.py'), '--output', $OutputPath)
if ($Receipt) { $arguments += @('--receipt', [IO.Path]::GetFullPath($Receipt)) }
& $python @arguments
if ($LASTEXITCODE -ne 0) { throw 'GALLERY_PACKAGE_FAILED' }
