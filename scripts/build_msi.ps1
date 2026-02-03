[CmdletBinding()]
param(
    [string]$DistDir = "dist",
    [string]$OutputDir = "artifacts"
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$distPath = Join-Path $root $DistDir
$wixRoot = Join-Path $root "installer\\wix"
$generatedDir = Join-Path $wixRoot "Generated"
$outputPath = Join-Path $root $OutputDir
$productWxs = Join-Path $wixRoot "Product.wxs"

if (-not (Test-Path $distPath)) {
    throw "No se encontró $distPath. Ejecuta build_dist.ps1 primero."
}

$heat = Get-Command "heat" -ErrorAction SilentlyContinue
$candle = Get-Command "candle" -ErrorAction SilentlyContinue
$light = Get-Command "light" -ErrorAction SilentlyContinue

if (-not $heat -or -not $candle -or -not $light) {
    throw "WiX Toolset (heat/candle/light) no está en PATH."
}

if (-not (Test-Path $generatedDir)) {
    New-Item -Path $generatedDir -ItemType Directory | Out-Null
}

$versionFile = Join-Path $root "VERSION.txt"
$version = "0.1.0"
if (Test-Path $versionFile) {
    $version = (Get-Content $versionFile -Raw).Trim()
}

$coreBinWxs = Join-Path $generatedDir "CoreBin.wxs"
$coreShareWxs = Join-Path $generatedDir "CoreShare.wxs"
$llvmWxs = Join-Path $generatedDir "LLVM.wxs"

$distBin = Join-Path $distPath "bin"
$distShare = Join-Path $distPath "share"
$distLLVM = Join-Path $distPath "llvm-backend"

if (-not (Test-Path $distBin)) {
    throw "No se encontró $distBin."
}

Write-Host "Generando fragmentos WiX con heat..."
& $heat.Source dir $distBin -cg CoreBinFiles -dr BINDIR -gg -ag -srd -sreg -var var.DistDir -out $coreBinWxs

if (Test-Path $distShare) {
    & $heat.Source dir $distShare -cg CoreShareFiles -dr SHAREDIR -gg -ag -srd -sreg -var var.DistDir -out $coreShareWxs
} else {
    @"
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<Wix xmlns=\"http://schemas.microsoft.com/wix/2006/wi\">
  <Fragment>
    <ComponentGroup Id=\"CoreShareFiles\" />
  </Fragment>
</Wix>
"@ | Set-Content -Path $coreShareWxs
}

if (Test-Path $distLLVM) {
    & $heat.Source dir $distLLVM -cg LLVMFiles -dr LLVMBACKENDDIR -gg -ag -srd -sreg -var var.DistDir -out $llvmWxs
} else {
    @"
<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<Wix xmlns=\"http://schemas.microsoft.com/wix/2006/wi\">
  <Fragment>
    <ComponentGroup Id=\"LLVMFiles\" />
  </Fragment>
</Wix>
"@ | Set-Content -Path $llvmWxs
}

$defines = @(
    "-dDistDir=$distPath",
    "-dProductVersion=$version"
)

$iconPath = Join-Path $root "assets\\logo.ico"
if (Test-Path $iconPath) {
    $defines += "-dProductIcon=$iconPath"
}

$bannerPath = Join-Path $root "assets\\banner.bmp"
if (Test-Path $bannerPath) {
    $defines += "-dWixBanner=$bannerPath"
}

$dialogPath = Join-Path $root "assets\\dialog.bmp"
if (Test-Path $dialogPath) {
    $defines += "-dWixDialog=$dialogPath"
}

if (-not (Test-Path $outputPath)) {
    New-Item -Path $outputPath -ItemType Directory | Out-Null
}

$wixObjDir = Join-Path $generatedDir "obj"
if (-not (Test-Path $wixObjDir)) {
    New-Item -Path $wixObjDir -ItemType Directory | Out-Null
}

$wxsFiles = @($productWxs, $coreBinWxs, $coreShareWxs, $llvmWxs)

Write-Host "Compilando WXS con candle..."
& $candle.Source -nologo -ext WixUtilExtension -out (Join-Path $wixObjDir "") @defines @wxsFiles

$wixObjs = Get-ChildItem -Path $wixObjDir -Filter "*.wixobj" | ForEach-Object { $_.FullName }
$outFile = Join-Path $outputPath "AymaraLang-Setup.msi"

Write-Host "Linkeando MSI con light..."
& $light.Source -nologo -ext WixUtilExtension -out $outFile $wixObjs

Write-Host "MSI generado en: $outFile"
