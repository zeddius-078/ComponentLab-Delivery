param([switch]$Test, [switch]$Preview, [switch]$CompileOnly, [string]$BuildDirectory, [string]$SdkRoot)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/NativeTools.ps1"
$support = Resolve-SupportRoot
. "$support/OperationLock.ps1"
$buildOperation = Enter-ProjectOperationLock -ProjectRoot $D2CProjectRoot
try {
$sdkRoot = & "$PSScriptRoot/Verify-GeneratedComponentSDK.ps1" -Package $SdkRoot
if (-not $sdkRoot) { throw 'VERIFIED_SDK_REQUIRED' }
$sdkManifestHash = (Get-FileHash -LiteralPath "$sdkRoot/manifest/component-sdk.json" -Algorithm SHA256).Hash
$python = Resolve-ProjectPython
$sourceFingerprint = & $python -B "$support/ArtifactLifecycle.py" fingerprint --project $D2CProjectRoot
if ($LASTEXITCODE) { throw 'SOURCE_FINGERPRINT_FAILED' }
$runTests = $Test -or -not $CompileOnly
Initialize-NativeTools
if (-not $BuildDirectory) { $BuildDirectory = "$D2CProjectRoot/output/work/cache/V2-html-dsl" }
$BuildDirectory = Assert-BuildOutput $BuildDirectory
$cache = Join-Path $BuildDirectory 'CMakeCache.txt'
if (Test-Path -LiteralPath $cache) {
    $cachedSdk = Select-String -LiteralPath $cache -Pattern '^D2C_CONFIGURED_SDK_ROOT:INTERNAL=(.+)$'
    if ($cachedSdk -and [IO.Path]::GetFullPath($cachedSdk.Matches[0].Groups[1].Value) -ne [IO.Path]::GetFullPath($sdkRoot)) {
        throw 'SDK_CACHE_CONFLICT: use a new build directory when changing GENERATED_COMPONENT_SDK_ROOT.'
    }
}
& $NativeCMake -S $PSScriptRoot -B $BuildDirectory -G Ninja '-DCMAKE_BUILD_TYPE=Release' `
    "-DGENERATED_COMPONENT_SDK_ROOT=$sdkRoot" `
    "-DCMAKE_PREFIX_PATH=$env:QT_ROOT" "-DD2C_BUILD_TESTS=$(if($runTests){'ON'}else{'OFF'})" `
    "-DD2C_BUILD_PREVIEW=$(if($Preview){'ON'}else{'OFF'})"
if ($LASTEXITCODE -ne 0) { throw 'Configure failed' }
& $NativeCMake --build $BuildDirectory --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
if ($runTests) {
    & $NativeCTest --test-dir $BuildDirectory --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
}
if ($CompileOnly) { return }
# Only this call's successfully tested bytes may become a controlled tool.
$stage = Assert-BuildOutput "$D2CProjectRoot/output/work/stage/layout-$([guid]::NewGuid().ToString('N'))"
New-Item -ItemType Directory -Force "$stage/bin","$stage/scripts","$stage/resources" | Out-Null
Copy-Item -LiteralPath "$BuildDirectory/v2_ui_compiler.exe","$env:QT_ROOT/bin/uic.exe" -Destination "$stage/bin"
Deploy-NativeQt -Exe "$stage/bin/v2_ui_compiler.exe"
Copy-Item -LiteralPath "$PSScriptRoot/HtmlDsl/tools" -Destination "$stage/scripts/tools" -Recurse
Copy-Item -LiteralPath "$PSScriptRoot/HtmlDsl/pipeline/capture" -Destination "$stage/scripts/capture" -Recurse
Copy-Item -LiteralPath "$PSScriptRoot/HtmlDsl/PreviewStructure.py" -Destination "$stage/scripts/PreviewStructure.py"
foreach ($entry in @('RuntimeRun.py','ArtifactLifecycle.py','AgentCapabilityProfile.py','SDKContract.py')) {
    Copy-Item -LiteralPath "$support/$entry" -Destination "$stage/scripts/$entry"
}
Copy-Item -LiteralPath "$support/offline/LayoutRun.cmd" -Destination "$stage/Run.cmd"
Copy-Item -LiteralPath "$D2CProjectRoot/docs/fixtures/layout/mapping" -Destination "$stage/resources/mapping" -Recurse
Copy-Item -LiteralPath "$D2CProjectRoot/docs/examples/resources/HarmonyOS_Sans_SC_Regular.ttf" -Destination "$stage/resources/font.ttf"
Copy-Item -LiteralPath "$D2CProjectRoot/docs/examples/styles/initial-light.json" -Destination "$stage/resources/initial-light.json"
New-Item -ItemType Directory -Force "$stage/licenses" | Out-Null
Copy-Item -Path "$sdkRoot/licenses/*" -Destination "$stage/licenses" -Recurse
if ((Get-FileHash -LiteralPath "$sdkRoot/manifest/component-sdk.json" -Algorithm SHA256).Hash -ne $sdkManifestHash) {
    throw 'SDK_CHANGED_DURING_BUILD: do not combine new mappings with old binary.'
}
$receipt = "$D2CProjectRoot/output/logs/latest-success/layout-build.json"
& $python -B "$support/ArtifactLifecycle.py" receipt --project $D2CProjectRoot --stage $stage --sdk $sdkRoot --source-fingerprint $sourceFingerprint --checks ctest --output $receipt
if ($LASTEXITCODE) { throw 'LAYOUT_ACCEPTANCE_FAILED' }
& $python -B "$support/ArtifactLifecycle.py" publish-tools --project $D2CProjectRoot --stage $stage --name layout --receipt $receipt
if ($LASTEXITCODE) { throw 'LAYOUT_PUBLICATION_FAILED' }
} finally { Exit-ProjectOperationLock -Handle $buildOperation }
