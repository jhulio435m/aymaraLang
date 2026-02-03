Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Log {
  param([string]$Message)
  Write-Host $Message
}

function Test-Command {
  param([string]$Name)
  return $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

Write-Log "Installing Windows build dependencies for AymaraLang..."

if (Test-Command "winget") {
  Write-Log "Detected winget. Installing packages..."
  $packages = @(
    "Kitware.CMake",
    "NASM.NASM",
    "NSIS.NSIS",
    "WiXToolset.WiXToolset",
    "LLVM.LLVM"
  )

  foreach ($package in $packages) {
    Write-Log "Installing $package..."
    winget install --id $package --silent --accept-package-agreements --accept-source-agreements
  }

  Write-Log "NOTE: Install Visual Studio Build Tools (C++ workload) if not present:"
  Write-Log "  winget install --id Microsoft.VisualStudio.2022.BuildTools --silent --accept-package-agreements --accept-source-agreements"
  Write-Log "  After install, open 'Visual Studio Installer' and enable 'Desktop development with C++'."
} elseif (Test-Command "choco") {
  Write-Log "Detected Chocolatey. Installing packages..."
  choco install -y cmake nasm nsis wixtoolset llvm
  Write-Log "NOTE: Install Visual Studio Build Tools (C++ workload) if not present:"
  Write-Log "  choco install -y visualstudio2022buildtools"
} elseif (Test-Command "scoop") {
  Write-Log "Detected Scoop. Installing packages..."
  scoop install cmake nasm nsis wixtoolset llvm
  Write-Log "NOTE: Install Visual Studio Build Tools (C++ workload) if not present."
} else {
  Write-Log "No supported package manager found (winget/choco/scoop)."
  Write-Log "Install these dependencies manually:"
  Write-Log " - Visual Studio Build Tools (Desktop development with C++) or MinGW-w64"
  Write-Log " - CMake"
  Write-Log " - NASM"
  Write-Log " - NSIS (for installer generation)"
  Write-Log " - WiX Toolset (para MSI)"
  Write-Log " - LLVM (optional, for --llvm backend)"
  exit 1
}

Write-Log "Done. You can now build with:"
Write-Log "  cmake -S . -B build -G \"Ninja\" -DCMAKE_BUILD_TYPE=Release"
Write-Log "  cmake --build build -j"
