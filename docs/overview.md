# Visión general

## Definición del proyecto

AymaraLang es un lenguaje compilado con sintaxis basada en aymara. El
compilador oficial, `aymc`, está implementado en C++17 y genera ejecutables
nativos.

## Componentes principales

| Componente | Propósito |
| --- | --- |
| `aymc` | Compila archivos `.aym` a ejecutables nativos |
| `aym` | Orquesta flujos de proyecto (`new`, `build`, `run`, `test`, `lock`, `cache`, `add`) |
| Runtime (`dist/share/aymaraLang/runtime`) | Soporte de funciones de ejecución |
| `samples/` | Programas de referencia y validación funcional |

## Flujo de compilación

```mermaid
flowchart LR
    A[Fuente .aym] --> B[Lexer]
    B --> C[Parser]
    C --> D[AST]
    D --> E[Semántica]
    E --> F[Codegen NASM]
    F --> G[Ensamblado y enlace]
    G --> H[Ejecutable nativo]
```

## Plataformas objetivo

- Windows x64
- Linux x64

## Estado funcional resumido

- Compilación nativa operativa.
- Asociación de archivos `.aym` en instaladores.
- Gestión de proyectos con manifest (`aym.toml`) y lockfile (`aym.lock`).
- Pruebas automatizadas para build, smoke y empaquetado.

## Referencias relacionadas

- [Instalación](install.md)
- [Compilación desde fuente](build.md)
- [CLI del compilador](compiler.md)
- [Arquitectura](arquitectura.md)
