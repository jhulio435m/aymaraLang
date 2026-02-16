# Benchmarks de pipeline

Esta carpeta contiene un benchmark reproducible para medir tiempos del backend
de `aymc` usando el export JSON de `--time-pipeline-json`.

## Archivos

- `pipeline_bench.aym`: fuente de referencia para medir compilacion.
- `run_pipeline_bench.ps1`: runner de iteraciones con resumen agregado.
- `check_pipeline_thresholds.ps1`: valida umbrales de regresion sobre
  `summary.json`.

## Ejecucion

Desde la raiz del repositorio:

```powershell
pwsh -File samples/bench/run_pipeline_bench.ps1 -Iterations 5
```

Opcionalmente, modo solo compilacion (sin enlace):

```powershell
pwsh -File samples/bench/run_pipeline_bench.ps1 -CompileOnly -Iterations 5
```

## Salida

Por defecto se escribe en `build/bench/pipeline/`:

- `run_XX.pipeline.json`: metricas por corrida.
- `summary.json`: promedio y detalle de iteraciones.

El `summary.json` usa el esquema `aymc.pipeline.benchmark.v1`.

## Umbrales

Los umbrales versionados viven en `docs/benchmarks/pipeline_thresholds.json`.

Validacion local:

```powershell
pwsh -File samples/bench/check_pipeline_thresholds.ps1 `
  -SummaryPath build/bench/pipeline/summary.json `
  -ThresholdsPath docs/benchmarks/pipeline_thresholds.json `
  -Target windows-latest
```
