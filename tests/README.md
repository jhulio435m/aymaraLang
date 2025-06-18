# Tests

Este directorio contiene pruebas automatizadas para el compilador `aymc`.
Para ejecutarlas solo es necesario ejecutar:

```bash
$ make test
```

El script `run_tests.sh` compila el compilador, genera cada ejemplo en
`samples/` y comprueba su salida. También valida que se reporten errores
cuando corresponde.
