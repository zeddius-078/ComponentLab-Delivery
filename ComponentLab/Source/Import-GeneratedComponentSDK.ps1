param([Parameter(Mandatory)][string]$Package)
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$target = [IO.Path]::GetFullPath((Join-Path $project 'build/sdk'))
$allowed = [IO.Path]::GetFullPath((Join-Path $project 'build'))
$source = [IO.Path]::GetFullPath($Package)
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
if (-not $target.StartsWith($allowed + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw "Unsafe dependency target: $target"
}
if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Recurse -Force }
New-Item -ItemType Directory -Path $target -Force | Out-Null
Get-ChildItem -LiteralPath $source -Force | Copy-Item -Destination $target -Recurse -Force
@{source=$source; target=$target; manifestSha256=(Get-FileHash "$target/manifest/component-sdk.json").Hash; importedAt=(Get-Date).ToString('o')} |
    ConvertTo-Json | Set-Content -LiteralPath "$project/build/reports/sdk-import.json" -Encoding utf8
Write-Output $target
