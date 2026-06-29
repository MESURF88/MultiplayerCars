param(
    [ValidateSet("DebugLocal", "Release")]
    [string]$Configuration = "DebugLocal",
    [switch]$SkipConfigure
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$toolsDir = Join-Path $repoRoot ".tools"
$vcpkgDir = Join-Path $toolsDir "vcpkg"
$cppClientDir = Join-Path $repoRoot "CPPClient"
$goServerDir = Join-Path $repoRoot "GoServer"

. (Join-Path $PSScriptRoot "windows_local_tls.ps1")

function Require-Command($Name, $Hint) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "$Name was not found. $Hint"
    }
}

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

function Ensure-Vcpkg {
    Require-Command git "Run scripts/windows_setup_system.ps1 first."
    Require-Command cmake "Run scripts/windows_setup_system.ps1 first."

    if (-not (Test-Path $toolsDir)) {
        New-Item -ItemType Directory -Path $toolsDir | Out-Null
    }

    if (-not (Test-Path $vcpkgDir)) {
        Write-Host "Cloning vcpkg into $vcpkgDir"
        Invoke-CheckedNative git clone https://github.com/microsoft/vcpkg.git $vcpkgDir
    }

    $vcpkgExe = Join-Path $vcpkgDir "vcpkg.exe"
    if (-not (Test-Path $vcpkgExe)) {
        Write-Host "Bootstrapping vcpkg"
        Invoke-CheckedNative (Join-Path $vcpkgDir "bootstrap-vcpkg.bat") -disableMetrics
    }

    return $vcpkgExe
}

function Ensure-LocalRuntimeFiles {
    $clientEnv = Join-Path $cppClientDir ".env"

    Ensure-WindowsLocalTlsFiles -RepoRoot $repoRoot

    if (-not (Test-Path $clientEnv)) {
        @'
{
  "username": "hill",
  "password": "1995"
}
'@ | Set-Content -Path $clientEnv -Encoding UTF8
    }
}

function Restore-GoServer {
    if (Get-Command go -ErrorAction SilentlyContinue) {
        Push-Location $goServerDir
        try {
            Invoke-CheckedNative go mod tidy
        } finally {
            Pop-Location
        }
    } else {
        Write-Warning "go was not found. The client can still configure, but run scripts/windows_setup_system.ps1 before starting the server."
    }
}

function Configure-Client($Preset) {
    Require-Command cmake "Run scripts/windows_setup_system.ps1 first."

    Write-Host "Configuring CMake preset $Preset"
    Invoke-CheckedNative cmake --preset $Preset
}

$preset = if ($Configuration -eq "Release") { "windows-release" } else { "windows-debug-local" }

$vcpkgExe = Ensure-Vcpkg
Write-Host "Restoring vcpkg manifest dependencies"
Invoke-CheckedNative $vcpkgExe install --triplet x64-windows --x-manifest-root=$cppClientDir

Ensure-LocalRuntimeFiles
Restore-GoServer

if (-not $SkipConfigure) {
    Configure-Client $preset
}

Write-Host ""
Write-Host "Windows project setup complete."
Write-Host "Build with:"
Write-Host "  powershell -ExecutionPolicy Bypass -File scripts/windows_build_client.ps1 -Configuration $Configuration"
