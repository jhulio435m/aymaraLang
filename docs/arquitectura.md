# Arquitectura del compilador `aymc`

Este documento describe los módulos principales del compilador de AymaraLang y la forma en que se comunican entre sí. También se explican las estructuras de datos usadas internamente y se presentan ejemplos de la compilación paso a paso de un programa sencillo.

## Módulos

### Lexer
- **Ubicación:** `compiler/lexer`
- Convierte el código fuente en una secuencia de *tokens*.
- Cada token posee tipo, texto, línea y columna (`struct Token`).
- Reconoce palabras clave en aymara (`si`, `sino`, `mientras`, etc.), operadores y literales.

### Parser
- **Ubicación:** `compiler/parser`
- Recibe la lista de tokens y produce un Árbol de Sintaxis Abstracta (AST).
- Implementa un análisis LL que construye nodos de tipo `Expr` y `Stmt`.

### AST
- **Ubicación:** `compiler/ast`
- Conjunto de clases que representan expresiones y sentencias.
- Los nodos derivan de `Node` y se visitan mediante el patrón *visitor* (`ASTVisitor`).
- Ejemplos de nodos: `NumberExpr`, `BinaryExpr`, `IfStmt`, `FunctionStmt`.

### Análisis semántico
- **Ubicación:** `compiler/semantic`
- Comprueba tipos, ámbito de variables y llamadas a funciones.
- Mantiene una pila de tablas de símbolos (`scopes`) y genera la lista de variables globales y tipos para la etapa de código.

### Generador de código
- **Ubicación:** `compiler/codegen`
- Toma el AST validado y lo transforma en ensamblador NASM para `x86_64`.
- Invoca a `nasm` y `ld` (vía `gcc`) para producir el ejecutable `.ayn`.

### Runtime
- **Ubicación:** `runtime`
- Biblioteca mínima con funciones de E/S usadas por los programas generados.

## Flujo de compilación

```text
Fuente (.aym) ──► Lexer ──► Parser ──► AST ──► Análisis semántico ──► CodeGen ──► NASM/ld ──► Ejecutable (.ayn)
```

Cada etapa toma la salida de la anterior y agrega información o transforma la representación del programa hasta generar código máquina.

## Estructuras de datos

### Tokens
Definidos en `lexer.h`:
```cpp
enum class TokenType { Identifier, String, Number, Plus, Minus, ... };
struct Token {
    TokenType type;
    std::string text;
    size_t line;
    size_t column;
};
```

### Nodos del AST
Definidos en `ast.h`. Los más usados son:
- `Expr` / `Stmt`: clases base.
- `BinaryExpr`, `UnaryExpr`, `CallExpr` para expresiones.
- `PrintStmt`, `IfStmt`, `ForStmt`, `FunctionStmt` para sentencias.

### Símbolos
El analizador semántico maneja un vector de mapas `scopes` donde cada mapa asocia nombres a tipos. Esto permite verificar declarciones y usos de variables y funciones.

## Ejemplo paso a paso

Tomemos `samples/hola.aym`:
```aymara
willt’aña("Kamisaraki!");
```
1. **Lexer** genera tokens: `KeywordPrint`, `LParen`, `String("Kamisaraki!")`, `RParen`, `Semicolon`.
2. **Parser** construye un `PrintStmt` con un `StringExpr`.
3. **Análisis semántico** valida que la llamada es correcta.
4. **CodeGen** produce un archivo NASM que imprime la cadena y genera el ejecutable con `nasm` y `ld`.
5. Al ejecutar el binario se obtiene:
```
Kamisaraki!
```

Este mismo flujo aplica a programas más grandes, incluyendo funciones, bucles y estructuras de control.

---

**Anterior:** [Gramática formal](grammar.md) | **Siguiente:** [CLI y flujo de compilación](compiler.md)
