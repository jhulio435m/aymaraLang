[CmdletBinding()]
param(
    [string]$DistDir = "dist",
    [string]$OutputDir = "artifacts",
    [switch]$RequireSigning
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$distPath = Join-Path $root $DistDir
$installerPath = Join-Path $root "installer"
$outputPath = Join-Path $root $OutputDir
$nsisScript = Join-Path $installerPath "aymaralang.nsi"
$vcRedist = Join-Path $installerPath "VC_redist.x64.exe"
$versionFile = Join-Path $root "VERSION.txt"
$signScript = Join-Path $root "scripts\build\sign_windows_artifacts.ps1"
$bundledNasm = Join-Path $distPath "toolchain\bin\nasm.exe"
$bundledGcc = Join-Path $distPath "toolchain\mingw64\bin\gcc.exe"
$version = "0.1.0"

if (-not (Test-Path $distPath)) {
    throw "No se encontró $distPath. Ejecuta build_dist.ps1 primero."
}
if (-not (Test-Path $bundledNasm) -or -not (Test-Path $bundledGcc)) {
    throw "dist no contiene toolchain embebida completa. Ejecuta build_dist.ps1 o bundle_windows_toolchain.ps1 antes de empaquetar."
}

if (Test-Path $versionFile) {
    $version = (Get-Content $versionFile -Raw).Trim()
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
    Write-Host "Compilando NSIS... Version=$version"
    & $makensis.Source "/DOUTPUT_FILE=$outFile" "/DPRODUCT_VERSION=$version" "/DDIST_ROOT=$distPath" "/DVC_REDIST_SOURCE=$vcRedist" $nsisScript
} finally {
    Pop-Location
}

Write-Host "Signing NSIS installer (if configured)..."
& $signScript -Files @($outFile) -RequireSigning:$RequireSigning
if ($LASTEXITCODE -ne 0) {
    throw "Firma de Setup NSIS falló con código $LASTEXITCODE."
}

Write-Host "Setup NSIS generado en: $outFile"
