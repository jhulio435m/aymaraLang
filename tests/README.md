# Tests

Este directorio contiene pruebas automatizadas para el compilador `aymc`.
Para ejecutarlas solo es necesario ejecutar:

```bash
$ make test
```

Tambien puedes ejecutar pruebas unitarias via CMake/CTest:

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Para instrumentar cobertura en Linux (GCC/Clang + gcovr):

```bash
cmake -S . -B build-coverage -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DAYM_ENABLE_UNIT_TESTS=ON -DAYM_ENABLE_COVERAGE=ON
cmake --build build-coverage -j
cmake --build build-coverage --target coverage
```

El reporte se genera en `build-coverage/coverage/coverage.xml` y
`build-coverage/coverage/coverage.html`.

La suite CTest incluye:

- `aym_unit_tests`: pruebas unitarias C++ (GoogleTest).
- `aym_wrapper_smoke`: smoke end-to-end del wrapper (`new/add/build/test`) y
  validación de checksums en `aym.lock`, incluyendo control de modo `--frozen`
  en `add` (solo lectura), `build`, `lock sync` y `cache sync`.
- `aym_diagnostics_smoke`: smoke de `--diagnostics-json` validando exportación
  de `AYM4006` (inconsistencia `aym.toml`/`aym.lock` en import por paquete).
- `aym_lock_contract_smoke`: contrato CLI de `aym lock` y `--frozen` (mensajes
  esperados en casos OK/error reproducibles).
- `aym_pipeline_modes_smoke`: contrato del pipeline nativo por etapas
  (`--compile-only`/`--link-only`) y validación de métricas (`--time-pipeline`
  y `--time-pipeline-json`, incluyendo reporte estructurado en casos de fallo
  (`result.success=false` + `failed_stage`), campo de configuración
  `tool_timeout_ms`, trazas por comando en `commands` (incluye `stdout`/`stderr`)
  con `exit_reason` normalizado y resumen agregado `phase_summary`.
- `aym_backend_ir_smoke`: contrato del backend `ir` experimental (emisión de
  `.ir`, presencia de `tool_timeout_ms`, bloque `commands` (con `ir-gen`) en
  pipeline JSON, `phase_summary`, `exit_reason` y rechazo explícito de `--link-only`
  en esta fase).

En Windows, `make test` ejecuta `tests/test_modulos_auto.ps1` (si `pwsh` no
esta disponible, intenta con `powershell`).

Para benchmark de pipeline (fuera de CTest), usa:

```powershell
pwsh -File samples/bench/run_pipeline_bench.ps1 -Iterations 5
pwsh -File samples/bench/check_pipeline_thresholds.ps1 -SummaryPath build/bench/pipeline/full/summary.json -Target windows-latest
pwsh -File samples/bench/run_pipeline_bench.ps1 -Iterations 5 -CompileOnly
pwsh -File samples/bench/check_pipeline_thresholds.ps1 -SummaryPath build/bench/pipeline/compile-only/summary.json -Target windows-latest
```

`check_pipeline_thresholds.ps1` valida umbrales de tiempo y salud del pipeline
(`failed_runs`, `reason_timeout`, `reason_nonzero_exit`, etc.) definidos en
`docs/benchmarks/pipeline_thresholds.json`.

El script `run_tests.sh` compila el compilador, genera los ejemplos principales
en `samples/` y comprueba la salida exacta de `samples/aymara_flow.aym`. Además,
valida la ejecución de los ejemplos `samples/fundamentos/basicos.aym`
y `samples/fundamentos/funciones_listas.aym` con entradas de prueba.

Además, ejecuta una prueba rápida de empaquetado con CPack si `cmake` y `cpack`
están disponibles en el entorno.

Para validación end-to-end del instalador Linux `.deb` (build Linux limpio,
instalación, compilación/ejecución de samples y purge final), ejecuta:

```bash
bash scripts/test/test_deb_e2e.sh
```

Para validación completa de ejemplos en Windows (compilación recursiva en
`samples` sin `bench`, ejecución de casos no interactivos y chequeos de salida),
ejecuta:

```powershell
pwsh -File tests/test_modulos_auto.ps1
```

Este script también valida que los ejemplos interactivos (`tetris`) en
`samples/games/console` y su versión GUI en `samples/games/gui` compilan
correctamente.

