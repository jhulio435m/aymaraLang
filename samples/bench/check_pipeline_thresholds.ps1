param(
    [string]$SummaryPath = "build/bench/pipeline/summary.json",
    [string]$ThresholdsPath = "docs/benchmarks/pipeline_thresholds.json",
    [string]$Target = ""
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

if (-not (Test-Path $SummaryPath)) {
    throw "No se encontro summary JSON: $SummaryPath"
}
if (-not (Test-Path $ThresholdsPath)) {
    throw "No se encontro archivo de umbrales: $ThresholdsPath"
}

$summary = Get-Content -Raw -Path $SummaryPath | ConvertFrom-Json
$thresholds = Get-Content -Raw -Path $ThresholdsPath | ConvertFrom-Json

$resolvedTarget = $Target
if ([string]::IsNullOrWhiteSpace($resolvedTarget)) {
    if ($env:RUNNER_OS -eq "Windows") {
        $resolvedTarget = "windows-latest"
    } elseif ($env:RUNNER_OS -eq "Linux") {
        $resolvedTarget = "ubuntu-latest"
    } else {
        throw "No se pudo inferir target desde RUNNER_OS. Pasa -Target explicitamente."
    }
}

if (-not $thresholds.targets.PSObject.Properties.Name.Contains($resolvedTarget)) {
    throw "Target sin umbrales definidos: $resolvedTarget"
}

$mode = [string]$summary.mode
$targetThresholds = $thresholds.targets.$resolvedTarget
if (-not $targetThresholds.PSObject.Properties.Name.Contains($mode)) {
    throw "Modo '$mode' sin umbrales para target '$resolvedTarget'"
}

$modeThresholds = $targetThresholds.$mode
$avg = $summary.averages_ms

$checks = @(
    @{ Name = "assemble"; Value = [double]$avg.assemble; Max = [double]$modeThresholds.assemble_max_ms },
    @{ Name = "runtime_compile"; Value = [double]$avg.runtime_compile; Max = [double]$modeThresholds.runtime_compile_max_ms },
    @{ Name = "link"; Value = [double]$avg.link; Max = [double]$modeThresholds.link_max_ms },
    @{ Name = "total"; Value = [double]$avg.total; Max = [double]$modeThresholds.total_max_ms }
)

$violations = @()
foreach ($check in $checks) {
    if ($check.Value -gt $check.Max) {
        $violations += "{0}: {1}ms > max {2}ms" -f $check.Name, $check.Value, $check.Max
    }
}

Write-Host "[bench-thresholds] target=$resolvedTarget mode=$mode"
foreach ($check in $checks) {
    Write-Host ("[bench-thresholds] {0}={1}ms (max {2}ms)" -f $check.Name, $check.Value, $check.Max)
}

if ($violations.Count -gt 0) {
    throw ("Regresion de benchmark detectada (timing): " + ($violations -join "; "))
}

$healthViolations = @()
if ($modeThresholds.PSObject.Properties.Name.Contains("pipeline_health")) {
    if (-not $summary.PSObject.Properties.Name.Contains("pipeline_health_totals")) {
        throw "Summary sin pipeline_health_totals, pero thresholds requieren pipeline_health."
    }
    $health = $summary.pipeline_health_totals
    $healthThresholds = $modeThresholds.pipeline_health
    $healthChecks = @()

    if ($healthThresholds.PSObject.Properties.Name.Contains("failed_runs_max")) {
        $healthChecks += @{
            Name = "failed_runs"
            Value = [double](Read-Int64OrZero $summary.failed_runs)
            Max = [double](Read-Int64OrZero $healthThresholds.failed_runs_max)
        }
    }
    if ($healthThresholds.PSObject.Properties.Name.Contains("failed_max")) {
        $healthChecks += @{
            Name = "failed"
            Value = [double](Read-Int64OrZero $health.failed)
            Max = [double](Read-Int64OrZero $healthThresholds.failed_max)
        }
    }
    if ($healthThresholds.PSObject.Properties.Name.Contains("timed_out_max")) {
        $healthChecks += @{
            Name = "timed_out"
            Value = [double](Read-Int64OrZero $health.timed_out)
            Max = [double](Read-Int64OrZero $healthThresholds.timed_out_max)
        }
    }
    if ($healthThresholds.PSObject.Properties.Name.Contains("reason_nonzero_exit_max")) {
        $healthChecks += @{
            Name = "reason_nonzero_exit"
            Value = [double](Read-Int64OrZero $health.reason_nonzero_exit)
            Max = [double](Read-Int64OrZero $healthThresholds.reason_nonzero_exit_max)
        }
    }
    if ($healthThresholds.PSObject.Properties.Name.Contains("reason_timeout_max")) {
        $healthChecks += @{
            Name = "reason_timeout"
            Value = [double](Read-Int64OrZero $health.reason_timeout)
            Max = [double](Read-Int64OrZero $healthThresholds.reason_timeout_max)
        }
    }
    if ($healthThresholds.PSObject.Properties.Name.Contains("reason_missing_input_max")) {
        $healthChecks += @{
            Name = "reason_missing_input"
            Value = [double](Read-Int64OrZero $health.reason_missing_input)
            Max = [double](Read-Int64OrZero $healthThresholds.reason_missing_input_max)
        }
    }
    if ($healthThresholds.PSObject.Properties.Name.Contains("reason_internal_error_max")) {
        $healthChecks += @{
            Name = "reason_internal_error"
            Value = [double](Read-Int64OrZero $health.reason_internal_error)
            Max = [double](Read-Int64OrZero $healthThresholds.reason_internal_error_max)
        }
    }
    if ($healthThresholds.PSObject.Properties.Name.Contains("reason_other_max")) {
        $healthChecks += @{
            Name = "reason_other"
            Value = [double](Read-Int64OrZero $health.reason_other)
            Max = [double](Read-Int64OrZero $healthThresholds.reason_other_max)
        }
    }

    foreach ($check in $healthChecks) {
        Write-Host ("[bench-thresholds] health.{0}={1} (max {2})" -f $check.Name, $check.Value, $check.Max)
        if ($check.Value -gt $check.Max) {
            $healthViolations += "{0}: {1} > max {2}" -f $check.Name, $check.Value, $check.Max
        }
    }
}

if ($healthViolations.Count -gt 0) {
    throw ("Regresion de benchmark detectada (health): " + ($healthViolations -join "; "))
}

Write-Host "[bench-thresholds] OK"
