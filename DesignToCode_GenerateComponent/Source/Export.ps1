param([Parameter(Mandatory)][string]$OutputPath)
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$projectName = Split-Path $project -Leaf
$zip = [IO.Path]::GetFullPath($OutputPath)
if ([IO.Path]::GetExtension($zip) -ne '.zip' -or (Test-Path -LiteralPath $zip)) { throw 'Provide a new .zip output path.' }
if ($zip.StartsWith($project + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Export outside the project.' }
# Collect only this project's maintained files and deployable programs.
$files = @((Get-Item -LiteralPath "$project/README.md"))
foreach ($folder in @('Source','docs','build/sdk','build/demo')) {
    $files += Get-ChildItem -LiteralPath "$project/$folder" -Recurse -File -Force |
        Where-Object { $_.FullName -notmatch '[\\/]__pycache__[\\/]' -and $_.Extension -notin '.pyc','.pdb','.obj' }
}
[void](New-Item -ItemType Directory -Force (Split-Path $zip))
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::Open($zip, [IO.Compression.ZipArchiveMode]::Create)
$manifest = @()
try {
    foreach ($file in $files) {
        $relative = [IO.Path]::GetRelativePath($project, $file.FullName).Replace('\','/')
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive,$file.FullName,"$projectName/$relative",[IO.Compression.CompressionLevel]::Optimal) | Out-Null
        $manifest += @{path=$relative;bytes=$file.Length;sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash}
    }
    $entry = $archive.CreateEntry("$projectName/DELIVERY.json")
    $writer = [IO.StreamWriter]::new($entry.Open(),[Text.UTF8Encoding]::new($false))
    try { $writer.Write((@{scope='Source, docs, build/sdk, build/demo';validation='Inventory only; see independently executed checks';files=$manifest} | ConvertTo-Json -Depth 6)) }
    finally { $writer.Dispose() }
} finally { $archive.Dispose() }
Write-Output $zip
