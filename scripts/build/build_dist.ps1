[CmdletBinding()]
param(
    [string]$BuildDir = "build",
    [ValidateSet("Debug","Release","RelWithDebInfo","MinSizeRel")]
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

$root      = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$buildPath = Join-Path $root $BuildDir
$distPath  = Join-Path $root "dist"

function Assert-ExitOk {
    param([string]$Step)
    if ($LASTEXITCODE -ne 0) {
        throw "$Step falló con código de salida $LASTEXITCODE."
    }
}

if (Test-Path $distPath) {
    Remove-Item -Path $distPath -Recurse -Force
}

if (-not (Test-Path $buildPath)) {
    New-Item -ItemType Directory -Path $buildPath | Out-Null
}

$cmakeArgs = @(
    "-S", $root,
    "-B", $buildPath,
    "-G", "Visual Studio 17 2022",
    "-A", "x64"
)

Write-Host "Configuring CMake..."
& cmake @cmakeArgs
Assert-ExitOk "Configuring CMake"

Write-Host "Building... (Config=$Config)"
& cmake --build $buildPath --config $Config
Assert-ExitOk "Build"

Write-Host "Installing to dist... (Config=$Config)"
& cmake --install $buildPath --config $Config --prefix $distPath
Assert-ExitOk "Install"

Write-Host "dist generado en: $distPath"
