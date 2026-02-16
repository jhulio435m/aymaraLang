# Estructura del proyecto

## Directorios principales

| Ruta | Contenido |
| --- | --- |
| `compiler/` | Implementación de `aymc` |
| `runtime/` | Runtime del lenguaje |
| `tools/aym/` | Gestor de proyectos `aym` |
| `samples/` | Programas de ejemplo |
| `tests/` | Pruebas automatizadas |
| `scripts/` | Automatización de build/install/test |
| `installer/` | Definiciones de instaladores Windows |
| `packaging/` | Empaquetado Linux (`.deb`) |
| `docs/` | Documentación técnica |

## CLI de proyecto (`aym`)

Uso:

```bash
aym <comando> [opciones]
```

Comandos principales:

- `new`: crea estructura de proyecto.
- `build`: compila el proyecto.
- `run`: compila y ejecuta.
- `test`: valida `tests/*.aym`.
- `lock`: sincroniza o valida lockfile.
- `cache`: inspecciona/sincroniza/limpia caché local.
- `add`: agrega dependencia al manifest.

## Archivos de proyecto

- `aym.toml`: manifest.
- `aym.lock`: lockfile reproducible.
- `.aym/repo` y `.aym/cache`: almacenamiento local de dependencias.

## Flujo recomendado

1. `aym new <nombre>`
2. `aym add <dep> <requirement>`
3. `aym build`
4. `aym run`
5. `aym test`
