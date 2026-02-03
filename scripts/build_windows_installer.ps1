Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

param(
  [string]$BuildDir = "build",
  [string]$StagingDir = "dist/windows-installer",
  [string]$OutputDir = "dist",
  [string]$AppVersion = "0.1.0",
  [string]$NsisScript = "scripts/aymara_installer.nsi"
)

function Ensure-Tool {
  param([string]$Name)
  if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
    throw "Required tool '$Name' not found. Please install it and try again."
  }
}

Ensure-Tool -Name "makensis"

$aymcPath = Join-Path $BuildDir "bin/aymc.exe"
if (-not (Test-Path $aymcPath)) {
  throw "Compiler binary not found at $aymcPath. Build it first."
}

$runtimePath = "runtime"
if (-not (Test-Path $runtimePath)) {
  throw "Runtime directory not found at $runtimePath."
}

$stagingBin = Join-Path $StagingDir "bin"
$stagingRuntime = Join-Path $StagingDir "runtime"
$stagingScripts = Join-Path $StagingDir "scripts"

New-Item -ItemType Directory -Force -Path $stagingBin | Out-Null
New-Item -ItemType Directory -Force -Path $stagingRuntime | Out-Null
New-Item -ItemType Directory -Force -Path $stagingScripts | Out-Null

Copy-Item $aymcPath $stagingBin -Force
Copy-Item -Path $runtimePath\* -Destination $stagingRuntime -Recurse -Force
Copy-Item -Path scripts\install_deps_windows.ps1 -Destination $stagingScripts -Force

if (-not (Test-Path $OutputDir)) {
  New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
}

$installerPath = Join-Path $OutputDir "AymaraLang-Setup-$AppVersion.exe"

Write-Host "Building installer: $installerPath"
& makensis "/DAPP_VERSION=$AppVersion" "/DOUTPUT_EXE=$installerPath" "/DSTAGING_DIR=$StagingDir" $NsisScript

Write-Host "Done. Installer created at $installerPath"
