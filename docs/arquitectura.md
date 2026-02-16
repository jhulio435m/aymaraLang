# Arquitectura del compilador

## Pipeline

```mermaid
flowchart LR
    A[Fuente .aym] --> B[Lexer]
    B --> C[Parser]
    C --> D[AST]
    D --> E[Semántica]
    E --> F[Codegen]
    F --> G[NASM + GCC/LD]
    G --> H[Binario nativo]
```

## Componentes

| Módulo | Ruta | Responsabilidad |
| --- | --- | --- |
| Lexer | `compiler/lexer/` | Tokenización |
| Parser | `compiler/parser/` | Análisis sintáctico y construcción AST |
| AST | `compiler/ast/` | Estructuras intermedias del programa |
| Semántica | `compiler/semantic/` | Validación de tipos, símbolos y reglas |
| Codegen | `compiler/codegen/` | Emisión de ASM/objeto/binario |
| Backend | `compiler/backend/` | Coordinación de modos de salida y pipeline |
| Utilidades | `compiler/utils/` | Diagnósticos, procesos, resolver de módulos |
| Runtime | `runtime/` | Soporte de ejecución para built-ins |

## Artefactos intermedios

- `*.asm`
- `*.o` / `*.obj`
- binario final

## Decisiones de diseño vigentes

- Backend principal nativo basado en NASM.
- Integración de diagnósticos estructurados en frontend.
- Pipeline por etapas con métricas y timeout configurable.
- Resolución de dependencias vinculada a `aym.toml` y `aym.lock`.

## Observabilidad

- `--time-pipeline` para tiempos por etapa.
- `--time-pipeline-json` para exporte estructurado.
- `--diagnostics-json` para integración con tooling externo.
