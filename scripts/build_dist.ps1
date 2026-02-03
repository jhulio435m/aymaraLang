[CmdletBinding()]
param(
    [string]$BuildDir = "build",
    [string]$Config = "Release",
    [switch]$EnableLLVM
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildPath = Join-Path $root $BuildDir
$distPath = Join-Path $root "dist"

if (Test-Path $distPath) {
    Remove-Item -Path $distPath -Recurse -Force
}

$cmakeArgs = @(
    "-S", $root,
    "-B", $buildPath,
    "-G", "Visual Studio 17 2022",
    "-A", "x64"
)

if ($EnableLLVM.IsPresent) {
    $cmakeArgs += "-DAYM_ENABLE_LLVM=ON"
} else {
    $cmakeArgs += "-DAYM_ENABLE_LLVM=OFF"
}

Write-Host "Configuring CMake..."
& cmake @cmakeArgs

Write-Host "Building..."
& cmake --build $buildPath --config $Config

Write-Host "Installing to dist..."
& cmake --install $buildPath --config $Config --prefix $distPath

Write-Host "dist generado en: $distPath"
