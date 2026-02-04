# AymaraLang — documentación

Bienvenido/a. Esta documentación resume el lenguaje, el compilador y los
materiales de investigación del proyecto de forma concisa.

## Rutas principales

- **Panorama del proyecto:** [Visión general](overview.md),
  [Compilación e instalación](build.md), [CLI del compilador](compiler.md) y
  [Arquitectura](arquitectura.md).
- **Lenguaje:** [Primeros pasos](language.md),
  [Referencia rápida](aymaraLang.md), [Manejo de errores](exceptions.md) y
  [Gramática formal](grammar.md).
- **Investigación y teoría:** [Fundamentos matemáticos y de compiladores](investigacion.md).
- **Ingeniería de sistemas (investigación):** documentos en
  [`docs/systems_engineering/`](systems_engineering/).
  Incluyen plan de gestión, requisitos, arquitectura y validación.
- **LaTeX/Overleaf:** los insumos en
  [`docs/systems_engineering/overleaf/`](systems_engineering/overleaf/) son
  parte del material de investigación y se mantienen vigentes.

## Sitio MkDocs

```bash
pip install mkdocs
mkdocs serve
```

Luego abre `http://127.0.0.1:8000`.

## Recursos visuales y fórmulas

La documentación usa diagramas Mermaid y fórmulas matemáticas para reforzar la
investigación. Por ejemplo, el tiempo de análisis léxico puede modelarse como
$O(n)$ respecto al tamaño de la entrada.
