# CLI y flujo de compilación

Esta sección describe cómo usar el compilador `aymc`, sus banderas principales y
el flujo de trabajo típico al compilar programas `.aym`.

## Uso básico

```bash
./bin/aymc archivo.aym
```

El compilador genera un ejecutable en `bin/` con el mismo nombre del archivo de
entrada. También produce archivos intermedios en `build/`. El flujo es
exclusivamente de compilación: no hay ejecución interactiva dentro del
compilador.

## Opciones principales

- `--llvm` genera un archivo LLVM IR (`.ll`) con un resumen del AST.
- `--windows` fuerza la salida a un ejecutable de Windows (`.exe`).
- `--linux` fuerza la salida a un ejecutable de Linux.
- `--seed <valor>` fija la semilla del generador aleatorio.

## Flujo de compilación

1. **Lexer**: tokeniza el código fuente.
2. **Parser**: construye el AST y valida la estructura.
3. **Análisis semántico**: verifica tipos, símbolos y alcance.
4. **Codegen**: produce NASM o LLVM IR.
5. **Ensamblado y enlace**: genera el ejecutable final.

## Archivos generados

- `build/<nombre>.asm`: salida NASM (se elimina automáticamente en Windows).
- `build/<nombre>.o`: objeto ensamblado (en Windows se usa `.obj` y se elimina automáticamente).
- `bin/<nombre>` o `bin/<nombre>.exe`: ejecutable final.

## Consejos de depuración

- Usa `--llvm` para inspeccionar el resumen del AST.
- Si aparece un error de símbolos, revisa declaraciones y ámbito.
- Para reproducir resultados aleatorios, fija `--seed`.

---

**Anterior:** [Arquitectura del compilador](arquitectura.md) | **Siguiente:** [Compilación y uso](build.md)
