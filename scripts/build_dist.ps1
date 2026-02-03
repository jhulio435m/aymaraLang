[CmdletBinding()]
param(
    [string]$BuildDir = "build",
    [ValidateSet("Debug","Release","RelWithDebInfo","MinSizeRel")]
    [string]$Config = "Release",
    [switch]$EnableLLVM,
    [string]$LLVMDir
)

$ErrorActionPreference = "Stop"

$root      = Resolve-Path (Join-Path $PSScriptRoot "..")
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

if ($EnableLLVM.IsPresent) {
    $cmakeArgs += "-DAYM_ENABLE_LLVM=ON"

    if (-not [string]::IsNullOrWhiteSpace($LLVMDir)) {
        $llvmDirResolved = (Resolve-Path -Path $LLVMDir -ErrorAction Stop).Path
        $llvmConfigPath  = Join-Path $llvmDirResolved "LLVMConfig.cmake"

        if (-not (Test-Path $llvmConfigPath)) {
            throw "LLVMDir no parece válido. No se encontró LLVMConfig.cmake en: $llvmDirResolved"
        }

        $cmakeArgs += "-DLLVM_DIR=$llvmDirResolved"
        Write-Host "LLVM backend habilitado (AYM_ENABLE_LLVM=ON)"
        Write-Host "Usando LLVM_DIR: $llvmDirResolved"
    } else {
        Write-Host "LLVM backend habilitado, pero LLVMDir no fue proporcionado."
    }
} else {
    $cmakeArgs += "-DAYM_ENABLE_LLVM=OFF"
    Write-Host "LLVM backend deshabilitado (AYM_ENABLE_LLVM=OFF)"
}

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
