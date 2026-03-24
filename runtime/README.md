# Runtime

Este directorio contiene la biblioteca estándar mínima para el lenguaje `aym`. Por ahora se apoya en la función `printf` de la libc para realizar operaciones de entrada y salida básicas.

Se incluye la función `leer_linea` para lectura básica desde entrada estándar.

Para mejorar mantenibilidad, `runtime.c` organiza bloques grandes mediante
`#include` de módulos locales:

- `runtime_arrays.c`
- `runtime_maps_strings.c`
- `runtime_exceptions.c`

El archivo `math.c` expone envoltorios sencillos (`aym_sin`, `aym_cos`, `aym_sqrt`, etc.) sobre `<math.h>` para que el compilador pueda enlazarlos como funciones builtin del lenguaje.

En Linux, el backend GUI (`uja_*`) usa X11 desde `runtime_gfx_linux.c`. Para
compilar y ejecutar juegos GUI se requieren headers/librerías de X11
(`pkg-config --exists x11`) y un `DISPLAY` activo o un runner headless como
`xvfb-run`.
