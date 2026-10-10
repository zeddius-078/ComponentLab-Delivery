param(
    [Parameter(Mandatory=$true)][string]$OutputPath,
    [switch]$Unverified
)
$ErrorActionPreference = 'Stop'
$source = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$root = Split-Path $source -Parent
$dist = Join-Path $root 'build/demo/component_gallery'
$documents = Join-Path $root 'docs'
$evidence = Join-Path $root 'build/reports'
$zip = [IO.Path]::GetFullPath($OutputPath)
if ([IO.Path]::GetExtension($zip) -ne '.zip') { throw 'OutputPath must end in .zip' }
if ($zip.StartsWith($root + '\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Export outside the delivery folder to avoid retaining a duplicate archive here' }
if (Test-Path -LiteralPath $zip) { throw "Output already exists: $zip" }
$exeHash = (Get-FileHash -LiteralPath (Join-Path $dist 'component_lab.exe') -Algorithm SHA256).Hash
if (-not $Unverified) {
    $currentInputsJson = & python (Join-Path $source 'delivery/tools/CheckProject.py') --print-inputs
    if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect current validation inputs' }
    $currentInputs = $currentInputsJson | ConvertFrom-Json -AsHashtable
    function Match-Inputs($Inputs) {
        if ($null -eq $Inputs -or $Inputs.Count -ne $currentInputs.Count) { return $false }
        foreach ($key in $currentInputs.Keys) { if ($Inputs[$key] -ne $currentInputs[$key]) { return $false } }
        return $true
    }
    $reports = @(Get-ChildItem -LiteralPath $evidence -Filter '*.json' -File -Recurse | ForEach-Object {
        Get-Content -LiteralPath $_.FullName -Raw | ConvertFrom-Json -AsHashtable
    })
    $builds = @($reports | Where-Object { $_.status -eq 'passed' -and $_.testsExecuted -and $_.exeSha256 -eq $exeHash -and (Match-Inputs $_.validationInputs) })
    $proofs = @($reports | Where-Object { $_.status -eq 'passed' -and $_.baselineUnchanged -and $null -ne $_.executables -and $_.executables['component_lab.exe'] -eq $exeHash -and (Match-Inputs $_.validationInputsBefore) -and (Match-Inputs $_.validationInputsAfter) })
    $native = @($reports | Where-Object { $_.exeSha256 -eq $exeHash -and $_.results.Count -eq 8 })
    if (-not $builds.Count -or -not $proofs.Count -or -not $native.Count) { throw 'Complete current build tests, Token proof and native verification, or explicitly use -Unverified' }
}
$items = [System.Collections.Generic.List[object]]::new()
foreach ($folder in @('build/demo/component_gallery','build/sdk','Source','docs')) {
    foreach ($file in Get-ChildItem -LiteralPath (Join-Path $root $folder) -Recurse -File -Force) {
        $relative = [IO.Path]::GetRelativePath($root,$file.FullName).Replace('\','/')
        if ($relative -match '^Source/(delivery/\.work|build|dist|release|delivery/tools/workspace)/' -or
            $relative -match '^Build/(DesignToCode|ComponentLab|qt-page-governance)/' -or
            $relative -match '(^|/)__pycache__/') { continue }
        $items.Add(@{path=$relative;file=$file.FullName;bytes=$file.Length;sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash})
    }
}
$readme = Join-Path $root 'README.md'
$items.Add(@{path='README.md';file=$readme;bytes=(Get-Item -LiteralPath $readme).Length;sha256=(Get-FileHash -LiteralPath $readme -Algorithm SHA256).Hash})
$manifest = @{id='ComponentLab-R4-sToken';validationStatus=$(if($Unverified){'limited-checks-no-full-regression'}else{'passed'});exeSha256=$exeHash;files=@($items | ForEach-Object { @{path=$_.path;bytes=$_.bytes;sha256=$_.sha256} })}
[void](New-Item -ItemType Directory -Path (Split-Path $zip -Parent) -Force)
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::Open($zip,[IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($item in $items) {
        [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive,$item.file,('ComponentLab/'+$item.path),[IO.Compression.CompressionLevel]::Optimal)
    }
    [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive,(Join-Path $source 'delivery/tools/OpenDelivery.cmd'),'ComponentLab/Run.cmd',[IO.Compression.CompressionLevel]::Optimal)
    $entry = $archive.CreateEntry('ComponentLab/DELIVERY.json')
    $writer = [IO.StreamWriter]::new($entry.Open(),[Text.UTF8Encoding]::new($false))
    try { $writer.Write(($manifest | ConvertTo-Json -Depth 8)) } finally { $writer.Dispose() }
} finally { $archive.Dispose() }
[pscustomobject]@{path=$zip;bytes=(Get-Item -LiteralPath $zip).Length;sha256=(Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash} | Format-List
