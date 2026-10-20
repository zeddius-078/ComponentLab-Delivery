function Enter-ProjectOperationLock {
    param([Parameter(Mandatory=$true)][string]$ProjectRoot)
    $resolved = [IO.Path]::GetFullPath($ProjectRoot)
    $cursor = $resolved
    while ($cursor) {
        if (Test-Path -LiteralPath $cursor) {
            if ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "PATH_IS_LINK: $cursor"
            }
        }
        $cursor = Split-Path $cursor -Parent
    }
    if ($env:D2C_OPERATION_LOCK_ROOT) { throw 'NESTED_POWERSHELL_OPERATION_NOT_SUPPORTED' }
    $state = Join-Path $resolved 'build/.state'
    foreach ($part in @((Join-Path $resolved 'build'), $state, (Join-Path $state 'operation.lock'))) {
        if ((Test-Path -LiteralPath $part) -and ((Get-Item -LiteralPath $part -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw "PATH_IS_LINK: $part"
        }
    }
    [void](New-Item -ItemType Directory -Path $state -Force)
    try { $handle = [IO.File]::Open((Join-Path $state 'operation.lock'), 'OpenOrCreate', 'ReadWrite', 'None') }
    catch { throw "PROJECT_OPERATION_LOCKED: $resolved" }
    $env:D2C_OPERATION_LOCK_ROOT = $resolved
    $env:D2C_OPERATION_LOCK_OWNER_PID = "$PID"
    return $handle
}

function Exit-ProjectOperationLock {
    param([Parameter(Mandatory=$true)]$Handle)
    $Handle.Dispose()
    Remove-Item Env:D2C_OPERATION_LOCK_ROOT -ErrorAction SilentlyContinue
    Remove-Item Env:D2C_OPERATION_LOCK_OWNER_PID -ErrorAction SilentlyContinue
}
