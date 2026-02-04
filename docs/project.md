# Guía del proyecto

## Tecnologías y herramientas

* **Lenguaje:** C++17
* **Arquitectura objetivo:** x86_64 Linux/Windows
* **Assembler:** NASM
* **Linker:** GNU LD / GCC (MinGW)
* **Sistema de construcción:** Make (Linux) / `build.bat` (Windows)
* **IDE recomendados:** CLion, VSCode, Vim
* **Control de versiones:** Git + GitHub
* **Tests:** `make test`

## Extensión para Visual Studio Code

La extensión básica para resaltado de sintaxis vive en `aymlang/`.
Para empaquetar un `.vsix`, sigue las instrucciones en
[`aymlang/README.md`](https://github.com/jhulio435m/aymaraLang/blob/main/aymlang/README.md).

## Documentación

El sitio de documentación está gestionado con **MkDocs** y vive en `docs/`.
Para levantarlo en local:

```bash
pip install mkdocs
mkdocs serve
```

## Estructura del repositorio

```
/aym/
├── compiler/        # Código fuente de 'aymc'
│   ├── lexer/       # Analizador léxico
│   ├── parser/      # Analizador sintáctico
│   ├── ast/         # Representación del AST
│   ├── codegen/     # Generador de código
│   └── utils/       # Utilidades comunes
├── runtime/         # Biblioteca estándar mínima
├── samples/         # Ejemplos en .aym (aymara_flow.aym, ejemplos/)
├── tests/           # Tests automatizados
├── docs/            # Documentación técnica (MkDocs)
├── build/           # Archivos generados
├── Makefile
└── README.md
```

## Cronograma de desarrollo

| Semana | Hito                                      |
| ------ | ----------------------------------------- |
| 1      | Diseño completo del lenguaje y gramática  |
| 2-3    | Implementación de Lexer y Parser          |
| 4      | Construcción del AST y sistema de tipos   |
| 5-6    | Generación de código + ensamblado inicial |
| 7-8    | Soporte para clases, ciclos y condiciones |
| 9-10   | Manejador de errores y mejoras de CLI     |
| 11     | Librería estándar mínima (`runtime/`)     |
| 12     | Documentación, empaquetado y publicación  |

## Licencia

Este proyecto se distribuye bajo la **licencia MIT**.

## Contribuciones

Este proyecto es abierto a toda colaboración. Nos interesa especialmente:

* Hablantes nativos de aymara
* Desarrolladores C++ con experiencia en compiladores
* Educadores y promotores de software libre

> ✨ ¡Únete al desarrollo y forma parte del cambio tecnológico-cultural!

---

**Anterior:** [Instaladores](installers.md)
