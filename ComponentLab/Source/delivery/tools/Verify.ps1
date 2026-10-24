param([string]$Dist, [string]$ReportDirectory, [string]$SdkRoot, [switch]$SkipCompilerChecks)
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
. "$root/ProjectTools.ps1"
$python = Resolve-ProjectPython
if (-not $Dist) { $Dist = Join-Path (Split-Path $root -Parent) 'build/tools/gallery' }
$Dist = (Resolve-Path -LiteralPath $Dist).Path
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$output = Assert-ProjectOutput $(if ($ReportDirectory) { $ReportDirectory } else {
    Join-Path (Split-Path $root -Parent) "output/logs/verify-$stamp"
}) -Lane logs
[void](New-Item -ItemType Directory -Path $output -Force)
$saved = @{}
foreach ($key in @('PATH','QT_PLUGIN_PATH','QT_QPA_PLATFORM_PLUGIN_PATH','QT_QPA_PLATFORM','QT_SCALE_FACTOR','GENERATED_COMPONENT_SDK_ROOT')) {
    $saved[$key] = [Environment]::GetEnvironmentVariable($key, 'Process')
}
$sdk = & "$root/Verify-GeneratedComponentSDK.ps1" -Package $SdkRoot
$env:GENERATED_COMPONENT_SDK_ROOT = $sdk
$sdkManifest = Get-Content -LiteralPath (Join-Path $sdk 'manifest/component-sdk.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$isBaselineSdk = $sdkManifest.packageKind -eq 'qt-native-baseline'
$results = @()
try {
    if (-not $SkipCompilerChecks) {
        $env:PATH = $Dist + ';' + $saved['PATH']
        & $python -B (Join-Path $root 'delivery/tools/CheckProject.py') --dist $Dist 2>&1 | Tee-Object -FilePath (Join-Path $output 'governance.log')
        if ($LASTEXITCODE -ne 0) { throw 'Project governance failed' }
    }
    if ($isBaselineSdk) {
        # The baseline has no Figma map or runtime Token JSON.  Validate the
        # portable binary as a direct QSS/QRC consumer instead of requiring
        # the retired gallery capture matrix.
        $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
        $env:QT_PLUGIN_PATH = 'D:\invalid-plugin-path'
        $env:QT_QPA_PLATFORM_PLUGIN_PATH = 'D:\invalid-platform-path'
        $env:QT_QPA_PLATFORM = 'offscreen'
        $capture = Join-Path $output 'baseline-native'
        [void](New-Item -ItemType Directory -Path $capture -Force)
        $exe = Join-Path $Dist 'component_lab.exe'
        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName = $exe
        $psi.Arguments = ('--capture-dir "' + $capture + '" --quit-after 2500')
        $psi.WorkingDirectory = $env:SystemRoot
        $psi.UseShellExecute = $false
        $psi.CreateNoWindow = $true
        $psi.RedirectStandardError = $true
        $psi.RedirectStandardOutput = $true
        $process = [System.Diagnostics.Process]::Start($psi)
        try {
            Start-Sleep -Milliseconds 900
            $process.Refresh()
            if ($process.HasExited) { throw 'Baseline gallery exited before module inspection' }
            $modules = @($process.Modules | Where-Object { $_.ModuleName -match '^(Qt5|qwindows|qoffscreen|vcruntime\d|msvcp\d|concrt\d)' } | ForEach-Object { @{name=$_.ModuleName;path=$_.FileName} })
            foreach ($module in $modules) {
                if (-not $module.path.StartsWith($Dist + '\',[StringComparison]::OrdinalIgnoreCase)) { throw "Dependency outside bundle: $($module.path)" }
            }
            foreach ($required in @('Qt5Core.dll','Qt5Gui.dll','Qt5Widgets.dll','qoffscreen.dll','vcruntime140.dll','msvcp140.dll')) {
                if ($required -notin $modules.name) { throw "Module not loaded: $required" }
            }
            if (-not $process.WaitForExit(30000)) { throw 'Baseline capture timed out' }
            [IO.File]::WriteAllText((Join-Path $capture 'stderr.log'), $process.StandardError.ReadToEnd())
            [IO.File]::WriteAllText((Join-Path $capture 'stdout.log'), $process.StandardOutput.ReadToEnd())
            if ($process.ExitCode -ne 0) { throw "Exit code: $($process.ExitCode)" }
        } finally {
            if (-not $process.HasExited) { $process.Kill(); [void]$process.WaitForExit(5000) }
        }
        $report = Get-Content -LiteralPath (Join-Path $capture 'runtime.json') -Raw -Encoding UTF8 | ConvertFrom-Json
        if ($report.packageKind -ne 'qt-native-baseline' -or
            $report.nativeComponentCount -ne 58 -or
            $report.compositeComponentCount -ne 1 -or
            $report.runtimeTokenJson -or $report.legacyRuntime -or -not $report.qssApplied -or
            $report.presentation -ne 'component-per-page-gallery' -or
            $report.gallerySections -ne 9 -or $report.galleryPages -ne 59 -or
            -not $report.objectInspector -or
            -not $report.indeterminateDashVisible) {
            throw 'Unexpected baseline runtime contract'
        }
        if ((Get-Item -LiteralPath (Join-Path $capture 'baseline-gallery.png')).Length -lt 5000) {
            throw 'Empty baseline screenshot'
        }
        $stderr = Get-Content -LiteralPath (Join-Path $capture 'stderr.log') -Raw
        if ($stderr -match 'Unknown property|Could not parse|Cannot load|QPainter::|Startup:|Capture:') {
            throw "Runtime rendering error: $stderr"
        }
        $results += @{packageKind='qt-native-baseline';modules=$modules;runtime=$report}
        @{status='PASS';timestamp=$stamp;dist=$Dist;packageKind='qt-native-baseline';exeSha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash;sdkManifestSha256=(Get-FileHash -LiteralPath (Join-Path $sdk 'manifest/component-sdk.json')).Hash;cleanPath=$true;invalidExternalPluginPath=$true;workingDirectory=$env:SystemRoot;freshMachineTest=$false;compilerChecks=(-not $SkipCompilerChecks);results=$results} | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath (Join-Path $output 'verification.json') -Encoding utf8
        Write-Output "PASS native offline simulation: qt-native-baseline"
        Write-Output "EVIDENCE: $output"
        return
    }
    $catalogPath = Join-Path $sdk 'resources/mappings/design-component-catalog.json'
    if (-not (Test-Path -LiteralPath $catalogPath)) {
        throw "Authoritative design component catalog missing: $catalogPath"
    }
    $designCatalog = Get-Content -LiteralPath $catalogPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $expectedComponents = [int]$designCatalog.componentCount
    $expectedSets = [int]$designCatalog.statistics.figmaComponentSetCount
    $expectedVariants = [int](($designCatalog.figmaPages | ForEach-Object {
        ($_.componentSets | ForEach-Object { $_.variants.Count } | Measure-Object -Sum).Sum
    } | Measure-Object -Sum).Sum)
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    $env:QT_PLUGIN_PATH = 'D:\invalid-plugin-path'
    $env:QT_QPA_PLATFORM_PLUGIN_PATH = 'D:\invalid-platform-path'
    # Environment adaptation (2026-10-22): the real-window (windows platform) capture
    # exceeds 18 minutes per combo on the fixed host (no interactive desktop and
    # software rendering), while offscreen completes in about 3.5 minutes with the
    # same screenshots, runtime.json counters and bundle-only plugin loading.
    $env:QT_QPA_PLATFORM = 'offscreen'
    foreach ($profile in @('standard-light','wide-light','standard-dark','wide-dark')) {
        foreach ($appearance in @('solid','gradient-elevated')) {
            $scale = if ($profile -eq 'wide-dark' -and $appearance -eq 'gradient-elevated') { '1.5' } else { '1' }
            $env:QT_SCALE_FACTOR = $scale
            $capture = Join-Path $output "$profile/$appearance"
            [void](New-Item -ItemType Directory -Path $capture -Force)
            $exe = Join-Path $Dist 'component_lab.exe'
            $psi = New-Object System.Diagnostics.ProcessStartInfo
            $psi.FileName = $exe
            $psi.Arguments = ('--profile ' + $profile + ' --appearance ' + $appearance + ' --capture-dir "' + $capture + '" --quit-after 7500')
            $psi.WorkingDirectory = $env:SystemRoot
            $psi.UseShellExecute = $false
            $psi.CreateNoWindow = $true
            $psi.RedirectStandardError = $true
            $psi.RedirectStandardOutput = $true
            $process = [System.Diagnostics.Process]::Start($psi)
            try {
                Start-Sleep -Milliseconds 2400
                $process.Refresh()
                if ($process.HasExited) { throw "Early exit: $profile" }
                $modules = @($process.Modules | Where-Object { $_.ModuleName -match '^(Qt5|qwindows|qoffscreen|vcruntime\d|msvcp\d|concrt\d)' } | ForEach-Object { @{name=$_.ModuleName;path=$_.FileName} })
                foreach ($module in $modules) {
                    if (-not $module.path.StartsWith($Dist + '\',[StringComparison]::OrdinalIgnoreCase)) { throw "Dependency outside bundle: $($module.path)" }
                }
                foreach ($required in @('Qt5Core.dll','Qt5Gui.dll','Qt5Widgets.dll','Qt5Svg.dll','qoffscreen.dll','vcruntime140.dll','msvcp140.dll')) {
                    if ($required -notin $modules.name) { throw "Module not loaded: $required" }
                }
                if (-not $process.WaitForExit(1800000)) { throw 'Capture timed out' }
                [IO.File]::WriteAllText((Join-Path $capture 'stderr.log'), $process.StandardError.ReadToEnd())
                [IO.File]::WriteAllText((Join-Path $capture 'stdout.log'), $process.StandardOutput.ReadToEnd())
                if ($process.ExitCode -ne 0) { throw "Exit code: $($process.ExitCode)" }
            } finally {
                if (-not $process.HasExited) { $process.Kill(); [void]$process.WaitForExit(5000) }
            }
            $report = Get-Content -LiteralPath (Join-Path $capture 'runtime.json') -Raw -Encoding UTF8 | ConvertFrom-Json
            $expectedPresets = @('solid','gradient','inset','gradient-inset','outline','elevated','gradient-elevated')
            if ($report.screens -ne 1 -or $report.capturedPages -ne 13 -or
                $report.designSystemComponents -ne $expectedComponents -or
                $report.figmaComponentSets -ne $expectedSets -or
                $report.figmaVariants -ne $expectedVariants -or
                $report.nativeDescriptors -ne $expectedComponents -or
                $report.nativeRegistryRows -ne $expectedComponents -or
                ($report.presets -join ',') -ne ($expectedPresets -join ',') -or
                $report.appearance -ne $appearance) { throw 'Unexpected Demo configuration' }
            $pages = @('172:2710','234:2717','339:2','242:2720','443:2','363:2')
            foreach ($i in 0..5) {
                $sections = -join ([char[]]@(0x5916,0x89C2,0x002C,0x5206,0x7EC4,0x4E0E,0x72B6,0x6001))
                if ($report.navigation[$i].pageId -ne $pages[$i] -or
                    ($report.navigation[$i].sections -join ',') -ne $sections) { throw 'Navigation contract failed' }
                $count = 2
                for ($j = 0; $j -lt $count; $j++) { if ((Get-Item -LiteralPath (Join-Path $capture "component-$i-$j.png")).Length -lt 5000) { throw 'Empty screenshot' } }
            }
            if ((Get-Item -LiteralPath (Join-Path $capture 'component-mapping.png')).Length -lt 5000) { throw 'Empty mapping screenshot' }
            $stderr = Get-Content -LiteralPath (Join-Path $capture 'stderr.log') -Raw
            if ($stderr -match 'Unknown property|Could not parse|Cannot load|QPainter::|Startup:|Capture:') { throw "Runtime rendering error: $stderr" }
            $results += @{profile=$profile;appearance=$appearance;scale=$scale;exitCode=$process.ExitCode;modules=$modules;runtime=$report}
            Write-Output "PASS native offline simulation: $profile / $appearance / scale=$scale"
        }
    }
    @{status='PASS';timestamp=$stamp;dist=$Dist;exeSha256=(Get-FileHash -LiteralPath (Join-Path $Dist 'component_lab.exe') -Algorithm SHA256).Hash;sdkManifestSha256=(Get-FileHash -LiteralPath (Join-Path $sdk 'manifest/component-sdk.json')).Hash;cleanPath=$true;invalidExternalPluginPath=$true;workingDirectory=$env:SystemRoot;freshMachineTest=$false;compilerChecks=(-not $SkipCompilerChecks);results=$results} | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath (Join-Path $output 'verification.json') -Encoding utf8
    Write-Output "EVIDENCE: $output"
} finally {
    foreach ($key in $saved.Keys) { [Environment]::SetEnvironmentVariable($key, $saved[$key], 'Process') }
}
