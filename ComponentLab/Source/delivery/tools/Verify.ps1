param([string]$Dist, [switch]$SkipCompilerChecks)
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
if (-not $Dist) { $Dist = Join-Path (Split-Path $root -Parent) 'build/demo/component_gallery' }
$Dist = (Resolve-Path -LiteralPath $Dist).Path
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$output = Join-Path (Split-Path $root -Parent) "build/reports/verify-$stamp"
[void](New-Item -ItemType Directory -Path $output -Force)
$saved = @{}
foreach ($key in @('PATH','QT_PLUGIN_PATH','QT_QPA_PLATFORM_PLUGIN_PATH','QT_QPA_PLATFORM','QT_SCALE_FACTOR')) {
    $saved[$key] = [Environment]::GetEnvironmentVariable($key, 'Process')
}
$results = @()
try {
    if (-not $SkipCompilerChecks) {
        $env:PATH = $Dist + ';' + $saved['PATH']
        & python (Join-Path $root 'delivery/tools/CheckProject.py') 2>&1 | Tee-Object -FilePath (Join-Path $output 'governance.log')
        if ($LASTEXITCODE -ne 0) { throw 'Project governance failed' }
        $sdk = Join-Path (Split-Path $root -Parent) 'build/sdk'
        $manifest = Get-Content -LiteralPath (Join-Path $sdk 'manifest/component-sdk.json') -Raw | ConvertFrom-Json
        foreach ($file in $manifest.files) {
            $path = Join-Path $sdk $file.path
            if (-not (Test-Path -LiteralPath $path) -or (Get-FileHash -LiteralPath $path).Hash -ne $file.sha256) {
                throw "Imported SDK drift: $($file.path)"
            }
        }
    }
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    $env:QT_PLUGIN_PATH = 'D:\invalid-plugin-path'
    $env:QT_QPA_PLATFORM_PLUGIN_PATH = 'D:\invalid-platform-path'
    $env:QT_QPA_PLATFORM = 'windows'
    foreach ($profile in @('light-comfortable','light-compact','dark-comfortable','dark-compact')) {
        foreach ($appearance in @('solid','gradient-elevated')) {
            $scale = if ($profile -eq 'dark-compact' -and $appearance -eq 'gradient-elevated') { '1.5' } else { '1' }
            $env:QT_SCALE_FACTOR = $scale
            $capture = Join-Path $output "$profile/$appearance"
            [void](New-Item -ItemType Directory -Path $capture -Force)
            $exe = Join-Path $Dist 'component_lab.exe'
            $process = Start-Process -FilePath $exe -ArgumentList @('--profile',$profile,'--appearance',$appearance,'--capture-dir',('"' + $capture + '"'),'--quit-after','7500') -WorkingDirectory $env:SystemRoot -WindowStyle Hidden -PassThru -RedirectStandardError (Join-Path $capture 'stderr.log') -RedirectStandardOutput (Join-Path $capture 'stdout.log')
            try {
                Start-Sleep -Milliseconds 2400
                $process.Refresh()
                if ($process.HasExited) { throw "Early exit: $profile" }
                $modules = @($process.Modules | Where-Object { $_.ModuleName -match '^(Qt5|qwindows|vcruntime\d|msvcp\d|concrt\d)' } | ForEach-Object { @{name=$_.ModuleName;path=$_.FileName} })
                foreach ($module in $modules) {
                    if (-not $module.path.StartsWith($Dist + '\',[StringComparison]::OrdinalIgnoreCase)) { throw "Dependency outside bundle: $($module.path)" }
                }
                foreach ($required in @('Qt5Core.dll','Qt5Gui.dll','Qt5Widgets.dll','Qt5Svg.dll','qwindows.dll','vcruntime140.dll','msvcp140.dll')) {
                    if ($required -notin $modules.name) { throw "Module not loaded: $required" }
                }
                if (-not $process.WaitForExit(45000)) { throw 'Capture timed out' }
                if ($process.ExitCode -ne 0) { throw "Exit code: $($process.ExitCode)" }
            } finally {
                if (-not $process.HasExited) { $process.Kill(); [void]$process.WaitForExit(5000) }
            }
            $report = Get-Content -LiteralPath (Join-Path $capture 'runtime.json') -Raw | ConvertFrom-Json
            $expectedPresets = @('solid','gradient','inset','gradient-inset','outline','elevated','gradient-elevated')
            if ($report.screens -ne 1 -or $report.capturedPages -ne 13 -or $report.designSystemComponents -ne 62 -or $report.nativeDescriptors -ne 59 -or $report.nativeRegistryRows -ne 62 -or ($report.presets -join ',') -ne ($expectedPresets -join ',') -or $report.appearance -ne $appearance) { throw 'Unexpected Demo configuration' }
            $components = @('commonWidget::Z_PushButton','commonWidget::Z_CheckBox','commonWidget::Z_InputField','commonWidget::Z_Select','commonWidget::Z_ToolButton','commonWidget::Z_TableView')
            foreach ($i in 0..5) {
                $sections = '外观,分组与状态'
                if ($report.navigation[$i].component -ne $components[$i] -or
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
    @{id='COMPONENT-LAB-CPP-QT-2026-09-15-R4-sToken';timestamp=$stamp;dist=$Dist;exeSha256=(Get-FileHash -LiteralPath (Join-Path $Dist 'component_lab.exe') -Algorithm SHA256).Hash;cleanPath=$true;invalidExternalPluginPath=$true;workingDirectory=$env:SystemRoot;freshMachineTest=$false;compilerChecks=(-not $SkipCompilerChecks);results=$results} | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath (Join-Path $output 'verification.json') -Encoding utf8
    Write-Output "EVIDENCE: $output"
} finally {
    foreach ($key in $saved.Keys) { [Environment]::SetEnvironmentVariable($key, $saved[$key], 'Process') }
}
