Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

param(
  [string]$BuildDir = "build",
  [string]$StagingDir = "dist/windows-installer",
  [string]$OutputDir = "dist",
  [string]$AppVersion = "0.1.0",
  [string]$WxsScript = "scripts/aymara_installer.wxs"
)

function Ensure-Tool {
  param([string]$Name)
  if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
    throw "Required tool '$Name' not found. Please install it and try again."
  }
}

Ensure-Tool -Name "heat"
Ensure-Tool -Name "candle"
Ensure-Tool -Name "light"

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
Copy-Item -Path README.md -Destination $StagingDir -Force
Copy-Item -Path LICENSE -Destination $StagingDir -Force

if (-not (Test-Path $OutputDir)) {
  New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
}

$harvestFile = Join-Path $StagingDir "harvest.wxs"
$wixobjMain = Join-Path $StagingDir "aymara_installer.wixobj"
$wixobjHarvest = Join-Path $StagingDir "harvest.wixobj"
$msiPath = Join-Path $OutputDir "AymaraLang-Setup-$AppVersion.msi"

Write-Host "Harvesting files for MSI..."
& heat dir $StagingDir -cg AymaraFiles -dr INSTALLDIR -srd -gg -sfrag -out $harvestFile

Write-Host "Building MSI: $msiPath"
& candle -dProductVersion=$AppVersion -dSourceDir=$StagingDir -out $wixobjMain $WxsScript
& candle -dProductVersion=$AppVersion -dSourceDir=$StagingDir -out $wixobjHarvest $harvestFile
& light -out $msiPath $wixobjMain $wixobjHarvest

Write-Host "Done. Installer created at $msiPath"
