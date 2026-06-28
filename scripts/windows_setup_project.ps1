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

function Require-Command($Name, $Hint) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "$Name was not found. $Hint"
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
        git clone https://github.com/microsoft/vcpkg.git $vcpkgDir
    }

    $vcpkgExe = Join-Path $vcpkgDir "vcpkg.exe"
    if (-not (Test-Path $vcpkgExe)) {
        Write-Host "Bootstrapping vcpkg"
        & (Join-Path $vcpkgDir "bootstrap-vcpkg.bat") -disableMetrics
    }

    return $vcpkgExe
}

function Ensure-LocalRuntimeFiles {
    $keysDir = Join-Path $goServerDir "keys"
    $serverKey = Join-Path $keysDir "server.key"
    $serverCert = Join-Path $keysDir "server.crt"
    $clientCert = Join-Path $cppClientDir "server.crt"
    $clientEnv = Join-Path $cppClientDir ".env"

    if (-not (Test-Path $keysDir)) {
        New-Item -ItemType Directory -Path $keysDir | Out-Null
    }

    if ((-not (Test-Path $serverKey)) -or (-not (Test-Path $serverCert))) {
        if (Get-Command openssl -ErrorAction SilentlyContinue) {
            Write-Host "Generating local TLS certificate"
            openssl req -x509 -newkey rsa:2048 -nodes `
                -keyout $serverKey `
                -out $serverCert `
                -days 365 `
                -subj "/CN=localhost" `
                -addext "subjectAltName=DNS:localhost,IP:127.0.0.1"
        } else {
            Write-Warning "openssl was not found. Create GoServer/keys/server.crt and GoServer/keys/server.key before running the local server."
        }
    }

    if (Test-Path $serverCert) {
        Copy-Item -Path $serverCert -Destination $clientCert -Force
    }

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
            go mod tidy
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
    cmake --preset $Preset
}

$preset = if ($Configuration -eq "Release") { "windows-release" } else { "windows-debug-local" }

$vcpkgExe = Ensure-Vcpkg
Write-Host "Restoring vcpkg manifest dependencies"
& $vcpkgExe install --triplet x64-windows --x-manifest-root=$cppClientDir

Ensure-LocalRuntimeFiles
Restore-GoServer

if (-not $SkipConfigure) {
    Configure-Client $preset
}

Write-Host ""
Write-Host "Windows project setup complete."
Write-Host "Build with:"
Write-Host "  powershell -ExecutionPolicy Bypass -File scripts/windows_build_client.ps1 -Configuration $Configuration"
