param(
    [string]$Url = "",
    [string]$RootId = "",
    [string]$FileKey = "",
    [string]$Batch = "",
    [string]$Page = "",
    [string]$PageId = "",
    [string]$Mapping = "",
    [string]$Font = "",
    [string]$FontFamily = "",
    [string]$LibraryProfile = "token-sdk",
    [string]$Endpoint = "http://127.0.0.1:3667/mcp",
    [ValidateSet('structure-only','strict')][string]$GenerationMode = 'structure-only',
    [switch]$FailOnError
)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/../NativeTools.ps1"
$python = Resolve-ProjectPython
$env:PYTHONDONTWRITEBYTECODE = '1'
$pythonArgs = @(
    '-B',
    "$PSScriptRoot/pipeline/RunPageAudit.py"
)
if ($Url) { $pythonArgs += @('--url', $Url) }
if ($RootId) { $pythonArgs += @('--root-id', $RootId) }
if ($FileKey) { $pythonArgs += @('--file-key', $FileKey) }
if ($Batch) { $pythonArgs += @('--batch', $Batch) }
if ($Page) { $pythonArgs += @('--page', $Page) }
if ($PageId) { $pythonArgs += @('--page-id', $PageId) }
if ($Mapping) { $pythonArgs += @('--mapping', $Mapping) }
if ($Font) { $pythonArgs += @('--font', $Font) }
if ($FontFamily) { $pythonArgs += @('--font-family', $FontFamily) }
if ($LibraryProfile) { $pythonArgs += @('--library-profile', $LibraryProfile) }
if ($Endpoint) { $pythonArgs += @('--endpoint', $Endpoint) }
if ($GenerationMode) { $pythonArgs += @('--generation-mode', $GenerationMode) }
if ($FailOnError) { $pythonArgs += '--fail-on-error' }
& $python @pythonArgs
exit $LASTEXITCODE
