# Estado de modernización

Este documento resume el estado técnico vigente del plan de modernización.

## Objetivo

Consolidar AymaraLang como toolchain reproducible y mantenible para uso real en
Windows y Linux.

## Estado actual

| Línea | Estado |
| --- | --- |
| Compilación nativa (`aymc`) | Operativa |
| Gestor de proyectos (`aym`) | Operativo |
| Manifest/lock (`aym.toml`, `aym.lock`) | Operativo |
| Diagnósticos estructurados | Operativo |
| Pipeline con métricas y timeout | Operativo |
| Empaquetado Windows (MSI/NSIS) | Operativo |
| Empaquetado Linux (.deb) | Operativo |
| Asociación `.aym` con icono | Operativa |

## Capacidades consolidadas

- Validación de manifest y lockfile con comandos dedicados.
- Resolución de imports por dependencia usando caché/repo local.
- Exporte de AST, diagnósticos y métricas en JSON.
- Modos de compilación por etapa (`--compile-only`, `--link-only`).
- Smoke tests de empaquetado e instalación.

## Trabajo pendiente (backlog activo)

- Endurecer cobertura de pruebas por módulos de lenguaje avanzados.
- Mejorar documentación de API/built-ins por dominio.
- Evolucionar tooling de editor (diagnóstico/UX).
- Definir política formal de compatibilidad de lenguaje para `1.x`.

## Criterio de avance

Se considera incremento aceptado cuando cumple simultáneamente:

1. Build reproducible en Windows y Linux.
2. Pruebas automatizadas en verde.
3. Documentación actualizada en el mismo cambio.
4. Sin regresiones en instaladores.
