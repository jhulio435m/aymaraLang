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

- [Inicio de la documentación](docs/source/inicio/bienvenida.rst)
- [Visión general del lenguaje](docs/source/inicio/vision_general.rst)
- [Características y ejemplos](docs/source/primeros_pasos/introduccion_al_lenguaje.rst)
- [Arquitectura del compilador](docs/source/primeros_pasos/cli_y_flujo_de_compilacion.rst)
- [Compilación, instalación y uso](docs/source/primeros_pasos/instalacion_y_compilacion.rst)
- [Guía del proyecto](docs/source/primeros_pasos/guia_del_proyecto.rst)
- [Gramática formal](docs/source/lenguaje/gramatica_formal.rst)

---

## 🚀 Inicio rápido

Compila y ejecuta un ejemplo sencillo:

```bash
$ ./bin/aymc samples/basics/hola.aym
$ ./bin/hola
Kamisaraki!
```

Para pasos detallados por sistema operativo, revisa la guía de compilación en
[`docs/source/primeros_pasos/instalacion_y_compilacion.rst`](docs/source/primeros_pasos/instalacion_y_compilacion.rst).

Para construir el sitio de documentación con Sphinx:

```bash
pip install sphinx sphinx_rtd_theme
sphinx-build -b html docs/source docs/build/html
```
