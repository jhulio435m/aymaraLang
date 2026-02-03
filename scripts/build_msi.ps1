[CmdletBinding()]
param(
    [string]$DistDir = "dist",
    [string]$OutputDir = "artifacts"
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$distPath = Join-Path $root $DistDir
$outputPath = Join-Path $root $OutputDir
$productWxs = Join-Path $root "installer\wix\Product.wxs"
$outFile = Join-Path $outputPath "AymaraLang-Setup.msi"

if (-not (Test-Path $productWxs)) {
    throw "No se encontró $productWxs. Verifica installer\\wix\\Product.wxs."
}

if (-not (Test-Path $distPath)) {
    throw "No se encontró $distPath. Ejecuta build_dist.ps1 primero."
}

$wix = Get-Command "wix" -ErrorAction SilentlyContinue
if (-not $wix) {
    throw "No se encontró el comando 'wix' en PATH. Instala WiX v4 (dotnet tool install --global wix)."
}

if (-not (Test-Path $outputPath)) {
    New-Item -Path $outputPath -ItemType Directory | Out-Null
}

$versionFile = Join-Path $root "VERSION.txt"
$version = "0.1.0"
if (Test-Path $versionFile) {
    $version = (Get-Content $versionFile -Raw).Trim()
}

Write-Host "Compilando MSI con WiX v4..."
& $wix.Source build $productWxs `
    -arch x64 `
    -ext WixToolset.Util.wixext `
    -dDistDir="$distPath" `
    -dProductVersion="$version" `
    -o "$outFile"

Write-Host "MSI generado en: $outFile"
