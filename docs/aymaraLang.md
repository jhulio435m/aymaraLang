# Referencia del lenguaje

Esta referencia describe la sintaxis completa, operadores, palabras clave y
funciones integradas de AymaraLang. Está pensada para consulta rápida durante el
desarrollo.

## Tipos soportados

- `jakhüwi` → números enteros
- `aru` → cadenas
- `chiqa` → booleanos
- `t'aqa` / `listaña` → listas (soporte inicial)
- `mapa` → mapas (experimental)

### Valores booleanos

En `aym` los literales lógicos utilizan vocabulario aymara. La palabra `chiqa`
(`utji`) representa **verdadero** (`true`) y `k'ari` (`janiutji`) equivale a
**falso** (`false`). Son útiles para inicializar variables y para las
condiciones en estructuras de control.

```aymara
yatiya chiqa bandera = chiqa;
jisa (bandera) {
    qillqa("activado");
}

yatiya chiqa otra = k'ari;
jisa (otra) {
    qillqa("esto no se imprime");
} maysatxa {
    qillqa("desactivado");
}
```

## Sintaxis soportada

- Variables y asignación.
- Impresión con `qillqa(expr)` y `write(str)`.
- Control de flujo: `jisa`/`maysatxa`, `ukhakamaxa`, `taki`.
- Funciones con `lurawi nombre(tipo param) { ... }`.
- Expresiones aritméticas `+ - * / % ^` y operadores unarios `-expr`, `+expr`, `!expr`.
- Operadores lógicos `&&`, `||`, `!`, comparaciones `== != < <= > >=`.
- Comentarios `//` y `/* */`.
- Lectura de consola con `katu(prompt, end)` o `input()`.
- Longitud de cadenas con `length()`.
- Números aleatorios con `random(max)`.
- Pausa de ejecución con `sleep(ms)`.
- Arreglos dinámicos con `array(n)`, `array_get(arr, i)`, `array_set(arr, i, v)`,
  `array_free(arr)`, `array_length(arr)`.
- Listas con literales `[]`, `largo(lista)` y `push(lista, valor)`.
- Funciones matemáticas con `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sqrt`,
  `pow`, `exp`, `log`, `log10`, `floor`, `ceil`, `round`, `fabs`.
- Operador ternario `cond ? a : b`.
- Manejo de errores con `yant'aña`/`katjaña`/`tukuyawi` y `pantja`.

## Ejemplos de sintaxis

### Entrada y salida
```aymara
yatiya aru nombre = katu("Suti?: ", tuku="");
qillqa(nombre);
```

### Arreglos dinámicos
```aymara
yatiya jakhüwi size = 3;
yatiya jakhüwi arr = array(size);
array_set(arr, 0, 10);
array_set(arr, 1, 20);
array_set(arr, 2, 30);
qillqa(array_get(arr, 1));
array_free(arr);
```

### Funciones y recursividad
```aymara
lurawi fact(jakhüwi n) : jakhüwi {
    jisa (n == 0) {
        kuttaya 1;
    }
    kuttaya n * fact(n - 1);
}

qillqa(fact(5));
```

### Bucle `taki`

```aymara
taki (yatiya jakhüwi x = 0; x < 4; x++) {
    qillqa(x);
}
```

## Errores comunes

- Variable no declarada.
- Tipos incompatibles en asignaciones o expresiones.
- `return` fuera de una función.

## Palabras clave

`qillqa`, `write`, `sleep`, `array`, `array_get`, `array_set`, `array_free`,
`array_length`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sqrt`, `pow`, `exp`,
`log`, `log10`, `floor`, `ceil`, `round`, `fabs`, `katu`, `largo`, `push`,
`qallta`, `tukuya`, `yatiya`, `jisa`, `maysatxa`, `ukhakamaxa`, `taki`,
`p'akhiña`, `sarantaña`, `lurawi`, `kuttaya`, `jakhüwi`, `aru`, `chiqa`,
`t'aqa`, `listaña`, `mapa`, `k'ari`, `utji`, `janiutji`, `input`, `length`,
`random`, `yant'aña`, `katjaña`, `tukuyawi`, `pantja`.

## Glosario de palabras clave

| Aimara / Español | Significado |
|------------------|------------|
| `qillqa`         | imprimir |
| `write`          | imprimir sin salto |
| `qallta`         | inicio de programa |
| `tukuya`         | fin de programa |
| `yatiya`         | declarar |
| `jisa`           | if |
| `maysatxa`       | else |
| `ukhakamaxa`     | while |
| `taki`           | for |
| `p'akhiña`       | break |
| `sarantaña`      | continue |
| `lurawi`         | func |
| `kuttaya`        | return |
| `yant'aña`       | try |
| `katjaña`        | catch |
| `tukuyawi`       | finally |
| `pantja`         | throw |
| `jakhüwi`        | numérico |
| `aru`            | string |
| `chiqa`          | bool / true |
| `t'aqa`          | lista |
| `mapa`           | mapa |
| `utji`           | true (legacy) |
| `k'ari`          | false |
| `janiutji`       | false (legacy) |

---

**Anterior:** [Guía de características](guide.md) | **Siguiente:** [Gramática formal](grammar.md)
