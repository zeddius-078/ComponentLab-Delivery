param([string]$Package)
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$workspace = Split-Path $project -Parent
$source = if ($env:GENERATED_COMPONENT_SDK_ROOT) {
    [IO.Path]::GetFullPath($env:GENERATED_COMPONENT_SDK_ROOT)
} else {
    [IO.Path]::GetFullPath((Join-Path $workspace 'DesignToCode_GenerateComponent/build/sdk'))
}
if ($Package -and [IO.Path]::GetFullPath($Package) -ne $source) {
    throw "ComponentLab must consume the authoritative SDK directly: $source"
}
$manifestPath = Join-Path $source 'manifest/component-sdk.json'
if (-not (Test-Path -LiteralPath $manifestPath)) { throw "Published SDK manifest missing: $source" }
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.owner -ne 'DesignToCode_GenerateComponent' -or -not $manifest.componentAuthority) {
    throw 'Package is not an authorized GenerateComponent publication.'
}
foreach ($file in $manifest.files) {
    $path = Join-Path $source $file.path
    if (-not (Test-Path -LiteralPath $path) -or (Get-FileHash -LiteralPath $path).Hash -ne $file.sha256) {
        throw "SDK source hash mismatch: $($file.path)"
    }
}
Write-Output $source
