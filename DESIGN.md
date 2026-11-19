---
name: AymaraLang
description: Awayu tejido con el propio código; cada línea es una pasada y cada clase de palabra, un hilo.
colors:
  pampa: "#22112a"
  pampa-alta: "#2f1a3a"
  pano: "#f6efe2"
  pano-bajo: "#eadfca"
  naranja: "#f26a0b"
  amarillo: "#ffd21f"
  azul: "#0b5fae"
  nudo-ambar: "#e0a800"
  nudo-verde: "#1f9d68"
  tinta-suave: "#5b4664"
  crema-suave: "#d3c4d9"
  celeste-suave: "#d9e8f8"
  hilo-clave: "#f0468a"
  hilo-tipo: "#f9792b"
  hilo-texto: "#ffd21f"
  hilo-numero: "#2cc283"
  hilo-nombre: "#58a6f5"
  hilo-nota: "#9a86a6"
  hilo-signo: "#d3c4d9"
typography:
  display:
    fontFamily: "Anybody Variable, Arial Narrow, sans-serif"
    fontSize: "clamp(2.6rem, min(5.6vw, 9vh), 5rem)"
    fontWeight: 800
    lineHeight: 0.98
    letterSpacing: "-0.02em"
    fontVariation: "\"wdth\" 116"
  headline:
    fontFamily: "Anybody Variable, Arial Narrow, sans-serif"
    fontSize: "clamp(2rem, 4.3vw, 3.75rem)"
    fontWeight: 800
    lineHeight: 1.02
    letterSpacing: "-0.02em"
    fontVariation: "\"wdth\" 114"
  title:
    fontFamily: "Anybody Variable, Arial Narrow, sans-serif"
    fontSize: "1.3rem"
    fontWeight: 750
    lineHeight: 1.2
    fontVariation: "\"wdth\" 110"
  body:
    fontFamily: "Schibsted Grotesk Variable, system-ui, sans-serif"
    fontSize: "1.0625rem"
    fontWeight: 400
    lineHeight: 1.6
  label:
    fontFamily: "Schibsted Grotesk Variable, system-ui, sans-serif"
    fontSize: "0.9rem"
    fontWeight: 550
    lineHeight: 1.35
  code:
    fontFamily: "Martian Mono Variable, ui-monospace, monospace"
    fontSize: "0.875rem"
    fontWeight: 400
    lineHeight: 1.75
    fontVariation: "\"wdth\" 88"
rounded:
  none: "0px"
spacing:
  margen: "clamp(1rem, 4vw, 3.5rem)"
  seccion: "clamp(4.5rem, 9vw, 8rem)"
  ancho: "78rem"
  cabecera: "68px"
  hilo: "2px"
components:
  boton:
    backgroundColor: "{colors.pampa}"
    textColor: "{colors.pano}"
    typography: "{typography.title}"
    rounded: "{rounded.none}"
    padding: "0.6rem 1.4rem"
    height: "3.5rem"
  boton-hover:
    backgroundColor: "{colors.amarillo}"
    textColor: "{colors.pampa}"
  ribete-celda:
    backgroundColor: "{colors.naranja}"
    textColor: "{colors.pampa}"
    rounded: "{rounded.none}"
    height: "7rem"
  ribete-celda-hover:
    backgroundColor: "{colors.amarillo}"
  pestana:
    backgroundColor: "{colors.pampa}"
    textColor: "{colors.crema-suave}"
    padding: "0.8rem 1.25rem"
  pestana-activa:
    backgroundColor: "{colors.pampa-alta}"
    textColor: "{colors.pano}"
---

# Design System: AymaraLang

## Overview

El sitio es un awayu. No usa el tejido como adorno: lo genera. `web/src/lib/aym.ts` clasifica cada token de un programa `.aym` y esa clasificación pinta dos cosas a la vez, el resaltado del código y el hilo del tejido. Las franjas entre secciones (`Franja`) y las figuras (`Salta`) son siempre código real del repositorio, reflejado sobre el eje central.

