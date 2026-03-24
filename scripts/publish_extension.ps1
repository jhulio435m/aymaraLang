param(
  [ValidateSet("package", "vscode", "openvsx", "all")]
  [string]$Target = "all",

  [switch]$SkipInstall,

  [string]$VsceToken = $env:VSCE_PAT,

  [string]$OvsxToken = $env:OVSX_PAT
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent $PSScriptRoot
$extensionDir = Join-Path $repoRoot "aymlang"

if (-not (Test-Path $extensionDir)) {
  throw "No se encontro aymlang/ en $repoRoot"
}

function Invoke-Step {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Command,

    [Parameter(Mandatory = $true)]
    [string[]]$Arguments,

    [Parameter(Mandatory = $true)]
    [string]$WorkingDirectory,

    [Parameter(Mandatory = $true)]
    [string]$Label
  )

  Write-Output "[publish] $Label"
  & $Command @Arguments
  if ($LASTEXITCODE -ne 0) {
    throw "Fallo '$Label' (exit=$LASTEXITCODE)"
  }
}

function Require-Token {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Value,

    [Parameter(Mandatory = $true)]
    [string]$Name,

    [Parameter(Mandatory = $true)]
    [string]$HelpText
  )

  if ([string]::IsNullOrWhiteSpace($Value)) {
    throw "Falta token '$Name'. $HelpText"
  }
}

Push-Location $extensionDir
try {
  if (-not $SkipInstall) {
    Invoke-Step -Command "npm" -Arguments @("install") -WorkingDirectory $extensionDir -Label "npm install"
  }

  Invoke-Step -Command "npx" -Arguments @("@vscode/vsce", "package") -WorkingDirectory $extensionDir -Label "vsce package"

  if ($Target -eq "package") {
    $vsix = Get-ChildItem -Path $extensionDir -Filter "*.vsix" | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
    if ($null -ne $vsix) {
      Write-Output "[publish] VSIX generado: $($vsix.FullName)"
    }
    exit 0
  }

  if ($Target -eq "vscode" -or $Target -eq "all") {
    Require-Token -Value $VsceToken -Name "VSCE_PAT" -HelpText "Exporta VSCE_PAT o pasa -VsceToken."
    Invoke-Step -Command "npx" -Arguments @("@vscode/vsce", "publish", "--pat", $VsceToken) -WorkingDirectory $extensionDir -Label "publicar VS Code Marketplace"
  }

  if ($Target -eq "openvsx" -or $Target -eq "all") {
    Require-Token -Value $OvsxToken -Name "OVSX_PAT" -HelpText "Exporta OVSX_PAT o pasa -OvsxToken."
    Invoke-Step -Command "npx" -Arguments @("ovsx", "publish", "-p", $OvsxToken) -WorkingDirectory $extensionDir -Label "publicar Open VSX"
  }

  Write-Output "[publish] Publicacion completada"
}
finally {
  Pop-Location
}
