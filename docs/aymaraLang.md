# AymaraLang

AymaraLang (`aym`) es un lenguaje de programación experimental con sintaxis inspirada en Python y un compilador escrito en C++17. Genera código NASM para x86_64 y enlaza con `gcc`.

## Sintaxis soportada
- Tipos: `int`, `float`, `bool`, `string`
- Variables y asignación
- Impresión con `willt’aña(expr)`
- Control de flujo: `si`/`sino`, `while`, `do...while`, `for`, `switch`
- Funciones con `func nombre(params) { ... }`
- Expresiones aritméticas `+ - * / % ^`
- Operadores lógicos `and`, `or`, `not`, comparaciones `== != < <= > >=`
- Comentarios `//` y `/* */`
- Lectura de consola con `input()`

## Compilación
Para compilar un archivo `.aym` se ejecuta:

```bash
$ ./bin/aymc samples/archivo.aym
$ ./build/out
```

El compilador produce un archivo NASM, lo ensambla y enlaza automáticamente.

## Errores comunes
- Variable no declarada
- Tipos incompatibles en asignaciones o expresiones
- Uso de `break` o `continue` fuera de bucles
- `return` fuera de una función

## Palabras clave
`willt’aña`, `si`, `sino`, `mientras`, `do`, `para`, `in`, `break`, `continue`, `func`, `retorna`, `switch`, `case`, `default`, `and`, `or`, `not`, `int`, `float`, `bool`, `string`, `input`

## Ejemplo
```aymara
string nombre = input();
willt’aña(nombre);
int edad = input();
willt’aña(edad);
```
