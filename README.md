# AymaraLang (`aym`) 🇵🇪

**AymaraLang** es un lenguaje de programación moderno basado en la lengua originaria aymara. Su compilador, `aymc`, ha sido desarrollado desde cero en **C++17**, y permite generar ejecutables nativos. El proyecto busca promover la inclusión tecnológica, la educación y la preservación lingüística.

Palabras clave principales del lenguaje:

- `jakhüwi`, `aru`, `chiqa`, `t'aqa` – tipos base (numérico, cadenas, booleanos/listas)
- `yatiya` – declaración de variables
- `qallta` / `tukuya` – inicio y fin del programa
- `qillqa` – salida por pantalla
- `katu` / `input` – lectura de consola
- `lurawi` / `kuttaya` – definición de funciones y retorno
- `jisa`/`maysatxa`, `ukhakamaxa`, `taki` (compatibles con `suti`, `jani`, `kunawsati`, `sapüru`)
- `apnaq` – importación de módulos desde otros archivos

---

## 📚 Documentación

- [Inicio](docs/index.md)
- [Visión general](docs/overview.md)
- [Compilación e instalación](docs/build.md)
- [CLI del compilador](docs/compiler.md)
- [Arquitectura del compilador](docs/arquitectura.md)
- [Primeros pasos](docs/language.md)
- [Referencia rápida](docs/aymaraLang.md)
- [Gramática formal](docs/grammar.md)
- [Investigación y teoría](docs/investigacion.md)

---

## 🚀 Inicio rápido

Compila y ejecuta un ejemplo sencillo:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/bin/aymc samples/aymara_flow.aym
./samples/aymara_flow
```

Para pasos detallados por sistema operativo, revisa la guía de compilación en
[`docs/build.md`](docs/build.md).

Para levantar el sitio de documentación con MkDocs:

```bash
pip install mkdocs
mkdocs serve
```
