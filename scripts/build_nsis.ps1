[CmdletBinding()]
param(
    [string]$DistDir = "dist",
    [string]$OutputDir = "artifacts"
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$distPath = Join-Path $root $DistDir
$installerPath = Join-Path $root "installer"
$outputPath = Join-Path $root $OutputDir
$nsisScript = Join-Path $installerPath "aymaralang.nsi"
$vcRedist = Join-Path $installerPath "VC_redist.x64.exe"

if (-not (Test-Path $distPath)) {
    throw "No se encontró $distPath. Ejecuta build_dist.ps1 primero."
}

$makensis = Get-Command "makensis" -ErrorAction SilentlyContinue
if (-not $makensis) {
    throw "NSIS (makensis) no está en PATH."
}

if (-not (Test-Path $vcRedist)) {
    Write-Host "Descargando VC_redist.x64.exe..."
    $url = "https://aka.ms/vs/17/release/vc_redist.x64.exe"
    Invoke-WebRequest -Uri $url -OutFile $vcRedist
}

if (-not (Test-Path $outputPath)) {
    New-Item -Path $outputPath -ItemType Directory | Out-Null
}

$outFile = Join-Path $outputPath "AymaraLang-Setup.exe"

Push-Location $root
try {
    & $makensis.Source "/DOUTPUT_FILE=$outFile" $nsisScript
} finally {
    Pop-Location
}

Write-Host "Setup NSIS generado en: $outFile"
