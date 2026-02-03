[CmdletBinding()]
param(
    [switch]$EnableLLVM
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$artifactsDir = Join-Path $root "artifacts"

function Invoke-Step {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Label,
        [Parameter(Mandatory = $true)]
        [string]$ScriptPath,
        [string[]]$Arguments = @()
    )

    Write-Host "==> $Label"
    if ($Arguments.Count -gt 0) {
        & $ScriptPath @Arguments
    } else {
        & $ScriptPath
    }

    if ($LASTEXITCODE -ne 0) {
        throw "$Label falló con código de salida $LASTEXITCODE."
    }
}

$distArgs = @()
if ($EnableLLVM.IsPresent) {
    $distArgs += "-EnableLLVM"
}

Invoke-Step -Label "build_dist.ps1" -ScriptPath (Join-Path $PSScriptRoot "build_dist.ps1") -Arguments $distArgs
Invoke-Step -Label "build_nsis.ps1" -ScriptPath (Join-Path $PSScriptRoot "build_nsis.ps1")
Invoke-Step -Label "build_msi.ps1" -ScriptPath (Join-Path $PSScriptRoot "build_msi.ps1")

$exePath = Join-Path $artifactsDir "AymaraLang-Setup.exe"
$msiPath = Join-Path $artifactsDir "AymaraLang-Setup.msi"

Write-Host "Artefacto EXE: $exePath"
Write-Host "Artefacto MSI: $msiPath"
