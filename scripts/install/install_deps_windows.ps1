[CmdletBinding()]
param(
  [ValidateSet("auto", "winget", "choco", "scoop")]
  [string]$PackageManager = "auto",
  [switch]$CheckOnly,
  [switch]$SkipPackagingTools
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Log {
  param(
    [string]$Message,
    [ValidateSet("INFO", "WARN", "ERROR")]
    [string]$Level = "INFO"
  )

  switch ($Level) {
    "WARN"  { Write-Host "[WARN] $Message" -ForegroundColor Yellow }
    "ERROR" { Write-Host "[ERROR] $Message" -ForegroundColor Red }
    default { Write-Host "[INFO] $Message" }
  }
}

function Test-Command {
  param([string]$Name)
  return $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

function Refresh-SessionPath {
  $machinePath = [Environment]::GetEnvironmentVariable("Path", "Machine")
  $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
  $env:Path = "$machinePath;$userPath"
}

function Add-SessionPathIfExists {
  param([string]$Directory)

  if ([string]::IsNullOrWhiteSpace($Directory)) {
    return
  }
  if (-not (Test-Path $Directory)) {
    return
  }

  $normalized = [System.IO.Path]::GetFullPath($Directory).TrimEnd("\")
  $entries = @($env:Path -split ";" | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
  $already = $false
  foreach ($entry in $entries) {
    try {
      $entryNorm = [System.IO.Path]::GetFullPath($entry).TrimEnd("\")
      if ($entryNorm -ieq $normalized) {
        $already = $true
        break
      }
    } catch {
      # Keep going even if one PATH segment is malformed.
    }
  }

  if (-not $already) {
    $env:Path = "$normalized;$env:Path"
    Write-Log "Added to current PATH: $normalized"
  }
}

function Get-CommandPath {
  param(
    [string]$Name,
    [string[]]$CandidateDirs = @()
  )

  $cmd = Get-Command $Name -ErrorAction SilentlyContinue
  if ($cmd) {
    return $cmd.Source
  }

  foreach ($dir in $CandidateDirs) {
    Add-SessionPathIfExists -Directory $dir
  }

  $cmd = Get-Command $Name -ErrorAction SilentlyContinue
  if ($cmd) {
    return $cmd.Source
  }

  return $null
}

function Get-MsvcInstallationPath {
  $programFilesX86 = ${env:ProgramFiles(x86)}
  if ([string]::IsNullOrWhiteSpace($programFilesX86)) {
    return $null
  }

  $vswhere = Join-Path $programFilesX86 "Microsoft Visual Studio\Installer\vswhere.exe"
  if (-not (Test-Path $vswhere)) {
    return $null
  }

  $path = & $vswhere `
    -latest `
    -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath

  if ($LASTEXITCODE -ne 0) {
    return $null
  }
  if ([string]::IsNullOrWhiteSpace($path)) {
    return $null
  }

  return $path.Trim()
}

function Get-PreferredPackageManager {
  param([string]$Requested)

  if ($Requested -ne "auto") {
    if (-not (Test-Command $Requested)) {
      throw "Requested package manager '$Requested' is not available in PATH."
    }
    return $Requested
  }

  foreach ($manager in @("winget", "choco", "scoop")) {
    if (Test-Command $manager) {
      return $manager
    }
  }

  return $null
}

function Test-WingetInstalled {
  param([string]$Id)

  $output = & winget list --id $Id --exact --accept-source-agreements 2>$null
  if ($LASTEXITCODE -ne 0) {
    return $false
  }

  $text = ($output -join "`n")
  return $text -match [regex]::Escape($Id)
}

function Install-WingetAny {
  param(
    [string[]]$Ids,
    [string]$Label,
    [switch]$Optional
  )

  foreach ($id in $Ids) {
    if (Test-WingetInstalled -Id $id) {
      Write-Log "$Label already installed ($id)."
      return $true
    }

    Write-Log "Installing $Label with winget ($id)..."
    & winget install `
      --id $id `
      --exact `
      --silent `
      --accept-package-agreements `
      --accept-source-agreements `
      --disable-interactivity

    if ($LASTEXITCODE -eq 0) {
      Write-Log "$Label installed with winget ($id)."
      return $true
    }

    Write-Log "winget failed for $id (exit=$LASTEXITCODE)." "WARN"
  }

  if ($Optional) {
    Write-Log "Unable to install optional dependency: $Label" "WARN"
    return $false
  }

  throw "Unable to install required dependency: $Label"
}

function Install-ChocoAny {
  param(
    [string[]]$Packages,
    [string]$Label,
    [switch]$Optional
  )

  foreach ($pkg in $Packages) {
    Write-Log "Installing $Label with chocolatey ($pkg)..."
    & choco install -y $pkg --no-progress
    if ($LASTEXITCODE -eq 0) {
      Write-Log "$Label installed with chocolatey ($pkg)."
      return $true
    }
    Write-Log "choco failed for $pkg (exit=$LASTEXITCODE)." "WARN"
  }

  if ($Optional) {
    Write-Log "Unable to install optional dependency: $Label" "WARN"
    return $false
  }

  throw "Unable to install required dependency: $Label"
}

function Ensure-ScoopBucket {
  param([string]$Bucket)

  $listed = & scoop bucket list 2>$null
  if ($LASTEXITCODE -ne 0) {
    return
  }

  $joined = ($listed -join "`n")
  if ($joined -match "(?m)^\s*$([regex]::Escape($Bucket))\s*$") {
    return
  }

  Write-Log "Adding scoop bucket '$Bucket'..."
  & scoop bucket add $Bucket
  if ($LASTEXITCODE -ne 0) {
    Write-Log "Could not add scoop bucket '$Bucket'." "WARN"
  }
}

function Install-ScoopAny {
  param(
    [string[]]$Packages,
    [string]$Label,
    [switch]$Optional
  )

  foreach ($pkg in $Packages) {
    Write-Log "Installing $Label with scoop ($pkg)..."
    & scoop install $pkg
    if ($LASTEXITCODE -eq 0) {
      Write-Log "$Label installed with scoop ($pkg)."
      return $true
    }
    Write-Log "scoop failed for $pkg (exit=$LASTEXITCODE)." "WARN"
  }

  if ($Optional) {
    Write-Log "Unable to install optional dependency: $Label" "WARN"
    return $false
  }

  throw "Unable to install required dependency: $Label"
}

function Install-Dependency {
  param(
    [string]$Manager,
    [string]$Label,
    [string[]]$WingetIds,
    [string[]]$ChocoPackages,
    [string[]]$ScoopPackages,
    [switch]$Optional
  )

  switch ($Manager) {
    "winget" { return Install-WingetAny -Ids $WingetIds -Label $Label -Optional:$Optional }
    "choco"  { return Install-ChocoAny -Packages $ChocoPackages -Label $Label -Optional:$Optional }
    "scoop"  { return Install-ScoopAny -Packages $ScoopPackages -Label $Label -Optional:$Optional }
    default  { throw "Unsupported package manager: $Manager" }
  }
}

function Install-Msys2Toolchain {
  $msysBash = "C:\msys64\usr\bin\bash.exe"
  if (-not (Test-Path $msysBash)) {
    return $false
  }

  Write-Log "MSYS2 detected. Installing gcc/nasm toolchain via pacman..."

  $packageSets = @(
    "mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-nasm",
    "mingw-w64-x86_64-gcc mingw-w64-x86_64-nasm"
  )

  foreach ($set in $packageSets) {
    & $msysBash -lc "pacman -Sy --noconfirm --needed $set"
    if ($LASTEXITCODE -eq 0) {
      Add-SessionPathIfExists -Directory "C:\msys64\ucrt64\bin"
      Add-SessionPathIfExists -Directory "C:\msys64\mingw64\bin"
      Write-Log "MSYS2 toolchain installed successfully."
      return $true
    }
    Write-Log "MSYS2 package set failed: $set" "WARN"
  }

  Write-Log "Unable to install MSYS2 gcc/nasm packages automatically." "WARN"
  return $false
}

function Get-ToolStatus {
  $programFiles = $env:ProgramFiles
  $programFilesX86 = ${env:ProgramFiles(x86)}

  $cmakePath = Get-CommandPath -Name "cmake" -CandidateDirs @(
    "$programFiles\CMake\bin"
  )

  $nasmPath = Get-CommandPath -Name "nasm" -CandidateDirs @(
    "$programFiles\NASM",
    "$programFiles\NASM\bin",
    "C:\msys64\ucrt64\bin",
    "C:\msys64\mingw64\bin",
    "C:\MinGW\bin"
  )

  $gccPath = Get-CommandPath -Name "gcc" -CandidateDirs @(
    "C:\msys64\ucrt64\bin",
    "C:\msys64\mingw64\bin",
    "C:\msys64\usr\bin",
    "C:\MinGW\bin"
  )

  $gppPath = Get-CommandPath -Name "g++" -CandidateDirs @(
    "C:\msys64\ucrt64\bin",
    "C:\msys64\mingw64\bin",
    "C:\msys64\usr\bin",
    "C:\MinGW\bin"
  )

  $clPath = Get-CommandPath -Name "cl" -CandidateDirs @()
  $msvcInstallPath = Get-MsvcInstallationPath

  $ninjaPath = Get-CommandPath -Name "ninja" -CandidateDirs @(
    "$programFiles\Ninja",
    "$programFiles\Ninja\bin"
  )

  $nsisPath = Get-CommandPath -Name "makensis" -CandidateDirs @(
    "$programFilesX86\NSIS",
    "$programFiles\NSIS"
  )

  $wixPath = Get-CommandPath -Name "wix" -CandidateDirs @(
    "$programFiles\WiX Toolset v6.0\bin",
    "$programFiles\WiX Toolset v5.0\bin",
    "$programFilesX86\WiX Toolset v3.11\bin"
  )

  $hasCompiler = (($null -ne $gccPath) -and ($null -ne $gppPath)) -or ($null -ne $clPath) -or ($null -ne $msvcInstallPath)

  return [pscustomobject]@{
    CMakePath = $cmakePath
    NasmPath = $nasmPath
    GccPath = $gccPath
    GppPath = $gppPath
    ClPath = $clPath
    MsvcInstallPath = $msvcInstallPath
    HasCompiler = $hasCompiler
    NinjaPath = $ninjaPath
    NsisPath = $nsisPath
    WixPath = $wixPath
  }
}

function Show-ToolStatus {
  param([pscustomobject]$Status)

  if ($Status.CMakePath) { Write-Log "cmake: $($Status.CMakePath)" } else { Write-Log "cmake: missing" "WARN" }
  if ($Status.NasmPath)  { Write-Log "nasm: $($Status.NasmPath)" } else { Write-Log "nasm: missing" "WARN" }
  if ($Status.GccPath)   { Write-Log "gcc: $($Status.GccPath)" }
  if ($Status.GppPath)   { Write-Log "g++: $($Status.GppPath)" }
  if ($Status.ClPath)    { Write-Log "cl: $($Status.ClPath)" }
  if ($Status.MsvcInstallPath) {
    Write-Log "MSVC Build Tools detected: $($Status.MsvcInstallPath)"
  }
  if (-not $Status.HasCompiler) {
    Write-Log "Compiler toolchain: missing (need gcc/g++ or MSVC Build Tools)" "WARN"
  }
  if ($Status.NinjaPath) {
    Write-Log "ninja: $($Status.NinjaPath)"
  } else {
    Write-Log "ninja: not found (optional)" "WARN"
  }
  if ($Status.NsisPath) {
    Write-Log "makensis: $($Status.NsisPath)"
  } elseif (-not $SkipPackagingTools) {
    Write-Log "makensis: not found (optional for installer packaging)" "WARN"
  }
  if ($Status.WixPath) {
    Write-Log "wix: $($Status.WixPath)"
  } elseif (-not $SkipPackagingTools) {
    Write-Log "wix: not found (optional for MSI packaging)" "WARN"
  }
}

function Get-MissingRequired {
  param([pscustomobject]$Status)

  $missing = @()
  if (-not $Status.CMakePath) { $missing += "CMake" }
  if (-not $Status.NasmPath) { $missing += "NASM" }
  if (-not $Status.HasCompiler) { $missing += "Compiler (gcc/g++ or MSVC Build Tools)" }
  return ,$missing
}

Write-Log "Checking Windows build dependencies for AymaraLang..."
Refresh-SessionPath
Add-SessionPathIfExists -Directory "C:\msys64\ucrt64\bin"
Add-SessionPathIfExists -Directory "C:\msys64\mingw64\bin"
Add-SessionPathIfExists -Directory "C:\MinGW\bin"

$status = Get-ToolStatus
Show-ToolStatus -Status $status
$missingRequired = Get-MissingRequired -Status $status

if ($CheckOnly) {
  if ($missingRequired.Count -gt 0) {
    throw "Missing required dependencies: $($missingRequired -join ', ')"
  }

  Write-Log "All required dependencies are available."
  exit 0
}

if ($missingRequired.Count -gt 0) {
  $manager = Get-PreferredPackageManager -Requested $PackageManager
  if (-not $manager) {
    throw "No supported package manager found (winget/choco/scoop). Install required dependencies manually: CMake, NASM, and gcc/g++ or MSVC Build Tools."
  }

  Write-Log "Using package manager: $manager"

  if ($manager -eq "scoop") {
    Ensure-ScoopBucket -Bucket "main"
    Ensure-ScoopBucket -Bucket "extras"
  }

  if (-not $status.CMakePath) {
    Install-Dependency -Manager $manager -Label "CMake" `
      -WingetIds @("Kitware.CMake") `
      -ChocoPackages @("cmake") `
      -ScoopPackages @("main/cmake", "cmake")
  }

  if (-not $status.NasmPath) {
    Install-Dependency -Manager $manager -Label "NASM" `
      -WingetIds @("NASM.NASM") `
      -ChocoPackages @("nasm") `
      -ScoopPackages @("main/nasm", "nasm")
  }

  if (-not $status.HasCompiler) {
    Install-Dependency -Manager $manager -Label "C/C++ toolchain" `
      -WingetIds @("WinLibs.Mingw-w64", "MSYS2.MSYS2") `
      -ChocoPackages @("mingw", "msys2") `
      -ScoopPackages @("main/mingw", "mingw") `
      -Optional:$false

    Install-Msys2Toolchain | Out-Null
  }
}

$status = Get-ToolStatus

if (-not $status.NinjaPath) {
  $manager = Get-PreferredPackageManager -Requested $PackageManager
  if ($manager) {
    Install-Dependency -Manager $manager -Label "Ninja" `
      -WingetIds @("Ninja-build.Ninja", "Kitware.Ninja") `
      -ChocoPackages @("ninja") `
      -ScoopPackages @("main/ninja", "ninja") `
      -Optional
  }
}

if (-not $SkipPackagingTools) {
  $manager = Get-PreferredPackageManager -Requested $PackageManager
  if ($manager) {
    if (-not $status.NsisPath) {
      Install-Dependency -Manager $manager -Label "NSIS" `
        -WingetIds @("NSIS.NSIS") `
        -ChocoPackages @("nsis") `
        -ScoopPackages @("extras/nsis", "nsis") `
        -Optional
    }

    if (-not $status.WixPath) {
      Install-Dependency -Manager $manager -Label "WiX Toolset" `
        -WingetIds @("WiXToolset.WiXToolset") `
        -ChocoPackages @("wixtoolset") `
        -ScoopPackages @("extras/wixtoolset", "wixtoolset", "wix") `
        -Optional
    }
  }
}

Refresh-SessionPath
Add-SessionPathIfExists -Directory "C:\msys64\ucrt64\bin"
Add-SessionPathIfExists -Directory "C:\msys64\mingw64\bin"
Add-SessionPathIfExists -Directory "C:\MinGW\bin"

$finalStatus = Get-ToolStatus
Show-ToolStatus -Status $finalStatus
$finalMissing = Get-MissingRequired -Status $finalStatus
if ($finalMissing.Count -gt 0) {
  throw "Missing required dependencies after install: $($finalMissing -join ', '). Open a new shell and rerun this script with -CheckOnly."
}

Write-Log ""
Write-Log "Done. Required dependencies are available."
Write-Log "If MSVC was installed, run from 'x64 Native Tools Command Prompt for VS' for cl.exe."
Write-Log "Build examples:"
Write-Log "  cmake -S . -B build -G \"MinGW Makefiles\" -DCMAKE_BUILD_TYPE=Release"
Write-Log "  cmake --build build -j"
Write-Log "  # or"
Write-Log "  cmake -S . -B build -G \"Visual Studio 17 2022\" -A x64"
Write-Log "  cmake --build build --config Release"
