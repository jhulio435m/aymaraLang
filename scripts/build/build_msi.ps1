[CmdletBinding()]
param(
    [string]$DistDir = "dist",
    [string]$OutputDir = "artifacts",
    [switch]$RequireSigning
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$distPath = Join-Path $root $DistDir
$outputPath = Join-Path $root $OutputDir
$productWxs = Join-Path $root "installer\wix\Product.wxs"
$outFile = Join-Path $outputPath "AymaraLang-Setup.msi"
$signScript = Join-Path $root "scripts\build\sign_windows_artifacts.ps1"
$bundledNasm = Join-Path $distPath "toolchain\bin\nasm.exe"
$bundledGcc = Join-Path $distPath "toolchain\mingw64\bin\gcc.exe"

if (-not (Test-Path $productWxs)) {
    throw "No se encontró $productWxs. Verifica installer\wix\Product.wxs."
}
if (-not (Test-Path $distPath)) {
    throw "No se encontró $distPath. Ejecuta build_dist.ps1 primero."
}
if (-not (Test-Path $bundledNasm) -or -not (Test-Path $bundledGcc)) {
    throw "dist no contiene toolchain embebida completa. Ejecuta build_dist.ps1 o bundle_windows_toolchain.ps1 antes de empaquetar."
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

Write-Host "Usando WiX: $($wixCmd.Source)"
Write-Host "Compilando MSI... Version=$version"

& wix build $productWxs `
    -arch x64 `
    -ext WixToolset.Util.wixext `
    -ext WixToolset.UI.wixext `
    -d DistDir="$distPath" `
    -d ProductVersion="$version" `
    -o "$outFile" `
    -v

Write-Host "Signing MSI installer (if configured)..."
& $signScript -Files @($outFile) -RequireSigning:$RequireSigning
if ($LASTEXITCODE -ne 0) {
    throw "Firma de MSI falló con código $LASTEXITCODE."
}

Write-Host "MSI generado en: $outFile"
    
