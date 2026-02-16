# CLI del compilador (`aymc`)

## Uso

```bash
aymc [opciones] archivo.aym ...
```

Si se entregan varios archivos, se compilan como una unidad.

## Opciones

- `-h`, `--help`: muestra ayuda.
- `-o <ruta>`: define salida del ejecutable.
- `--backend <nombre>`: selecciona backend (`native` o `ir`).
- `--debug`: imprime tokens.
- `--dump-ast`: imprime resumen de nodos AST.
- `--check`: valida sintaxis/semántica sin generar binario.
- `--emit-asm`: conserva ASM intermedio.
- `--compile-only`: genera ASM/objeto sin enlazar.
- `--link-only`: enlaza objeto existente (requiere `-o`).
- `--time-pipeline`: imprime tiempos por etapa.
- `--time-pipeline-json[=ruta]`: exporta métricas de pipeline a JSON.
- `--tool-timeout-ms <ms>`: timeout para comandos externos (`0` sin límite).
- `--emit-ast-json[=ruta]`: exporta AST en JSON.
- `--diagnostics-json[=ruta]`: exporta diagnósticos en JSON.
- `--check-manifest[=ruta]`: valida `aym.toml`.
- `--manifest <ruta>`: ruta explícita de manifest.
- `--emit-lock[=ruta]`: genera `aym.lock`.
- `--check-lock[=ruta]`: valida `aym.lock`.
- `--lock <ruta>`: ruta explícita de lockfile.
- `--windows`: fuerza objetivo Windows.
- `--linux`: fuerza objetivo Linux.
- `--seed <valor>`: fija semilla de PRNG.

## Artefactos de salida

- Ejecutable final: `output` o `output.exe`.
- ASM: `output.asm` (si se conserva).
- Objeto: `output.o` o `output.obj`.
- IR: `output.ir` (backend `ir`).
- Diagnósticos JSON: `output.diagnostics.json`.
- AST JSON: `output.ast.json`.
- Pipeline JSON: `output.pipeline.json`.

## Manifest y lockfile

Formato mínimo de `aym.toml`:

```toml
[package]
name = "mi_proyecto"
version = "0.1.0"
edition = "2026"

[dependencies]
math = "^1.2.3"
```

Validaciones típicas:

```bash
aymc --check-manifest --emit-lock
aymc --check-manifest --check-lock
```

## Resolución de módulos

`apnaq("ruta")` resuelve por:

1. Directorio del archivo fuente.
2. `modules/` bajo el directorio fuente.
3. `AYM_PATH`.

Para imports por paquete (`apnaq("dep/mod")`) y `aym.lock` presente, usa:

1. `.aym/cache/<dep>/<resolved>/modules/`
2. `.aym/repo/<dep>/<resolved>/modules/`

## Ejemplos

```bash
aymc programa.aym
aymc --check --diagnostics-json programa.aym
aymc --emit-asm -o build/app programa.aym
aymc --compile-only -o build/app programa.aym
aymc --time-pipeline-json -o build/app programa.aym
```
