param(
    [string]$Compiler,
    [string]$Source = "samples/bench/pipeline_bench.aym",
    [string]$OutputDir = "build/bench/pipeline",
    [int]$Iterations = 5,
    [switch]$CompileOnly,
    [switch]$KeepArtifacts
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Read-Int64OrZero {
    param($Value)
    if ($null -eq $Value) {
        return [int64]0
    }
    try {
        return [int64]$Value
    } catch {
        return [int64]0
    }
}

function Resolve-RepoRoot {
    param([string]$ScriptRoot)
    return (Resolve-Path (Join-Path $ScriptRoot "..\..")).Path
}

function Resolve-CompilerPath {
    param(
        [string]$RepoRoot,
        [string]$Candidate
    )
    if (-not [string]::IsNullOrWhiteSpace($Candidate)) {
        if (-not [System.IO.Path]::IsPathRooted($Candidate)) {
            $Candidate = Join-Path $RepoRoot $Candidate
        }
        if (Test-Path $Candidate) {
            return (Resolve-Path $Candidate).Path
        }
        throw "No se encontro compilador en ruta indicada: $Candidate"
    }

    $fallbacks = @(
        "build/bin/Release/aymc.exe",
        "build/bin/aymc.exe",
        "build/bin/Release/aymc",
        "build/bin/aymc"
    )
    foreach ($entry in $fallbacks) {
        $path = Join-Path $RepoRoot $entry
        if (Test-Path $path) {
            return (Resolve-Path $path).Path
        }
    }
    throw "No se pudo localizar aymc. Compila primero el proyecto (ej. cmake --build build --config Release)."
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$PathValue
    )
    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return $PathValue
    }
    return (Join-Path $RepoRoot $PathValue)
}

$repoRoot = Resolve-RepoRoot -ScriptRoot $PSScriptRoot
$compilerPath = Resolve-CompilerPath -RepoRoot $repoRoot -Candidate $Compiler
$sourcePath = Resolve-RepoPath -RepoRoot $repoRoot -PathValue $Source
$outputRoot = Resolve-RepoPath -RepoRoot $repoRoot -PathValue $OutputDir

if (-not (Test-Path $sourcePath)) {
    throw "No se encontro archivo fuente benchmark: $sourcePath"
}
if ($Iterations -lt 1) {
    throw "Iterations debe ser >= 1"
}

$mode = if ($CompileOnly) { "compile-only" } else { "full" }
$modeOutputRoot = Join-Path $outputRoot $mode

New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null
New-Item -ItemType Directory -Force -Path $modeOutputRoot | Out-Null

$runs = @()
$healthTotals = [ordered]@{
    commands_total = [int64]0
    ok = [int64]0
    failed = [int64]0
    cache_hit = [int64]0
    timed_out = [int64]0
    reason_ok = [int64]0
    reason_nonzero_exit = [int64]0
    reason_timeout = [int64]0
    reason_missing_input = [int64]0
    reason_cache_hit = [int64]0
    reason_internal_error = [int64]0
    reason_other = [int64]0
}
$failedRuns = [int64]0

