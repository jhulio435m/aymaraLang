[CmdletBinding()]
param(
    [switch]$EnableLLVM
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")

Write-Host "==> build_dist.ps1"
if ($EnableLLVM.IsPresent) {
    & (Join-Path $PSScriptRoot "build_dist.ps1") -EnableLLVM
} else {
    & (Join-Path $PSScriptRoot "build_dist.ps1")
}

Write-Host "==> build_nsis.ps1"
& (Join-Path $PSScriptRoot "build_nsis.ps1")

Write-Host "==> build_msi.ps1"
& (Join-Path $PSScriptRoot "build_msi.ps1")

Write-Host "Artefactos disponibles en: $(Join-Path $root "artifacts")"
