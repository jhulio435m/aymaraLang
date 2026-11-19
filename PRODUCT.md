# Product

<!-- impeccable:product-schema 1 -->

## Platform

web

## Stack

Next.js (App Router, TypeScript) con Tailwind CSS 4, en `web/`, exportado como sitio estático y publicado en Cloudflare Pages conectado a Git. Mismo patrón que `chemistry.soryntel.com` y `kusi.soryntel.com`. Aislado del compilador. Elegido por el usuario al aprobar el plan.

## Users

Público principal: estudiantes y docentes que aprenden o enseñan programación en Perú y Bolivia (colegios, institutos, universidades), hablen o no aymara. Llegan para saber qué es AymaraLang y si vale la pena instalarlo. Acción principal: descargar el instalador. Acción secundaria: abrir la documentación.

## Product Purpose

AymaraLang (`aym`) es un lenguaje de programación cuyas palabras clave están en aymara. La web presenta el producto y lleva a la descarga; no aloja la documentación. El visitante debe salir convencido de que es un lenguaje de verdad, no una traducción de juguete.

## Positioning

Lenguaje con compilador propio (`aymc`, escrito desde cero en C++17) que genera ejecutables nativos, con sintaxis canónica en aymara: `qallta`/`tukuya`, `yatiya`, `qillqa`, `ukaxa`/`maysatxa`, `kuti`, `lurawi`/`kuttaya`, `apnaq`. Se distribuye con instaladores para Windows y paquete `.deb` para Linux.

## Operating Context

- Se escribe un archivo `.aym`, se compila con `aymc archivo.aym` y se ejecuta el binario resultante.
- Windows: `AymaraLang-Setup.exe` (NSIS, recomendado) o `AymaraLang-Setup.msi`; incluyen `nasm` y `gcc`/MinGW, agregan `bin` al `PATH` y asocian `.aym`.
- Linux: paquete `.deb`; requiere `nasm`, `gcc`, `g++` del sistema y X11 para programas con GUI.
- Documentación: MkDocs en `https://jhulio435m.github.io/aymaraLang/`. Se migrará al mismo dominio más adelante (pendiente, fuera de este alcance).
- Código fuente: `https://github.com/jhulio435m/aymaraLang`.

## Capabilities and Constraints

- Tipos base `jakhüwi`, `aru`, `chiqa`, `t'aqa`, `mapa`; funciones, control de flujo, enumeraciones y selección, módulos (`apnaq`), excepciones, orientación a objetos, colecciones.
- Programas de ejemplo reales en `samples/`: apps de consola (n reinas, banca con excepciones, inventario con archivo) y juegos de consola y con GUI (tetris, katari, jamachi, sudoku).
- El compilador emite código nativo x86‑64 vía NASM: no hay playground en el navegador y la web no debe simular uno.
- Sin macOS publicado en releases (existe `Makefile.macos`, sin instalador): no ofrecer descarga para macOS.
- Versión publicada: v1.0.2. El `.deb` se publica con nombre `aymaralang_0.1.0_amd64.deb`; la web toma nombres y tamaños reales del release.

## Brand Commitments

- Nombre: AymaraLang; comando `aym`, compilador `aymc`, extensión `.aym`.
- Logo existente: chakana multicolor con una llama y rótulo naranja/azul (`docs/img/logo_aymaralang.png`, `assets/logo.png`).
- Idioma de la web: español. Las palabras clave en aymara se muestran tal cual, con su grafía (`jakhüwi`, `t'aqa`).
- Dirección visual fijada por el usuario: identidad andina propia derivada del logo, con el código aymara como protagonista; distinta de kusi y chemistry. Pie con "Un producto de Soryntel".
- Debe evitar, por decisión del usuario: plantilla genérica de herramienta dev (fondo oscuro, neón, terminal); parecer un juguete; folclore de postal (web de turismo o artesanía); tono solemne o institucional.

## Evidence on Hand

- `README.md`, `docs/language.md`, `docs/aymaraLang.md`, `docs/grammar.md`, `docs/compiler.md`, `docs/install.md`: sintaxis, CLI e instalación.
- `samples/`: código real para mostrar en la web.
- GitHub Releases v1.0.2: `AymaraLang-Setup.exe`, `AymaraLang-Setup.msi`, `aymaralang_0.1.0_amd64.deb`.
- Licencia en `LICENSE`.
- No hay testimonios, cifras de uso, instituciones aliadas, benchmarks publicados ni capturas de pantalla: no deben inventarse.

## Product Principles

- Demostrar con código real que compila; cada fragmento mostrado sale del repositorio.
- La lengua aymara es el material del producto, no su decoración.
- Tratar al estudiante como programador: serio sin ser solemne.
- Un solo camino claro: descargar, escribir `qallta`, compilar.
- No afirmar nada que el repositorio no respalde.
