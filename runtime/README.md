# Runtime

Este directorio contiene la biblioteca estándar mínima para el lenguaje `aym`. Por ahora se apoya en la función `printf` de la libc para realizar operaciones de entrada y salida básicas.

Se incluye la función `leer_linea` para lectura básica desde entrada estándar.

El archivo `math.c` expone envoltorios sencillos (`aym_sin`, `aym_cos`, `aym_sqrt`, etc.) sobre `<math.h>` para que el compilador pueda enlazarlos como funciones builtin del lenguaje.
