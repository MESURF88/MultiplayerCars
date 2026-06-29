param(
    [ValidateSet("DebugLocal", "Release")]
    [string]$Configuration = "DebugLocal"
)

$ErrorActionPreference = "Stop"

function Invoke-CheckedNative {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$FilePath failed with exit code $LASTEXITCODE"
    }
}

$preset = if ($Configuration -eq "Release") { "windows-release" } else { "windows-debug-local" }

Invoke-CheckedNative cmake --build --preset $preset

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$exePath = if ($Configuration -eq "Release") {
    Join-Path $repoRoot "CPPClient\build\CPPClient\Release\carclient.exe"
} else {
    Join-Path $repoRoot "CPPClient\builddbg\CPPClient\Debug\carclient.exe"
}

Write-Host ""
Write-Host "Client build complete:"
Write-Host "  $exePath"
