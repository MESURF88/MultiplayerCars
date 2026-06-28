param(
    [switch]$Install
)

$ErrorActionPreference = "Stop"

function Test-Command($Name) {
    return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function Write-Check($Name, $Ok, $InstallHint) {
    if ($Ok) {
        Write-Host "[ok] $Name"
    } else {
        Write-Host "[missing] $Name"
        if ($InstallHint) {
            Write-Host "          $InstallHint"
        }
    }
}

function Install-WingetPackage($Id, $Name, $Override = $null) {
    if (-not $Install) {
        return
    }
    if (-not (Test-Command winget)) {
        throw "winget is not available. Install prerequisites manually, then rerun without -Install to verify."
    }

    Write-Host "Installing $Name with winget..."
    if ($Override) {
        winget install --id $Id --exact --accept-package-agreements --accept-source-agreements --override $Override
    } else {
        winget install --id $Id --exact --accept-package-agreements --accept-source-agreements
    }
}

Write-Host "Checking Windows build prerequisites..."
Write-Host ""

$hasGit = Test-Command git
$hasCmake = Test-Command cmake
$hasNinja = Test-Command ninja
$hasGo = Test-Command go
$hasVsWhere = Test-Path "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

Write-Check "Git" $hasGit "winget install --id Git.Git --exact"
Write-Check "CMake" $hasCmake "winget install --id Kitware.CMake --exact"
Write-Check "Ninja (optional)" $hasNinja "winget install --id Ninja-build.Ninja --exact"
Write-Check "Go" $hasGo "winget install --id GoLang.Go --exact"
Write-Check "Visual Studio Build Tools / vswhere" $hasVsWhere "winget install --id Microsoft.VisualStudio.2022.BuildTools --exact --override `"--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended`""

Install-WingetPackage "Git.Git" "Git"
Install-WingetPackage "Kitware.CMake" "CMake"
Install-WingetPackage "Ninja-build.Ninja" "Ninja"
Install-WingetPackage "GoLang.Go" "Go"
Install-WingetPackage "Microsoft.VisualStudio.2022.BuildTools" "Visual Studio Build Tools" "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"

Write-Host ""
if ($Install) {
    Write-Host "Install pass complete. Open a new PowerShell terminal so PATH changes are visible, then run:"
} else {
    Write-Host "To install missing tools automatically, rerun:"
    Write-Host "  powershell -ExecutionPolicy Bypass -File scripts/windows_setup_system.ps1 -Install"
    Write-Host ""
    Write-Host "After prerequisites are installed, run:"
}
Write-Host "  powershell -ExecutionPolicy Bypass -File scripts/windows_setup_project.ps1"
