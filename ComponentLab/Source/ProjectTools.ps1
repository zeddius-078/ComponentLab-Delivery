# Shared local entry helpers. Tool implementations remain in the support bundle.
. "$PSScriptRoot/ProjectPython.ps1"
$ComponentLabProject = Split-Path $PSScriptRoot -Parent
function Resolve-SupportTool {
    param([Parameter(Mandatory)][string]$Name)
    if ($env:D2C_SUPPORT_ROOT) {
        $candidate = Join-Path ([IO.Path]::GetFullPath($env:D2C_SUPPORT_ROOT)) $Name
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) { throw "SUPPORT_TOOL_MISSING: $candidate" }
        return $candidate
    }
    foreach ($folder in @((Join-Path (Split-Path $ComponentLabProject -Parent) 'Source'),
                           (Join-Path $ComponentLabProject 'build/tools/support'))) {
        $candidate = Join-Path $folder $Name
        if (Test-Path -LiteralPath $candidate -PathType Leaf) { return $candidate }
    }
    throw "SUPPORT_TOOL_MISSING: $Name; provide D2C_SUPPORT_ROOT or a complete managed workspace."
}
function Assert-ProjectOutput {
    param([Parameter(Mandatory)][string]$Path,
          [ValidateSet('work','logs','delivery')][string]$Lane = 'work')
    $resolved = [IO.Path]::GetFullPath($Path)
    $allowed = [IO.Path]::GetFullPath((Join-Path $ComponentLabProject "output/$Lane"))
    if (-not $resolved.StartsWith($allowed + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "OUTPUT_OUTSIDE_$($Lane.ToUpperInvariant())"
    }
    $cursor = $resolved
    while ($cursor) {
        if ((Test-Path -LiteralPath $cursor) -and
            ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "OUTPUT_PATH_IS_LINK: $cursor"
        }
        $cursor = Split-Path $cursor -Parent
    }
    return $resolved
}
