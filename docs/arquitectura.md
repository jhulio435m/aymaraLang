# Arquitectura del compilador `aymc`

Este documento resume los módulos principales y su flujo de compilación.

## Vista general

```mermaid
flowchart LR
classDef front fill:#dbeafe,stroke:#1e40af,color:#000;
classDef middle fill:#dcfce7,stroke:#166534,color:#000;
classDef back fill:#fee2e2,stroke:#7f1d1d,color:#000;

A[Fuente .aym] --> B[Lexer]
B --> C[Parser]
C --> D[AST]
D --> E[Analisis semantico]
E --> F[Codegen NASM]
F --> G[Ensamblador + Linker]
G --> H[Ejecutable nativo]
E --> I[Codegen LLVM opcional]
I --> J[IR LLVM .ll]

class A,B,C,D front
class E middle
class F,G,H,I,J back
```

## Módulos principales

- **Lexer (`compiler/lexer`)**: tokeniza el código fuente.
- **Parser (`compiler/parser`)**: construye el AST y valida la sintaxis.
- **AST (`compiler/ast`)**: nodos de expresiones y sentencias.
- **Semántica (`compiler/semantic`)**: tipos, símbolos, clases y excepciones.
- **Codegen (`compiler/codegen`)**: genera NASM o LLVM IR.
- **Runtime (`runtime/`)**: funciones base usadas por los binarios.
- **Module resolver (`utils/module_resolver`)**: carga `apnaq` y gestiona rutas.

```mermaid
flowchart TB
    Lexer --> Parser
    Parser --> AST
    AST --> Semantica[Semántica]
    Semantica --> Codegen
    Codegen --> Runtime
    Semantica --> Resolver[Module Resolver]
```

## Artefactos clave

- `*.asm` y `*.o`/`*.obj` para el backend NASM.
- `*.ll` para el backend LLVM (si está habilitado).
- Binario final nativo.

---

**Siguiente:** [Guía del proyecto](project.md)
