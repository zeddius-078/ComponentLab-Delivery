function Resolve-ProjectPython {
    param([string]$Executable = $env:D2C_PYTHON_EXE)
    if (-not $Executable) {
        $Executable = & py -3.9 -B -c 'import sys; print(sys.executable)'
        if ($LASTEXITCODE -ne 0) { throw 'Python 3.9 is required. No environment will be installed.' }
    }
    $resolved = (Get-Command $Executable -ErrorAction Stop).Source
    & $resolved -B -c 'import sys; sys.exit(0 if sys.version_info[:2] == (3,9) else 1)'
    if ($LASTEXITCODE -ne 0) { throw "Python 3.9 required: $resolved" }
    $env:PYTHONDONTWRITEBYTECODE = '1'
    return $resolved
}
