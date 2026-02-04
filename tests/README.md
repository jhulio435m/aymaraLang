# Tests

Este directorio contiene pruebas automatizadas para el compilador `aymc`.
Para ejecutarlas solo es necesario ejecutar:

```bash
$ make test
```

El script `run_tests.sh` compila el compilador, genera los ejemplos principales
en `samples/` y comprueba la salida exacta de `samples/aymara_flow.aym`. Además,
valida la ejecución de los ejemplos `samples/ejemplos/basicos.aym` y
`samples/ejemplos/funciones_listas.aym` con entradas de prueba.

Además, ejecuta una prueba rápida de empaquetado con CPack si `cmake` y `cpack`
están disponibles en el entorno.
