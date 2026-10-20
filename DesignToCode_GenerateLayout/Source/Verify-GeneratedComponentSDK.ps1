param([string]$Package, [ValidateSet('Release','Debug')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/NativeTools.ps1"
$source = if ($Package) { [IO.Path]::GetFullPath($Package) } else { $GeneratedComponentSDKRoot }
$python = Resolve-ProjectPython
$support = Resolve-SupportRoot
& $python -B "$support/SDKContract.py" verify --sdk $source --configuration $Configuration | Out-Host
if ($LASTEXITCODE -ne 0) { throw 'SDK_VERIFICATION_FAILED' }
return $source
