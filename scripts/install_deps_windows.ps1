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

  function Install-WingetPackage {
    param(
      [string[]]$Ids,
      [string]$Label
    )

    foreach ($id in $Ids) {
      try {
        Write-Log "Installing $Label ($id)..."
        winget install --id $id --silent --accept-package-agreements --accept-source-agreements
        return $true
      } catch {
        Write-Log "Failed to install $Label with $id. Trying next option..."
      }
    }

    Write-Log "WARNING: Unable to install $Label automatically. Please install it manually."
    return $false
  }

  $basePackages = @(
    "Kitware.CMake",
    "NASM.NASM",
    "NSIS.NSIS",
    "WiXToolset.WiXToolset",
    "LLVM.LLVM"
  )

  foreach ($package in $basePackages) {
    Write-Log "Installing $package..."
    winget install --id $package --silent --accept-package-agreements --accept-source-agreements
  }

  Install-WingetPackage -Ids @(
    "Kitware.Ninja",
    "Ninja-build.Ninja"
  ) -Label "Ninja"

  Install-WingetPackage -Ids @(
    "WinLibs.Mingw-w64",
    "MSYS2.MSYS2"
  ) -Label "MinGW-w64 (GCC/G++)"

  Write-Log "NOTE: If you prefer MSVC instead of MinGW:"
  Write-Log "  winget install --id Microsoft.VisualStudio.2022.BuildTools --silent --accept-package-agreements --accept-source-agreements"
  Write-Log "  After install, open 'Visual Studio Installer' and enable 'Desktop development with C++'."
} elseif (Test-Command "choco") {
  Write-Log "Detected Chocolatey. Installing packages..."
  choco install -y cmake nasm nsis wixtoolset llvm ninja mingw
  Write-Log "NOTE: If you prefer MSVC instead of MinGW:"
  Write-Log "  choco install -y visualstudio2022buildtools"
} elseif (Test-Command "scoop") {
  Write-Log "Detected Scoop. Installing packages..."
  scoop install cmake nasm nsis wixtoolset llvm ninja mingw
  Write-Log "NOTE: If you prefer MSVC instead of MinGW, install Visual Studio Build Tools."
} else {
  Write-Log "No supported package manager found (winget/choco/scoop)."
  Write-Log "Install these dependencies manually:"
  Write-Log " - MinGW-w64 (GCC/G++) or Visual Studio Build Tools (Desktop development with C++)"
  Write-Log " - CMake"
  Write-Log " - Ninja (optional, recommended)"
  Write-Log " - NASM"
  Write-Log " - NSIS (for installer generation)"
  Write-Log " - WiX Toolset (para MSI)"
  Write-Log " - LLVM (optional, for --llvm backend)"
  exit 1
}

Write-Log "Done. You can now build with:"
Write-Log "  cmake -S . -B build -G \"MinGW Makefiles\" -DCMAKE_BUILD_TYPE=Release"
Write-Log "  cmake -S . -B build -G \"Ninja\" -DCMAKE_BUILD_TYPE=Release"
Write-Log "  cmake --build build -j"
