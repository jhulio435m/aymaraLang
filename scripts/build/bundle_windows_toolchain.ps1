[CmdletBinding()]
param(
    [string]$DistDir = "dist",
    [string]$ToolchainRoot = "",
    [string]$NasmPath = ""
)

$ErrorActionPreference = "Stop"

function Write-Log {
    param([string]$Message)
    Write-Host "[bundle-toolchain] $Message"
}

function Invoke-Robocopy {
    param(
        [string]$Source,
        [string]$Destination
    )

    if (-not (Test-Path $Source)) {
        throw "No existe la ruta origen: $Source"
    }

    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    & robocopy $Source $Destination /E /R:2 /W:1 /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) {
        throw "robocopy falló copiando '$Source' a '$Destination' (exit=$LASTEXITCODE)."
    }
}

function Resolve-MingwRoot {
    param([string]$RequestedRoot)

    $candidates = @()
    if (-not [string]::IsNullOrWhiteSpace($RequestedRoot)) {
        $candidates += $RequestedRoot
    } else {
        $gcc = Get-Command "gcc" -ErrorAction SilentlyContinue
        if ($gcc -and $gcc.Source) {
            $gccDir = Split-Path -Parent $gcc.Source
            $candidates += (Split-Path -Parent $gccDir)
        }
        $candidates += @(
            "C:\msys64\ucrt64",
            "C:\msys64\mingw64",
            "C:\MinGW"
        )
    }

    foreach ($candidate in $candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }
        $resolved = $null
        try {
            $resolved = (Resolve-Path $candidate -ErrorAction Stop).Path
        } catch {
            continue
        }
        if (Test-Path (Join-Path $resolved "bin\gcc.exe")) {
            return $resolved
        }
    }

    throw "No se encontró una raíz portable de MinGW/MSYS2 con bin\gcc.exe."
}

function Resolve-NasmExecutable {
    param([string]$RequestedPath)

    $candidates = @()
    if (-not [string]::IsNullOrWhiteSpace($RequestedPath)) {
        $candidates += $RequestedPath
    } else {
        $nasm = Get-Command "nasm" -ErrorAction SilentlyContinue
        if ($nasm -and $nasm.Source) {
            $candidates += $nasm.Source
        }
        $candidates += @(
            "C:\Program Files\NASM\nasm.exe",
            "C:\Program Files (x86)\NASM\nasm.exe"
        )
    }

    foreach ($candidate in $candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw "No se encontró nasm.exe para embebido."
}

$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$distPath = Join-Path $root $DistDir
if (-not (Test-Path $distPath)) {
    throw "No se encontró $distPath. Genera dist antes de embebir toolchain."
}

$mingwRoot = Resolve-MingwRoot -RequestedRoot $ToolchainRoot
$nasmExe = Resolve-NasmExecutable -RequestedPath $NasmPath
$toolchainPath = Join-Path $distPath "toolchain"
$mingwDestRoot = Join-Path $toolchainPath "mingw64"
$toolchainBin = Join-Path $toolchainPath "bin"

Write-Log "Usando MinGW/MSYS2 root: $mingwRoot"
Write-Log "Usando NASM: $nasmExe"

if (Test-Path $toolchainPath) {
    Remove-Item -Path $toolchainPath -Recurse -Force
}

New-Item -ItemType Directory -Path $toolchainBin -Force | Out-Null

foreach ($dirName in @("bin", "include", "lib", "x86_64-w64-mingw32")) {
    $sourceDir = Join-Path $mingwRoot $dirName
    if (Test-Path $sourceDir) {
        Write-Log "Copiando $dirName..."
        Invoke-Robocopy -Source $sourceDir -Destination (Join-Path $mingwDestRoot $dirName)
    }
}

$licensesDir = Join-Path $mingwRoot "share\licenses"
if (Test-Path $licensesDir) {
    Write-Log "Copiando licencias de toolchain..."
    Invoke-Robocopy -Source $licensesDir -Destination (Join-Path $mingwDestRoot "share\licenses")
}

Copy-Item -Path $nasmExe -Destination (Join-Path $toolchainBin "nasm.exe") -Force

$readmePath = Join-Path $toolchainPath "README.txt"
@(
    "Bundled Windows toolchain for AymaraLang."
    "Contents:"
    "- toolchain\\bin\\nasm.exe"
    "- toolchain\\mingw64\\bin\\gcc.exe and related MinGW runtime files"
    ""
    "This toolchain is intended to let aymc compile programs without requiring"
    "a separate system-wide installation of GCC or NASM."
) | Set-Content -Path $readmePath -Encoding ASCII

Write-Log "Toolchain embebida en: $toolchainPath"
$global:LASTEXITCODE = 0
