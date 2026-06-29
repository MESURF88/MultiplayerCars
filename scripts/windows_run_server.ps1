$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$goServerDir = Join-Path $repoRoot "GoServer"

. (Join-Path $PSScriptRoot "windows_local_tls.ps1")

if (-not (Get-Command go -ErrorAction SilentlyContinue)) {
    throw "go was not found. Run scripts/windows_setup_system.ps1 first."
}

Ensure-WindowsLocalTlsFiles -RepoRoot $repoRoot

Push-Location $goServerDir
try {
    go run .
} finally {
    Pop-Location
}