for ($i = 1; $i -le $Iterations; $i++) {
    $runBase = Join-Path $modeOutputRoot ("run_{0:D2}" -f $i)
    $jsonPath = "$runBase.pipeline.json"

    $args = @("--time-pipeline-json=$jsonPath", "-o", $runBase, $sourcePath)
    if ($CompileOnly) {
        $args = @("--compile-only", "--emit-asm") + $args
    }

    Write-Host "[bench] run $i/$Iterations -> $mode"
    & $compilerPath @args
    if ($LASTEXITCODE -ne 0) {
        throw "Fallo compilacion benchmark en iteracion $i (exit $LASTEXITCODE)"
    }
    if (-not (Test-Path $jsonPath)) {
        throw "No se genero metrics JSON en iteracion ${i}: $jsonPath"
    }

    $jsonData = Get-Content -Raw -Path $jsonPath | ConvertFrom-Json
    $runResult = $jsonData.result
    $runSuccess = $true
    $runFailedStage = ""
    $runExitCode = [int64]0
    if ($null -ne $runResult) {
        if ($null -ne $runResult.success) {
            $runSuccess = [bool]$runResult.success
        }
        if ($null -ne $runResult.failed_stage) {
            $runFailedStage = [string]$runResult.failed_stage
        }
        if ($null -ne $runResult.exit_code) {
            $runExitCode = Read-Int64OrZero $runResult.exit_code
        }
    }
    if (-not $runSuccess) {
        $failedRuns += 1
    }

    $phaseSummary = $jsonData.phase_summary
    $runPhaseSummary = [pscustomobject]@{
        commands_total = Read-Int64OrZero $phaseSummary.commands_total
        ok = Read-Int64OrZero $phaseSummary.ok
        failed = Read-Int64OrZero $phaseSummary.failed
        cache_hit = Read-Int64OrZero $phaseSummary.cache_hit
        timed_out = Read-Int64OrZero $phaseSummary.timed_out
        reason_ok = Read-Int64OrZero $phaseSummary.reason_ok
        reason_nonzero_exit = Read-Int64OrZero $phaseSummary.reason_nonzero_exit
        reason_timeout = Read-Int64OrZero $phaseSummary.reason_timeout
        reason_missing_input = Read-Int64OrZero $phaseSummary.reason_missing_input
        reason_cache_hit = Read-Int64OrZero $phaseSummary.reason_cache_hit
        reason_internal_error = Read-Int64OrZero $phaseSummary.reason_internal_error
        reason_other = Read-Int64OrZero $phaseSummary.reason_other
    }
    foreach ($name in @($healthTotals.Keys)) {
        $healthTotals[$name] += Read-Int64OrZero $runPhaseSummary.$name
    }

    $runs += [pscustomobject]@{
        iteration = $i
        mode = $jsonData.mode
        assemble_ms = [int64]$jsonData.timing_ms.assemble
        runtime_compile_ms = [int64]$jsonData.timing_ms.runtime_compile
        link_ms = [int64]$jsonData.timing_ms.link
        total_ms = [int64]$jsonData.timing_ms.total
        result_success = $runSuccess
        result_failed_stage = $runFailedStage
        result_exit_code = $runExitCode
        phase_summary = $runPhaseSummary
        json = $jsonPath
    }

    if (-not $KeepArtifacts) {
        Remove-Item -Force -ErrorAction SilentlyContinue "$runBase.asm", "$runBase.o", "$runBase.obj", "$runBase.exe", "$runBase"
    }
}

$avgAssemble = [math]::Round((($runs | Measure-Object -Property assemble_ms -Average).Average), 2)
$avgRuntime = [math]::Round((($runs | Measure-Object -Property runtime_compile_ms -Average).Average), 2)
$avgLink = [math]::Round((($runs | Measure-Object -Property link_ms -Average).Average), 2)
$avgTotal = [math]::Round((($runs | Measure-Object -Property total_ms -Average).Average), 2)

$summary = [pscustomobject]@{
    schema = "aymc.pipeline.benchmark.v2"
    generated_at = (Get-Date).ToString("yyyy-MM-ddTHH:mm:ssK")
    compiler = $compilerPath
    source = $sourcePath
    mode = $mode
    iterations = $Iterations
    averages_ms = [pscustomobject]@{
        assemble = $avgAssemble
        runtime_compile = $avgRuntime
        link = $avgLink
        total = $avgTotal
    }
    pipeline_health_totals = [pscustomobject]$healthTotals
    failed_runs = $failedRuns
    runs = $runs
}

$summaryPath = Join-Path $modeOutputRoot "summary.json"
$legacySummaryPath = Join-Path $outputRoot "summary.json"
$summaryJson = $summary | ConvertTo-Json -Depth 6
$summaryJson | Set-Content -Path $summaryPath -Encoding UTF8
$summaryJson | Set-Content -Path $legacySummaryPath -Encoding UTF8

Write-Host "[bench] summary: $summaryPath"
Write-Host "[bench] summary (legacy): $legacySummaryPath"
Write-Host ("[bench] avg(ms) assemble={0} runtime={1} link={2} total={3}" -f $avgAssemble, $avgRuntime, $avgLink, $avgTotal)
