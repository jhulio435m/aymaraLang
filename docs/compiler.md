# Arquitectura del compilador

El compilador `aymc` está estructurado en varias etapas clásicas de diseño de compiladores:

1. **Análisis Léxico** – Tokenización de entrada `.aym`.
2. **Análisis Sintáctico** – Construcción del árbol de derivación según la gramática LL(k).
3. **Construcción del AST** – Representación semántica abstracta.
4. **Análisis Semántico** – Tipado, resolución de símbolos, validaciones.
5. **Optimización Intermedia** – (opcional) Reescritura del AST para mejoras.
6. **Generación de Código** – Código ensamblador x86_64 o LLVM IR (backend experimental).
7. **Ensamblado y Enlace** – Uso de `nasm` y `gcc` para crear `.ayn`.

Las estructuras ahora incluyen `else`, ciclos `for` y funciones simples.

Las condiciones y bucles ahora se ejecutan en tiempo de ejecución gracias a un
AST más completo, análisis semántico y generación de código en ensamblador.

> ⚙️ El backend LLVM está disponible de forma experimental mediante `aymc --llvm`.

## Backend LLVM experimental

El backend basado en LLVM IR se puede invocar añadiendo la bandera `--llvm` al compilador.
Genera un archivo `.ll` con un módulo LLVM que describe de forma resumida el programa analizado.

```bash
$ ./bin/aymc --llvm samples/hola.aym
$ cat build/hola.ll
```

El IR generado imprime por consola un resumen del AST, útil para validar la integración con LLVM antes de ampliar el backend.

## Modo REPL

El compilador incluye un modo interactivo que permite ejecutar código línea por línea:

```bash
$ ./bin/aymc --repl
AymaraLang REPL - escribe código línea por línea (escribe 'salir' para terminar)
aym> jach’a x = 5;
aym> x + 2
7
aym> salir
```

## Errores comunes

- **Variable no declarada:** usar una variable sin declararla mostrará un mensaje `Error: variable 'x' no declarada`.
- **`break` fuera de ciclo:** si se usa `break` fuera de `mientras`, `para` o `tantachaña` se emitirá `Error: 'break' fuera de un ciclo o switch`.
