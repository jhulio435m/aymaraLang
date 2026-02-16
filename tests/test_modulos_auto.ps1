$ErrorActionPreference = "Stop"
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -ErrorAction SilentlyContinue) {
  $PSNativeCommandUseErrorActionPreference = $false
}
$IsWindowsHost = $false
if (Get-Variable -Name IsWindows -ErrorAction SilentlyContinue) {
  $IsWindowsHost = [bool]$IsWindows
} else {
  $IsWindowsHost = ($env:OS -eq "Windows_NT")
}

$repo = Split-Path -Parent $PSScriptRoot
Set-Location $repo

$compilerCandidates = @(
  (Join-Path $repo "build\bin\Release\aymc.exe"),
  (Join-Path $repo "build\bin\aymc.exe"),
  (Join-Path $repo "bin\aymc.exe"),
  (Join-Path $repo "build\bin\Release\aymc"),
  (Join-Path $repo "build\bin\aymc"),
  (Join-Path $repo "bin\aymc")
)
$compiler = $null
foreach ($candidate in $compilerCandidates) {
  if (Test-Path $candidate) {
    $compiler = $candidate
    break
  }
}
if (-not $compiler) {
  throw "No se encontro el compilador. Rutas buscadas:`n$($compilerCandidates -join "`n")`nPrimero compila el proyecto."
}

function Assert-Contains {
  param(
    [string]$OutputText,
    [string[]]$Expected,
    [string]$Context
  )
  foreach ($needle in $Expected) {
    if (-not $OutputText.Contains($needle)) {
      throw "Salida invalida en '$Context'. Falta: '$needle'"
    }
  }
}

function Invoke-NativeCommand {
  param(
    [string]$FilePath,
    [string[]]$Arguments = @()
  )
  if ($IsWindowsHost) {
    $quotedPath = '"' + $FilePath + '"'
    $quotedArgs = $Arguments | ForEach-Object { '"' + ($_ -replace '"', '\"') + '"' }
    $cmdLine = (@($quotedPath) + $quotedArgs) -join " "
    $outputText = (& cmd /d /c "$cmdLine 2>&1" | Out-String)
    return [pscustomobject]@{
      Output = $outputText
      ExitCode = $LASTEXITCODE
    }
  }

  $outputText = (& $FilePath @Arguments 2>&1 | Out-String)
  return [pscustomobject]@{
    Output = $outputText
    ExitCode = $LASTEXITCODE
  }
}

function Compile-Example {
  param(
    [string]$SourcePath,
    [bool]$ShouldFail = $false
  )
  if (-not $ShouldFail) {
    $outputPath = Get-OutputPath -SourcePath $SourcePath
    Stop-LockingOutputProcess -OutputPath $outputPath
  }
  $result = Invoke-NativeCommand -FilePath $compiler -Arguments @($SourcePath)
  $compileOut = $result.Output
  if ($ShouldFail) {
    if ($result.ExitCode -eq 0) {
      throw "Debia fallar y compilo: $SourcePath"
    }
    if ([string]::IsNullOrWhiteSpace($compileOut)) {
      Write-Output "[WARN] Fallo sin mensaje visible: $SourcePath"
    }
    return
  }
  if ($result.ExitCode -ne 0) {
    throw "Fallo compilando: $SourcePath`n$compileOut"
  }
}

function Get-OutputPath {
  param([string]$SourcePath)
  $base = [System.IO.Path]::ChangeExtension($SourcePath, $null)
  if ($IsWindowsHost) {
    return "$base.exe"
  }
  return $base
}

function Stop-LockingOutputProcess {
  param([string]$OutputPath)
  if (-not $IsWindowsHost) {
    return
  }
  if (-not (Test-Path $OutputPath)) {
    return
  }
  $exeName = [System.IO.Path]::GetFileNameWithoutExtension($OutputPath)
  if ([string]::IsNullOrWhiteSpace($exeName)) {
    return
  }
  $normalizedOutput = [System.IO.Path]::GetFullPath($OutputPath)
  $candidates = Get-Process -Name $exeName -ErrorAction SilentlyContinue
  foreach ($proc in $candidates) {
    $procPath = $null
    try {
      $procPath = $proc.Path
    } catch {
      $procPath = $null
    }
    if ([string]::IsNullOrWhiteSpace($procPath)) {
      continue
    }
    if ([System.IO.Path]::GetFullPath($procPath) -ieq $normalizedOutput) {
      Write-Output "[WARN] Cerrando proceso bloqueando '$OutputPath' (PID=$($proc.Id))"
      Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    }
  }
}

function Should-Compile {
  param(
    [string]$SourcePath,
    [string]$CompilerPath,
    [string[]]$Dependencies
  )
  $outputPath = Get-OutputPath -SourcePath $SourcePath
  if (-not (Test-Path $outputPath)) {
    return $true
  }
  $outputTime = (Get-Item $outputPath).LastWriteTimeUtc
  if ((Get-Item $SourcePath).LastWriteTimeUtc -gt $outputTime) {
    return $true
  }
  if ((Get-Item $CompilerPath).LastWriteTimeUtc -gt $outputTime) {
    return $true
  }
  foreach ($dep in $Dependencies) {
    if ((Test-Path $dep) -and ((Get-Item $dep).LastWriteTimeUtc -gt $outputTime)) {
      return $true
    }
  }
  return $false
}

