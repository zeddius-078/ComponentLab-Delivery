$ErrorActionPreference = 'Stop'
$workspace = [IO.Path]::GetFullPath("$PSScriptRoot/../../..")
$savedPython = $env:D2C_PYTHON_EXE
try {
    foreach ($project in @('DesignToCode_GenerateLayout', 'DesignToCode_GenerateComponent')) {
        . "$workspace/$project/Source/NativeTools.ps1"
        $env:D2C_PYTHON_EXE = $null
        $selected = Resolve-ProjectPython
        $version = & $selected -B -c 'import sys; print("%d.%d" % sys.version_info[:2])'
        if ($LASTEXITCODE -ne 0 -or $version -ne '3.9') { throw 'DEFAULT_IS_NOT_PYTHON39' }
        $env:D2C_PYTHON_EXE = $selected
        if ((Resolve-ProjectPython) -ne $selected) { throw 'EXPLICIT_INTERPRETER_NOT_PRESERVED' }
        $env:D2C_PYTHON_EXE = 'Z:\missing-python-for-test\python.exe'
        $rejected = $false
        try { Resolve-ProjectPython | Out-Null } catch { $rejected = $true }
        if (-not $rejected) { throw 'MISSING_INTERPRETER_ACCEPTED' }
        # Reject a newer interpreter only when one is already present; never install one.
        $candidate = (Get-Command python -ErrorAction SilentlyContinue).Source
        if ($candidate) {
            $version = & $candidate -B -c 'import sys; print("%d.%d" % sys.version_info[:2])'
            if ($version -ne '3.9') {
                $env:D2C_PYTHON_EXE = $candidate
                $rejected = $false
                try { Resolve-ProjectPython | Out-Null } catch { $rejected = $true }
                if (-not $rejected) { throw 'WRONG_VERSION_ACCEPTED' }
            }
        }
        Write-Output "$project : Python 3.9 selection and rejection checks PASS"
    }
} finally { $env:D2C_PYTHON_EXE = $savedPython }
