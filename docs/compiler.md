# CLI del compilador (`aymc`)

Esta página resume sólo lo necesario para usar `aymc`. Para un flujo completo,
empieza por el [Tutorial rápido](language.md) o el [Manual de usuario](manual_usuario.md).

## Forma básica

```bash
aymc [opciones] archivo.aym ...
```

Si entregas varios archivos, `aymc` los compila como una sola unidad.

## Opciones más usadas

- `-h`, `--help`: muestra ayuda.
- `-o <ruta>`: define el ejecutable de salida.
- `--check`: valida sintaxis y semántica sin generar binario.
- `--backend <nombre>`: selecciona `native` o `ir`.
- `--emit-asm`: conserva el ASM intermedio.
- `--compile-only`: genera ASM u objeto sin enlazar.
- `--link-only`: enlaza un objeto existente.
- `--windows`, `--linux`: fuerza la plataforma objetivo.
- `--diagnostics-json[=ruta]`: exporta diagnósticos en JSON.
- `--emit-ast-json[=ruta]`: exporta AST en JSON.
- `--time-pipeline-json[=ruta]`: exporta tiempos del pipeline.

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

Opciones relacionadas:

- `--check-manifest[=ruta]`
- `--manifest <ruta>`
- `--emit-lock[=ruta]`
- `--check-lock[=ruta]`
- `--lock <ruta>`

## Resolución de módulos

`apnaq("ruta")` resuelve por:

1. Directorio del archivo fuente.
2. `modules/` bajo el directorio fuente.
3. `AYM_PATH`.

Para imports por paquete (`apnaq("dep/mod")`) y `aym.lock` presente, usa:

1. `.aym/cache/<dep>/<resolved>/modules/`
2. `.aym/repo/<dep>/<resolved>/modules/`

## Ejemplos mínimos

```bash
aymc programa.aym
aymc --check --diagnostics-json programa.aym
aymc --emit-asm -o build/app programa.aym
aymc --compile-only -o build/app programa.aym
aymc --time-pipeline-json -o build/app programa.aym
```