function Run-Example {
  param(
    [string]$ExePath,
    [string[]]$ProgramArgs = @(),
    [string[]]$Expected = @(),
    [string]$Name = ""
  )
  $out = & $ExePath @ProgramArgs 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Fallo ejecutando '$ExePath' (caso: $Name)`n$out"
  }
  if ($Expected.Count -gt 0) {
    Assert-Contains -OutputText $out -Expected $Expected -Context $Name
  }
}

Write-Output "[TEST] Compilando todos los .aym en samples/ (sin bench)"
$expectedCompileFail = @(
  "modulos_imports_error.aym",
  "modulos_alias_error.aym",
  "poo_error_privado.aym",
  "poo_error_override.aym",
  "poo_error_ctor.aym",
  "poo_error_tipos.aym"
)
$compileDeps = @(
  (Join-Path $repo "runtime\runtime.c"),
  (Join-Path $repo "runtime\math.c")
)
$compiledCount = 0
$skippedCount = 0
$sampleRoots = @(
  "samples\fundamentos",
  "samples\colecciones",
  "samples\control_flujo",
  "samples\poo",
  "samples\modulos",
  "samples\apps",
  "samples\games"
)
$allExamples = @()
foreach ($root in $sampleRoots) {
  $path = Join-Path $repo $root
  if (Test-Path $path) {
    $allExamples += Get-ChildItem -Path $path -Recurse -Filter *.aym
  }
}
$allExamples = $allExamples | Sort-Object FullName
foreach ($file in $allExamples) {
  $mustFail = $expectedCompileFail -contains $file.Name
  if (-not $mustFail) {
    if (-not (Should-Compile -SourcePath $file.FullName -CompilerPath $compiler -Dependencies $compileDeps)) {
      $skippedCount += 1
      continue
    }
  }
  Compile-Example -SourcePath $file.FullName -ShouldFail:$mustFail
  $compiledCount += 1
}
Write-Output "[TEST] Compilacion incremental: compilados=$compiledCount omitidos=$skippedCount"

Write-Output "[TEST] Ejecutando ejemplos no interactivos con validaciones"
$arkataTmpPaths = @(
  (Join-Path $repo "samples\_tmp_arkata.txt"),
  (Join-Path $repo "samples\apps\_tmp_arkata.txt")
)
$inventarioTmpPaths = @(
  (Join-Path $repo "samples\_tmp_inventario.txt"),
  (Join-Path $repo "samples\apps\_tmp_inventario.txt")
)
foreach ($tmp in $arkataTmpPaths) {
  if (Test-Path $tmp) { Remove-Item $tmp -Force }
}
foreach ($tmp in $inventarioTmpPaths) {
  if (Test-Path $tmp) { Remove-Item $tmp -Force }
}

$runCases = @(
  @{
    Name = "basicos"
    Exe = (Join-Path $repo "samples\fundamentos\basicos.exe")
    Args = @()
    Expected = @("kamisaraki", "n = 5", "n = 123")
  },
  @{
    Name = "funciones_listas"
    Exe = (Join-Path $repo "samples\fundamentos\funciones_listas.exe")
    Args = @()
    Expected = @("Suti = Ana", "120", "[10, 99, 30, 123]")
  },
  @{
    Name = "stdlib_texto_listas"
    Exe = (Join-Path $repo "samples\colecciones\stdlib_texto_listas.exe")
    Args = @()
    Expected = @("VACIO", "INDICE", "CONVERSION")
  },
  @{
    Name = "mapas"
    Exe = (Join-Path $repo "samples\colecciones\mapas.exe")
    Args = @()
    Expected = @("{nombre: ""Ana"", ciudad: ""Huancayo""}", "x=10", "y=20")
  },
  @{
    Name = "poo"
    Exe = (Join-Path $repo "samples\poo\poo.exe")
    Args = @()
    Expected = @("hola Ana", "base+hijo", "hp=150")
  },
  @{
    Name = "poo_combinaciones"
    Exe = (Join-Path $repo "samples\poo\poo_combinaciones.exe")
    Args = @()
    Expected = @(
      "private interno = A1",
      "static total = 5",
      "override = base+sub",
      "ctor0 = 0,0",
      "ctor2 = 3,4",
      "composicion = hp=150",
      "polimorfismo = perro",
      "poo_combinaciones OK"
    )
  },
  @{
    Name = "foreach"
    Exe = (Join-Path $repo "samples\fundamentos\foreach.exe")
    Args = @()
    Expected = @("item 1", "item 4", "suma = 10")
  },
  @{
    Name = "colecciones_avanzadas"
    Exe = (Join-Path $repo "samples\colecciones\colecciones_avanzadas.exe")
    Args = @()
    Expected = @("wakicha(xs) = [1, 1, 2, 3, 5, 5]", "thaqha(xs_ordenado, 3) = 3", "colecciones avanzadas OK")
  },
  @{
    Name = "hof_aymara"
    Exe = (Join-Path $repo "samples\colecciones\hof_aymara.exe")
    Args = @()
    Expected = @("ys = [2, 4, 6, 8, 10]", "zs = [2, 4, 6, 8, 10]", "total = 30")
  },
  @{
    Name = "hof_map_filter_reduce"
    Exe = (Join-Path $repo "samples\colecciones\hof_map_filter_reduce.exe")
    Args = @()
    Expected = @("map = [2, 4, 6, 8, 10]", "filter = [2, 4, 6, 8, 10]", "reduce = 30")
  },
  @{
    Name = "assert_y_salida"
    Exe = (Join-Path $repo "samples\apps\assert_y_salida.exe")
    Args = @()
    Expected = @("capturada ASSERT", "ASSERT", "fallo esperado")
  },
  @{
    Name = "cli_args"
    Exe = (Join-Path $repo "samples\fundamentos\cli_args.exe")
    Args = @("uno", "paya")
    Expected = @("argc = 3", "arg 1 = uno", "arg 2 = paya")
  },
  @{
    Name = "arkata_stdlib"
    Exe = (Join-Path $repo "samples\apps\arkata_stdlib.exe")
    Args = @()
    Expected = @("qillqana = 1", "ullana = kamisaraki aymara", "arkata stdlib OK")
  },
  @{
    Name = "modulos_imports"
    Exe = (Join-Path $repo "samples\modulos\modulos_imports.exe")
    Args = @()
    Expected = @("a = 5", "b = 18", "c = 12", "modulos imports OK")
  },
  @{
    Name = "banca_excepciones"
    Exe = (Join-Path $repo "samples\apps\banca_excepciones.exe")
    Args = @()
    Expected = @("error capturado SALDO", "Ana:430", "Luis:320", "banca_excepciones OK")
  },
  @{
    Name = "n_reinas"
    Exe = (Join-Path $repo "samples\apps\n_reinas.exe")
    Args = @()
    Expected = @("n = 8", "soluciones = 92", "n_reinas OK")
  },
  @{
    Name = "inventario_archivo"
    Exe = (Join-Path $repo "samples\apps\inventario_archivo.exe")
    Args = @()
    Expected = @("canihua=3", "papa=17", "inventario_archivo OK")
  },
  @{
    Name = "enum_match"
    Exe = (Join-Path $repo "samples\control_flujo\enum_match.exe")
    Args = @()
    Expected = @("menu", "jugando", "default", "enum_match OK")
  },
  @{
    Name = "match_aru"
    Exe = (Join-Path $repo "samples\control_flujo\match_aru.exe")
    Args = @()
    Expected = @("match_aru = dos", "match_aru OK")
  },
  @{
    Name = "match_multi_case"
    Exe = (Join-Path $repo "samples\control_flujo\match_multi_case.exe")
    Args = @()
    Expected = @("p8 = par", "p5 = impar", "r1 = aceptado", "r2 = rechazado", "match_multi_case OK")
  },
  @{
    Name = "match_rango"
    Exe = (Join-Path $repo "samples\control_flujo\match_rango.exe")
    Args = @()
    Expected = @("nivel45 = jan wali", "nivel130 =", "a2 = ajllita", "match_rango OK")
  },
  @{
    Name = "sudoku_4x4"
    Exe = (Join-Path $repo "samples\games\console\sudoku_4x4.exe")
    Args = @()
    Expected = @("fila1 1 2 3 4", "fila4 4 3 2 1", "sudoku_4x4 OK")
  },
  @{
    Name = "regresion_llamadas"
    Exe = (Join-Path $repo "samples\apps\regresion_llamadas.exe")
    Args = @()
    Expected = @("r = 122334", "c32 = 9", "c22 = 0", "nested = 9", "regresion_llamadas OK")
  }
)

foreach ($case in $runCases) {
  Run-Example -ExePath $case.Exe -ProgramArgs $case.Args -Expected $case.Expected -Name $case.Name
}

Write-Output "[TEST] Juegos interactivos (compilacion valida, ejecucion manual interactiva)"
if (-not (Test-Path (Join-Path $repo "samples\games\console\tetris.exe"))) {
  throw "No se genero tetris.exe"
}
if (-not (Test-Path (Join-Path $repo "samples\games\gui\tetris.exe"))) {
  throw "No se genero tetris.exe"
}
if (-not (Test-Path (Join-Path $repo "samples\games\gui\sudoku_uja.exe"))) {
  throw "No se genero sudoku_uja.exe"
}
if (-not (Test-Path (Join-Path $repo "samples\games\gui\jamachi_uja.exe"))) {
  throw "No se genero jamachi_uja.exe"
}
if (-not (Test-Path (Join-Path $repo "samples\games\gui\katari_uja.exe"))) {
  throw "No se genero katari_uja.exe"
}

foreach ($tmp in $arkataTmpPaths) {
  if (Test-Path $tmp) { Remove-Item $tmp -Force }
}
foreach ($tmp in $inventarioTmpPaths) {
  if (Test-Path $tmp) { Remove-Item $tmp -Force }
}

Write-Output "[OK] test_modulos_auto completado"


