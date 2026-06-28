param(
    [ValidateSet("DebugLocal", "Release")]
    [string]$Configuration = "DebugLocal"
)

$ErrorActionPreference = "Stop"

$preset = if ($Configuration -eq "Release") { "windows-release" } else { "windows-debug-local" }

cmake --build --preset $preset

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$exePath = if ($Configuration -eq "Release") {
    Join-Path $repoRoot "CPPClient\build\carclient.exe"
} else {
    Join-Path $repoRoot "CPPClient\builddbg\carclient.exe"
}

Write-Host ""
Write-Host "Client build complete:"
Write-Host "  $exePath"
