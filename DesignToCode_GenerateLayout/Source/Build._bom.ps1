param([switch]$Test, [switch]$Preview, [switch]$CompileOnly, [string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/NativeTools.ps1"
Initialize-NativeTools
if (-not $BuildDirectory) { $BuildDirectory = "$D2CProjectRoot/build/cache/V2-html-dsl" }
$BuildDirectory = Assert-BuildOutput $BuildDirectory
& $NativeCMake -S $PSScriptRoot -B $BuildDirectory -G Ninja '-DCMAKE_BUILD_TYPE=Release' `
    "-DCMAKE_PREFIX_PATH=$env:QT_ROOT" "-DD2C_BUILD_TESTS=$(if($Test){'ON'}else{'OFF'})" `
    "-DD2C_BUILD_PREVIEW=$(if($Preview){'ON'}else{'OFF'})"
if ($LASTEXITCODE -ne 0) { throw 'Configure failed' }
& $NativeCMake --build $BuildDirectory --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
if ($Test) {
    & $NativeCTest --test-dir $BuildDirectory --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
}