Serio sin ser solemne. Lejos de la plantilla de herramienta dev (negro, neón, terminal), del juguete y del folclore de postal: nada de llamas, montañas ni texturas «andinas». La única imagen es el logo.

## Colors

Paleta completa con papeles fijos.

- **Pampa** (`pampa`, `pampa-alta`): berenjena, el campo liso del tejido. Fondo de la portada, de los ejemplos y de todo bloque de código.
- **Paño** (`pano`, `pano-bajo`): crema, para leer. Cabecera, vocabulario, compilación y pie.
- **Campos**: `azul` para «Hecho con AymaraLang» y `naranja` para descargar. Ocupan secciones enteras, nunca acentos sueltos. `amarillo` es el estado activo (hover, selección, línea resaltada).
- **Hilos**: un color por clase de palabra y solo para eso. Rosa palabra clave, naranja tipo, amarillo texto, verde número o valor, azul nombre, malva comentario, lila signo. El mismo color en el código y en el tejido.

Texto secundario teñido del fondo: `tinta-suave` sobre paño, `crema-suave` sobre pampa, `celeste-suave` sobre azul. Sobre naranja, siempre pampa.

## Typography

- **Anybody** ancha y pesada (`wdth` 108 a 116, peso 750 a 800) para titulares, rótulos y botones. Sin mayúsculas forzadas salvo la marca.
- **Schibsted Grotesk** para lectura.
- **Martian Mono** estrecha (`wdth` 80 a 88) solo para código, nombres de archivo y comandos. En la portada el programa se muestra a tamaño de cartel.

Todas autoalojadas con `@fontsource-variable`.

## Layout

Bandas de borde a borde; el contenido va en `.ancho` (78rem). Cada sección abre con `.titular`: titular a la izquierda y párrafo a la derecha, alineados abajo. Entre secciones, una franja tejida de 14 px por pasada. La portada llena la primera pantalla (tope de 58rem) y termina en el ribete naranja de descarga.

Bajo 1100 px se oculta la navegación; bajo 860 px todo pasa a una columna y el tejido del ejemplo sube sobre el código; bajo 560 px el tendedero se recorre de lado y cada tejido conserva su largo real.

## Elevation & Depth

Plano. No hay sombras ni desenfoques. La profundidad es de capas de color: pampa y `pampa-alta` para el telar y su pestaña activa.

## Shapes

Todo es recto, sin radios: el tejido es una retícula. Separaciones de 2 px del color de la pampa entre tramos y pasadas. Las reglas de 4 px marcan de dónde cuelga algo (grupos del vocabulario, tendedero, urdimbre, columnas de descarga).

## Components

- **Franja**: pasadas a todo el ancho, HTML estático, `aria-hidden`.
- **Salta**: figura en canvas; la línea crece desde el centro y la sangría queda como pampa. Entra pasada a pasada al cambiar de programa (cúbica de salida, máximo 900 ms) y atenúa las demás pasadas al resaltar una. Respeta `prefers-reduced-motion`.
- **Botón**: ribete pampa con filete inferior amarillo que sube y llena el botón al pasar el cursor.
- **Ribete de descarga**: celdas naranjas de alto completo que son enlaces directos al instalador.
- **Pestañas**: `tablist` con flechas; la activa toma el color del telar (`pampa-alta`) con filete rosa arriba. En teléfono, una sola fila que se desliza.
- **Botón de descarga**: nombra la acción; el nombre del archivo y su tamaño van debajo en mono pequeña.
- **Muestrario**: listas de definición en columnas, cuadro de hilo junto a cada palabra.
- **Foco**: contorno de 3 px del color del texto, separado 3 px.

## Do's and Don'ts

- Sí: generar toda franja o figura desde código real con `tokenLines`.
- Sí: usar los colores de hilo solo con su significado.
- Sí: campos de color a escala de sección.
- No: radios, sombras, degradados, vidrio ni tarjetas con icono.
- No: ilustraciones o motivos andinos decorativos.
- No: fondo negro puro ni acentos neón; la pampa es berenjena.
- No: mono fuera del código.
