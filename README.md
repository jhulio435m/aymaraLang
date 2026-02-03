# AymaraLang (`aym`) 🇵🇪

**AymaraLang** es un lenguaje de programación moderno basado en la lengua originaria aymara. Su compilador, `aymc`, ha sido desarrollado desde cero en **C++17**, y permite generar ejecutables nativos `.ayn`. El proyecto busca promover la inclusión tecnológica, la educación y la preservación lingüística.

Palabras clave principales del lenguaje:

- `jach’a`, `lliphiphi`, `qillqa`, `chuymani` – tipos primitivos (int, float, string, bool)
- `willt’aña` – salida por pantalla
- `input` – lectura de consola
- `luräwi` / `kutiyana` – definición de funciones y retorno
- `si`, `sino`, `mientras`, `haceña`, `para`, `tantachaña`
- `apu` – importación de módulos desde otros archivos

---

## 📚 Documentación

Empieza aquí y navega por la documentación en módulos más pequeños:

- [Inicio de la documentación](docs/index.md)
- [Visión general del lenguaje](docs/overview.md)
- [Características y ejemplos](docs/language.md)
- [Arquitectura del compilador](docs/compiler.md)
- [Compilación, instalación y uso](docs/build.md)
- [Guía del proyecto](docs/project.md)
- [Gramática formal](docs/grammar.md)

---

## 🚀 Inicio rápido

Compila y ejecuta un ejemplo sencillo:

```bash
$ ./bin/aymc samples/basics/hola.aym
$ ./bin/hola
Kamisaraki!
```

Para pasos detallados por sistema operativo, revisa la guía de compilación en
[`docs/build.md`](docs/build.md).

Para levantar el sitio de documentación con MkDocs:

```bash
pip install mkdocs
mkdocs serve
```
