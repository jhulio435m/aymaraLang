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
    throw "No se encontró $productWxs. Verifica installer\wix\Product.wxs."
}
if (-not (Test-Path $distPath)) {
    throw "No se encontró $distPath. Ejecuta build_dist.ps1 primero."
}

$wixCmd = Get-Command "wix" -ErrorAction SilentlyContinue
if (-not $wixCmd) {
    throw "No se encontró el comando 'wix' en PATH. Instala WiX: dotnet tool install --global wix"
}

if (-not (Test-Path $outputPath)) {
    New-Item -Path $outputPath -ItemType Directory | Out-Null
}

# Borrar MSI anterior para no confundir
if (Test-Path $outFile) {
    Remove-Item $outFile -Force
}

# Versión
$versionFile = Join-Path $root "VERSION.txt"
$version = "0.1.0"
if (Test-Path $versionFile) {
    $version = (Get-Content $versionFile -Raw).Trim()
}

# --- Extensiones necesarias (Util para PATH, UI para WixUI_*) ---
$requiredExts = @(
    "WixToolset.Util.wixext",
    "WixToolset.UI.wixext"
)

$installedText = ""
try {
    $installedText = (& wix extension list -g 2>$null) -join "`n"
} catch {
    $installedText = ""
}

foreach ($ext in $requiredExts) {
    if ($installedText -notmatch [regex]::Escape($ext)) {
        Write-Host "Instalando extensión WiX: $ext"
        & wix extension add -g $ext | Out-Null
    }
}

# --- Detectar si hay backend LLVM en dist ---
$llvmDir = Join-Path $distPath "llvm-backend"
$hasLlvm = (Test-Path $llvmDir) -and `
           ((Get-ChildItem $llvmDir -Recurse -File -ErrorAction SilentlyContinue | Measure-Object).Count -gt 0)
$hasLlvmValue = if ($hasLlvm) { "1" } else { "0" }

Write-Host "Usando WiX: $($wixCmd.Source)"
Write-Host "Compilando MSI... Version=$version HasLlvmBackend=$hasLlvmValue"

& wix build $productWxs `
    -arch x64 `
    -ext WixToolset.Util.wixext `
    -ext WixToolset.UI.wixext `
    -d DistDir="$distPath" `
    -d ProductVersion="$version" `
    -d HasLlvmBackend="$hasLlvmValue" `
    -o "$outFile" `
    -v

Write-Host "MSI generado en: $outFile"
    