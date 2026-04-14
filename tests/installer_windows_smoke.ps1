[CmdletBinding()]
param(
  [string]$BuildDir = "build",
  [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
  [string]$Config = "Release",
  [string]$DistDir = "build\\tmp\\installer_dist_smoke",
  [string]$OutputDir = "artifacts\\installer-smoke",
  [switch]$UseExistingDist
)

$ErrorActionPreference = "Stop"

function Assert-ExitOk {
  param([string]$Step)
  if ($LASTEXITCODE -ne 0) {
    throw "$Step fallo con codigo de salida $LASTEXITCODE."
  }
}

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildPath = if ([System.IO.Path]::IsPathRooted($BuildDir)) { $BuildDir } else { Join-Path $root $BuildDir }
$distPath = Join-Path $root $DistDir
$outputPath = Join-Path $root $OutputDir
$bundleToolchainScript = Join-Path $root "scripts\\build\\bundle_windows_toolchain.ps1"
$compilerCandidates = @(
  (Join-Path $buildPath "bin\\$Config\\aymc.exe"),
  (Join-Path $buildPath "bin\\aymc.exe")
)
$builtCompiler = $null
foreach ($candidate in $compilerCandidates) {
  if (Test-Path $candidate) {
    $builtCompiler = $candidate
    break
  }
}

if (-not $UseExistingDist -and -not $builtCompiler) {
  throw "No se encontro aymc compilado. Rutas buscadas:`n$($compilerCandidates -join '`n')"
}

if (-not $UseExistingDist -and (Test-Path $distPath)) {
  Remove-Item -Path $distPath -Recurse -Force
}

if (Test-Path $outputPath) {
  Remove-Item -Path $outputPath -Recurse -Force
}

if ($UseExistingDist) {
  Write-Output "[test] reutilizando dist existente para validacion de instaladores Windows"
} else {
  Write-Output "[test] preparando dist para instaladores Windows"
  & cmake --install $buildPath --config $Config --prefix $distPath
  Assert-ExitOk "cmake --install"

  Write-Output "[test] embebiendo toolchain portable en dist"
  & $bundleToolchainScript -DistDir $DistDir
  Assert-ExitOk "bundle_windows_toolchain.ps1"
}

if (-not (Test-Path (Join-Path $distPath "bin\\aymc.exe")) -or -not (Test-Path (Join-Path $distPath "bin\\aym.exe"))) {
  throw "El directorio dist no contiene binarios validos: $distPath"
}
foreach ($requiredDll in @("libstdc++-6.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll")) {
  if (-not (Test-Path (Join-Path $distPath "bin\\$requiredDll"))) {
    throw "El directorio dist no contiene la DLL requerida junto al compilador: $requiredDll"
  }
}
if (-not (Test-Path (Join-Path $distPath "toolchain\\bin\\nasm.exe")) -or -not (Test-Path (Join-Path $distPath "toolchain\\mingw64\\bin\\gcc.exe"))) {
  throw "El directorio dist no contiene toolchain embebida completa: $distPath"
}

Write-Output "[test] validando compilacion con toolchain embebida"
$bundledCompiler = Join-Path $distPath "bin\\aymc.exe"
$samplePath = Join-Path $root "samples\\fundamentos\\basicos.aym"
$smokeDir = Join-Path $root "build\\tmp\\bundled_toolchain_smoke"
$smokeOutputBase = Join-Path $smokeDir "basicos"
$smokeExe = "$smokeOutputBase.exe"
$pipelineJson = Join-Path $smokeDir "pipeline.json"
if (Test-Path $smokeDir) {
  Remove-Item -Path $smokeDir -Recurse -Force
}
New-Item -ItemType Directory -Path $smokeDir -Force | Out-Null

$originalPath = $env:Path
try {
  & $bundledCompiler $samplePath -o $smokeOutputBase "--time-pipeline-json=$pipelineJson"
  Assert-ExitOk "aymc bundled toolchain smoke compile"

  if (-not (Test-Path $smokeExe)) {
    throw "La compilacion con toolchain embebida no produjo ejecutable: $smokeExe"
  }
  if (-not (Test-Path $pipelineJson)) {
    throw "No se genero pipeline JSON para validar la toolchain embebida."
  }

  $runOutput = (& $smokeExe 2>&1 | Out-String)
  if ($LASTEXITCODE -ne 0) {
    throw "El ejecutable compilado con toolchain embebida fallo.`n$runOutput"
  }
  if (-not $runOutput.Contains("kamisaraki")) {
    throw "La salida del ejecutable compilado con toolchain embebida no contiene el texto esperado."
  }

  $pipeline = Get-Content $pipelineJson -Raw | ConvertFrom-Json
  $expectedNasm = Join-Path (Join-Path (Join-Path $distPath "toolchain") "bin") "nasm.exe"
  $expectedGcc = Join-Path (Join-Path (Join-Path (Join-Path $distPath "toolchain") "mingw64") "bin") "gcc.exe"
  $commands = @($pipeline.commands | ForEach-Object { $_.command })
  if (-not ($commands | Where-Object { $_ -like "$expectedNasm *" })) {
    throw "El pipeline no usó nasm embebido: $expectedNasm"
  }
  if (-not ($commands | Where-Object { $_ -like "$expectedGcc *" })) {
    throw "El pipeline no usó gcc embebido: $expectedGcc"
  }
} finally {
  $env:Path = $originalPath
}

Write-Output "[test] construyendo instalador NSIS"
& (Join-Path $root "scripts\\build\\build_nsis.ps1") -DistDir $DistDir -OutputDir $OutputDir
Assert-ExitOk "build_nsis.ps1"

Write-Output "[test] construyendo instalador MSI"
& (Join-Path $root "scripts\\build\\build_msi.ps1") -DistDir $DistDir -OutputDir $OutputDir
Assert-ExitOk "build_msi.ps1"

$expectedArtifacts = @(
  (Join-Path $outputPath "AymaraLang-Setup.exe"),
  (Join-Path $outputPath "AymaraLang-Setup.msi")
)

foreach ($artifact in $expectedArtifacts) {
  if (-not (Test-Path $artifact)) {
    throw "No se genero el artefacto esperado: $artifact"
  }

  $length = (Get-Item $artifact).Length
  if ($length -le 0) {
    throw "El artefacto esta vacio: $artifact"
  }
}

Write-Output "[test] windows installer smoke test passed"
