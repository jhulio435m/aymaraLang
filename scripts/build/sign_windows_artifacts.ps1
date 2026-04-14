[CmdletBinding()]
param(
    [string[]]$Files = @(),
    [string]$PfxPath = $env:AYM_SIGN_PFX_PATH,
    [string]$PfxPassword = $env:AYM_SIGN_PFX_PASSWORD,
    [string]$CertThumbprint = $env:AYM_SIGN_CERT_THUMBPRINT,
    [string]$TimestampUrl = $env:AYM_SIGN_TIMESTAMP_URL,
    [string]$SignToolPath = $env:AYM_SIGNTOOL_PATH,
    [switch]$RequireSigning
)

$ErrorActionPreference = "Stop"

function Write-Log {
    param([string]$Message)
    Write-Host "[sign] $Message"
}

function Find-SignTool {
    param([string]$RequestedPath)

    if (-not [string]::IsNullOrWhiteSpace($RequestedPath)) {
        if (Test-Path $RequestedPath) {
            return (Resolve-Path $RequestedPath).Path
        }
        throw "No se encontró signtool en la ruta indicada: $RequestedPath"
    }

    $signtool = Get-Command "signtool.exe" -ErrorAction SilentlyContinue
    if ($signtool -and $signtool.Source) {
        return $signtool.Source
    }

    $sdkRoots = @(
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin",
        "${env:ProgramFiles}\Windows Kits\10\bin"
    )

    foreach ($sdkRoot in $sdkRoots) {
        if (-not (Test-Path $sdkRoot)) {
            continue
        }
        $candidate = Get-ChildItem -Path $sdkRoot -Recurse -Filter "signtool.exe" -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($candidate) {
            return $candidate.FullName
        }
    }

    return $null
}

function Get-SigningMode {
    param(
        [string]$PfxPath,
        [string]$CertThumbprint
    )

    if (-not [string]::IsNullOrWhiteSpace($PfxPath)) {
        return "pfx"
    }
    if (-not [string]::IsNullOrWhiteSpace($CertThumbprint)) {
        return "thumbprint"
    }
    return ""
}

if (-not $TimestampUrl) {
    $TimestampUrl = "http://timestamp.digicert.com"
}

$resolvedFiles = @()
foreach ($file in $Files) {
    if ([string]::IsNullOrWhiteSpace($file)) {
        continue
    }
    if (-not (Test-Path $file)) {
        throw "No se encontró el archivo a firmar: $file"
    }
    $resolvedFiles += (Resolve-Path $file).Path
}

if ($resolvedFiles.Count -eq 0) {
    Write-Log "No se recibieron archivos para firmar."
    exit 0
}

$signingMode = Get-SigningMode -PfxPath $PfxPath -CertThumbprint $CertThumbprint
if (-not $signingMode) {
    if ($RequireSigning) {
        throw "La firma es obligatoria, pero no se configuró AYM_SIGN_PFX_PATH ni AYM_SIGN_CERT_THUMBPRINT."
    }
    Write-Log "Firma deshabilitada: no hay certificado configurado."
    exit 0
}

$resolvedSignTool = Find-SignTool -RequestedPath $SignToolPath
if (-not $resolvedSignTool) {
    if ($RequireSigning) {
        throw "La firma es obligatoria, pero no se encontró signtool.exe."
    }
    Write-Log "Firma omitida: no se encontró signtool.exe."
    exit 0
}

$commonArgs = @(
    "sign",
    "/fd", "SHA256",
    "/tr", $TimestampUrl,
    "/td", "SHA256",
    "/v"
)

if ($signingMode -eq "pfx") {
    if (-not (Test-Path $PfxPath)) {
        throw "No se encontró el certificado PFX: $PfxPath"
    }
    $commonArgs += @("/f", (Resolve-Path $PfxPath).Path)
    if (-not [string]::IsNullOrWhiteSpace($PfxPassword)) {
        $commonArgs += @("/p", $PfxPassword)
    }
} elseif ($signingMode -eq "thumbprint") {
    $commonArgs += @("/sha1", $CertThumbprint)
    if ($env:AYM_SIGN_CERT_MACHINE_STORE -eq "1") {
        $commonArgs += "/sm"
    }
}

Write-Log "Usando signtool: $resolvedSignTool"
Write-Log "Modo de firma: $signingMode"

foreach ($file in $resolvedFiles) {
    Write-Log "Firmando $file"
    & $resolvedSignTool @commonArgs $file
    if ($LASTEXITCODE -ne 0) {
        throw "signtool falló firmando $file con código $LASTEXITCODE."
    }
}

Write-Log "Firma completada."
