$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$goServerDir = Join-Path $repoRoot "GoServer"

if (-not (Get-Command go -ErrorAction SilentlyContinue)) {
    throw "go was not found. Run scripts/windows_setup_system.ps1 first."
}

Push-Location $goServerDir
try {
    go run .
} finally {
    Pop-Location
}
