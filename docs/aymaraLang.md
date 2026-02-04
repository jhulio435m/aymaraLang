# Referencia del lenguaje

Esta referencia describe la sintaxis completa, operadores, palabras clave y
funciones integradas de AymaraLang. Está pensada para consulta rápida durante el
desarrollo.

## Tipos soportados

- `jakhüwi` → números enteros
- `aru` → cadenas
- `chiqa` → booleanos
- `listaña` → listas (soporte inicial)
- `mapa` → mapas (Español)

### Valores booleanos

En `aym` los literales lógicos utilizan vocabulario aymara. La palabra `utji`
representa **verdadero** (`true`) y `janiutji` equivale a **falso** (`false`).
Son útiles para inicializar variables y para las condiciones en estructuras de
control.

```aymara
qallta
yatiya chiqa bandera = utji;
suti (bandera) {
    qillqa("activado");
}

yatiya chiqa otra = janiutji;
suti (otra) {
    qillqa("esto no se imprime");
} jani {
    qillqa("desactivado");
}
tukuya
```

## Sintaxis soportada

- Variables y asignación.
- Impresión con `qillqa(expr)` y `write(str)`.
- Control de flujo: `suti`/`jani`, `kunawsati`, `sapüru`.
- Funciones con `lurawi nombre(tipo param) { ... }`.
- Expresiones aritméticas `+ - * / % ^` y operadores unarios `-expr`, `+expr`, `!expr`.
- Operadores lógicos `&&`, `||`, `!`, comparaciones `== != < <= > >=`.
- Comentarios `//` y `/* */`.
- Lectura de consola con `input()`.
- Longitud de cadenas con `length()`.
- Números aleatorios con `random(max)`.
- Pausa de ejecución con `sleep(ms)`.
- Arreglos dinámicos con `array(n)`, `array_get(arr, i)`, `array_set(arr, i, v)`,
  `array_free(arr)`, `array_length(arr)`.
- Funciones matemáticas con `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sqrt`,
  `pow`, `exp`, `log`, `log10`, `floor`, `ceil`, `round`, `fabs`.

## Ejemplos de sintaxis

### Entrada y salida
```aymara
qallta
yatiya aru nombre = input();
qillqa(nombre);
tukuya
```

### Arreglos dinámicos
```aymara
qallta
yatiya jakhüwi size = 3;
yatiya jakhüwi arr = array(size);
array_set(arr, 0, 10);
array_set(arr, 1, 20);
array_set(arr, 2, 30);
qillqa(array_get(arr, 1));
array_free(arr);
tukuya
```

### Funciones y recursividad
```aymara
lurawi fact(jakhüwi n) : jakhüwi {
    suti (n == 0) {
        kuttaya(1);
    }
    kuttaya(n * fact(n - 1));
}

qillqa(fact(5));
```

### Bucle `sapüru`

```aymara
qallta
sapüru (yatiya jakhüwi x = 0; x < 4; x = x + 1;) {
    qillqa(x);
}
tukuya
```

## Errores comunes

- Variable no declarada.
- Tipos incompatibles en asignaciones o expresiones.
- `return` fuera de una función.

## Palabras clave

`qillqa`, `write`, `sleep`, `array`, `array_get`, `array_set`, `array_free`,
`array_length`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sqrt`, `pow`, `exp`,
`log`, `log10`, `floor`, `ceil`, `round`, `fabs`, `qallta`, `tukuya`, `yatiya`,
`suti`, `jani`, `kunawsati`, `sapüru`, `lurawi`, `kuttaya`, `jakhüwi`, `aru`,
`chiqa`, `listaña`, `mapa`, `utji`, `janiutji`, `input`, `length`, `random`.

## Glosario de palabras clave

| Aimara / Español | Significado |
|------------------|------------|
| `qillqa`         | imprimir |
| `write`          | imprimir sin salto |
| `qallta`         | inicio de programa |
| `tukuya`         | fin de programa |
| `yatiya`         | declarar |
| `suti`           | if |
| `jani`           | else |
| `kunawsati`      | while |
| `sapüru`         | for |
| `lurawi`         | func |
| `kuttaya`        | return |
| `jakhüwi`        | numérico |
| `aru`            | string |
| `chiqa`          | bool |
| `listaña`        | lista |
| `mapa`           | mapa |
| `utji`           | true |
| `janiutji`       | false |

---

**Anterior:** [Guía de características](guide.md) | **Siguiente:** [Gramática formal](grammar.md)
