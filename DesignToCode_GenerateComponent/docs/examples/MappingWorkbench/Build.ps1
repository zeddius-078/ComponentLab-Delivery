param(
    [string]$QtRoot = $env:QT_ROOT,
    [string]$BuildDirectory,
    [string]$Cmake = 'cmake',
    [string]$Generator = 'Ninja',
    [ValidateRange(1, 64)]
    [int]$Jobs = 4,
    [switch]$GenerateTokens,
    [switch]$Test,
    [switch]$Deploy
)
$ErrorActionPreference = 'Stop'
$env:PYTHONDONTWRITEBYTECODE = '1'
$env:QT_ROOT = $QtRoot
. "$PSScriptRoot/../../../Source/NativeTools.ps1"
Initialize-NativeTools
$python = Resolve-ProjectPython
if (-not $QtRoot -or -not (Test-Path -LiteralPath "$QtRoot/lib/cmake/Qt5")) {
    throw 'Pass -QtRoot or set QT_ROOT to your Qt 5.15 SDK.'
}
if (-not $BuildDirectory) {
    $BuildDirectory = Join-Path $D2CProjectRoot 'output/work/cache/Workbench'
}
$BuildDirectory = Assert-BuildOutput $BuildDirectory
$testing = if ($Test) { 'ON' } else { 'OFF' }
$generation = if ($GenerateTokens) { 'ON' } else { 'OFF' }
& $Cmake -S $PSScriptRoot -B $BuildDirectory -G $Generator -DCMAKE_BUILD_TYPE=Release `
    "-DCMAKE_PREFIX_PATH=$QtRoot" "-DPython3_EXECUTABLE=$python" "-DBUILD_TESTING=$testing" "-DD2C_BUILD_TOKEN_TOOLS=$generation" `
    "-DTEST_OUTPUT_DIR=$D2CProjectRoot/output/logs/Workbench" '-DBUTTON_MSVC_INCLUDE_PREFIX=注意: 包含文件:'
if ($LASTEXITCODE) { throw 'Configure failed.' }
# Token generation runs before the GUI build so RCC embeds the refreshed local outputs.
if ($GenerateTokens) {
    & $Cmake --build $BuildDirectory --config Release --target d2c_generate_tokens
    if ($LASTEXITCODE) { throw 'Private Token generation failed.' }
    & $Cmake --build $BuildDirectory --config Release --target d2c_check_tokens
    if ($LASTEXITCODE) { throw 'Private Token verification failed.' }
}
& $Cmake --build $BuildDirectory --config Release --parallel $Jobs
if ($LASTEXITCODE) { throw 'Build failed.' }
if ($Test) {
    $env:PATH = "$QtRoot/bin;" + $env:PATH
    $ctest = Join-Path (Split-Path (Get-Command $Cmake).Source) 'ctest.exe'
    & $ctest --test-dir $BuildDirectory -C Release -j $Jobs --output-on-failure --output-junit "$D2CProjectRoot/output/logs/Workbench/ctest.xml"
    if ($LASTEXITCODE) { throw 'Tests failed.' }
}
if ($Deploy) {
    # This standalone example exports a new delivery batch, never a stable tool.
    $batch = 'mapping-workbench-' + [guid]::NewGuid().ToString('N')
    $package = Assert-BuildOutput (Join-Path $D2CProjectRoot "output/work/staging/$batch")
    $delivery = Assert-DeliveryOutput (Join-Path $D2CProjectRoot "output/delivery/$batch")
    if ((Test-Path -LiteralPath $package) -or (Test-Path -LiteralPath $delivery)) { throw 'DELIVERY_ALREADY_EXISTS' }
    & $Cmake --install $BuildDirectory --config Release --prefix $package
    if ($LASTEXITCODE) { throw 'Install failed.' }
    & "$QtRoot/bin/windeployqt.exe" --release --no-translations --compiler-runtime "$package/design_to_code.exe"
    if ($LASTEXITCODE) { throw 'Qt deployment failed.' }
    # MSVC runtime DLLs come from the SDK, independently of any ComponentLab package.
    if ($env:VCToolsRedistDir) {
        $crt = Get-ChildItem "$env:VCToolsRedistDir/x64/Microsoft.VC*.CRT" -Directory | Select-Object -First 1
        if ($crt) { Get-ChildItem -LiteralPath $crt.FullName -Filter '*.dll' | Copy-Item -Destination $package }
    }
    foreach ($file in @('Qt5Widgets.dll', 'platforms/qwindows.dll', 'msvcp140.dll', 'vcruntime140.dll')) {
        if (-not (Test-Path -LiteralPath (Join-Path $package $file))) { throw "Missing deployment file: $file" }
    }
    # Recheck both parents before promotion; Directory.Move refuses overwrite.
    $package = Assert-BuildOutput $package
    $delivery = Assert-DeliveryOutput $delivery
    foreach ($entry in Get-ChildItem -LiteralPath $package -Recurse -Force) {
        if ($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'OUTPUT_PATH_IS_LINK' }
    }
    New-Item -ItemType Directory -Path (Split-Path $delivery) -Force | Out-Null
    [IO.Directory]::Move($package, $delivery)
    Write-Output "Demo export (not a controlled tool release): $delivery"
    Write-Output "Open: $delivery/Run.cmd"
}
